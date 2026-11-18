# Architecture

## Commands

Toolchain: **Lattice Radiant** (synthesis, place & route, programming) + **Questa**
(simulation). Exact command lines still TBD; fill in during task 1.

| Purpose | Command |
|---|---|
| Simulate (one module) | TBD |
| Simulate (all testbenches) | TBD |
| Lint | TBD |
| Synthesize + place & route (Output FPGA) | TBD |
| Program (Output FPGA) | TBD |
| Build / flash (MCU) | TBD |

## Overview

Final project with three hardware pieces: an MCU, a "Debouncer FPGA", and an "Output FPGA".
This document currently covers only the **Output FPGA** and the MCU code that talks to it.
The existing `MCU/LUTs.c` and the `Debouncer FPGA/` folder belong to a different part of the
project and are not described here yet.

The Output FPGA is a Lattice **iCE40UP5K-SG48** on a **UPduino v3.1**. It:

- is an **SPI slave** to the MCU, receiving three kinds of messages (LEDs, select image,
  store image);
- stores up to **64 monochrome 128×128 images** (16384 bits each) in its on-chip SPRAM;
- is an **SPI master** to an Adafruit 1.12" 128×128 mono OLED (ADA5297, SH1107 controller,
  module ZJY-2828TSWIG01), continuously re-sending the currently selected image;
- drives **3 external LEDs** on GPIO, **active low** (LED bit 1 → pin low). All off at reset.

Until the MCU sends its first select (`10`) message, the display is initialized, cleared,
and left blank, because SPRAM contents are undefined at power-up.

HDL: SystemVerilog (assumed — confirm).

## Structure

```
Output FPGA/
  src/
    output_fpga_pkg.sv   shared constants: protocol codes, widths, memory geometry, clocks
    top.sv               pins, SB_HFOSC clock, power-on reset, wiring
    spi_slave_rx.sv      MCU link: synchronizes SCK/MOSI/CS, emits one bit per SCK edge
    cmd_decoder.sv       parses messages -> LED register, image select, RAM write port
    image_ram.sv         4x SB_SPRAM256KA + write/read arbiter
    spi_master_tx.sv     generic byte-wide SPI master (mode 0, MSB first) with D/C pin
    sh1107_ctrl.sv       display reset, init sequence, frame streaming loop
  sim/
    tb_<module>.sv       one testbench per module
    tb_top.sv            end-to-end: MCU BFM -> FPGA -> SH1107 model
    mcu_spi_bfm.sv       testbench driver that sends protocol messages
    sh1107_model.sv      testbench receiver that decodes display SPI into a framebuffer
  constraints/
    pins.pdc             pin assignments (Radiant)
MCU/
  fpga_link.h / .c       encodes the three messages and sends them over SPI (deferred)
tools/
  img2bits.py            (optional) PNG -> 2048-byte row-major image / C array
```

### Data flow

```
MCU --SPI--> spi_slave_rx --bits--> cmd_decoder --+--> led_state[2:0] --------> LED pins
                                                  +--> select_image[5:0] --+
                                                  +--> write port --> image_ram
                                                                           | read port
                                                         sh1107_ctrl <-----+
                                                              |
                                                        spi_master_tx --SPI--> OLED
```

## MCU → FPGA protocol

- SPI mode 0 (CPOL=0, CPHA=0), **MSB first**. FPGA has no MISO output (write-only link).
- **One message per CS-low transaction.** The FPGA resets its parser when CS goes low.
  Bits after the end of a message are ignored until CS goes high. This lets every message
  be sent as whole bytes.
- The 2-bit start code is the first two bits on the wire, written left to right.

| Code | Meaning | Following bits | Total | Sent as |
|---|---|---|---|---|
| `00` | Set LEDs | 3 bits: `led[2] led[1] led[0]` (1 = on) | 5 | 1 byte: `00 abc xxx` |
| `10` | Select displayed image | 6-bit image index | 8 | 1 byte |
| `01` | Store image | 6-bit image index, then 16384 image bits | 16392 | 2049 bytes |
| `11` | Undefined | — | — | ignored until CS high |

- **Image format:** column-major. Column 0 (leftmost) first, 128 columns of 128 bits.
  Within a column the first bit is the top pixel (y = 0). 1 = pixel on.
  (Chosen over row-major because it matches the SH1107's vertical bytes. See below.)
- **Select** latches a new image index; the display controller switches to it at the next
  frame boundary (no half-old/half-new frames). It keeps re-sending that image until another
  `10` message arrives.
- **Store** to the image currently on screen shows up on the next frame (tearing possible
  during the upload — acceptable).
- An aborted store (CS high early) leaves a partially written image. No error is reported.

## Image memory map

64 images × 16384 bits = 1 Mbit = exactly the UP5K's four 256 Kbit SPRAM blocks
(each 16K × 16 bit). EBR is not used for images.

- One RAM word = 16 vertically stacked pixels. One image = 1024 words. One column = 8 words.
- Word address (16 bits) = `{image[5:0], x[6:0], y[6:4]}`. That is also just
  `{image, word_index}` where `word_index` counts words in arrival order.
  - `addr[15:14]` selects the SPRAM block, `addr[13:0]` is the address inside it.
- Pixel `y` within a word is at bit `15 - y[3:0]` (first received bit → MSB → top pixel).

## Display (SH1107) notes

Sources: ZJY-2828TSWIG01 module spec, SH1107 V2.1 datasheet.

- Board ships in **I2C mode**; bridge solder jumper **J1** for SPI.
- 4-wire SPI signals: SCLK, MOSI (SI), CS, DC (A0: 1 = data, 0 = command), RST. Write-only.
- SH1107 shifts SI in on each **rising** SCLK edge, MSB (D7) first, and samples A0 on the
  8th clock. A CS falling edge resets its shift register.
- Timing at VDD 2.4–3.5 V: t_SCYC ≥ 250 ns (SCLK ≤ 4 MHz), SCLK high/low ≥ 100 ns,
  A0 setup/hold ≥ 150 ns, t_CSS ≥ 120 ns, t_CSH ≥ 60 ns. Plan: SCLK = sysclk / 8 = 3 MHz.
- Reset: RST low ≥ 10 µs, then ≤ 2 µs until ready (ZJY says wait ≥ 3 µs). Plan: 1 ms each.
- Power-on (SH1107 p. 43–44): reset → init commands → clear RAM → `AF` (display on) →
  **wait 100 ms** → send display data.
- Init sequence (ZJY §4.5): `AE`, `D5 50`, `20`, `81 4F`, `AD 8A`, `A0`, `C0`, `DC 00`,
  `D3 60`, `D9 22`, `DB 35`, `A8 7F`, `A4`, `A6`, clear, `AF`. `AD 8A` = internal DC-DC off
  (external VPP). Offset `D3 60` may need changing for the Adafruit board.
- Commands used by the frame loop: `B0 | p` page (0–15), `00 | c[3:0]` column low,
  `10 | c[6:4]` column high. Page addressing mode (`20`): column auto-increments.

### SH1107 RAM → screen mapping

- SH1107 RAM = 16 pages × 128 columns of bytes. Page `p` bit `Dk` drives segment
  `SEG(8p + k)`; column `c` drives `COM c` (SH1107 Figure 10, ADC = 0).
- On this module segments run along screen **rows** and commons along screen **columns**
  (ZJY §1.3: Segment 0 = Row 1, Common 0 = Column 1).
- So one display byte = **8 vertically stacked pixels**: byte (page p, column c) holds
  pixels x = c, y = 8p … 8p + 7, with **D0 = top** (y = 8p).
- Images are stored column-major, so display byte (page p, column c) is **half of one RAM
  word**: word `{image, x = c, p[3:1]}`, upper half (`[15:8]`) if `p[0] = 0`, else lower
  half (`[7:0]`). The top pixel is the word's MSB but the display's D0, so the 8 bits are
  **reversed** (pure wiring).
- Mirroring (`A0`/`A1`, `C0`/`C8`) and the `D3` offset still need a hardware check. The
  mapping lives in **one function** so a flip is a one-line fix. (A 90° rotation can't be
  fixed by commands; rotate the board instead.)

### Frame

For each page p = 0 … 15: commands `B0|p`, `00`, `10`, then 128 data bytes (c = 0 … 127),
one RAM read per data byte. 2096 bytes per frame ≈ 6 ms at 3 MHz (~160 frames/s).

## Clocking and reset

- Single clock domain: `SB_HFOSC` 48 MHz ÷ 2 = **24 MHz sysclk**.
- MCU SPI is oversampled: SCK, MOSI, CS each pass through a 2-flop synchronizer; edges are
  detected in the sysclk domain. Requires MCU SCK ≤ sysclk / 8 = **3 MHz**.
  Uploading one image at 3 MHz ≈ 5.5 ms.
- Power-on reset counter in `top.sv`; synchronous active-high reset internally.

## Module map

| Module | Purpose | Location | Doc |
|---|---|---|---|
| output_fpga_pkg | Shared constants | `Output FPGA/src/output_fpga_pkg.sv` | `docs/modules/output_fpga_pkg.md` |
| top | Pins, clock, reset, wiring | `Output FPGA/src/top.sv` | `docs/modules/top.md` |
| spi_slave_rx | MCU SPI bit receiver | `Output FPGA/src/spi_slave_rx.sv` | `docs/modules/spi_slave_rx.md` |
| cmd_decoder | Message parser | `Output FPGA/src/cmd_decoder.sv` | `docs/modules/cmd_decoder.md` |
| image_ram | 64-image SPRAM store + arbiter | `Output FPGA/src/image_ram.sv` | `docs/modules/image_ram.md` |
| spi_master_tx | Generic SPI master | `Output FPGA/src/spi_master_tx.sv` | `docs/modules/spi_master_tx.md` |
| sh1107_ctrl | Display init + frame streaming | `Output FPGA/src/sh1107_ctrl.sv` | `docs/modules/sh1107_ctrl.md` |
| fpga_link | MCU-side message encoder | `MCU/fpga_link.c` | `docs/modules/fpga_link.md` |

## Shared code registry

| Name | What it does | Location |
|---|---|---|
| `output_fpga_pkg` | Start codes, widths, image/RAM geometry, clock dividers. Every FPGA module imports it. | `Output FPGA/src/output_fpga_pkg.sv` |
| `spi_master_tx` | Byte-wide SPI master, mode 0, with per-byte D/C | `Output FPGA/src/spi_master_tx.sv` |
| `spi_slave_rx` | Oversampled SPI slave bit receiver | `Output FPGA/src/spi_slave_rx.sv` |
| `mcu_spi_bfm` | Testbench: sends protocol messages | `Output FPGA/sim/mcu_spi_bfm.sv` |
| `sh1107_model` | Testbench: decodes display SPI into a framebuffer | `Output FPGA/sim/sh1107_model.sv` |
| `fpga_link.h` | MCU copy of protocol constants | `MCU/fpga_link.h` |

## Conventions

- Protocol constants exist in two languages: `output_fpga_pkg.sv` and `MCU/fpga_link.h`.
  Change both together; each file names the other in a comment.
- No magic numbers in HDL: widths and counts come from `output_fpga_pkg`.
- One clock domain (sysclk). Anything from outside goes through a 2-flop synchronizer in
  the module that owns that input.
- Don't touch `Debouncer FPGA/` or `MCU/LUTs.*` as part of Output FPGA work.
