# Plan

## Goals

Build the Output FPGA (iCE40 UP5K): SPI slave to the MCU, 64-image SPRAM store, SPI master
to the 128×128 SH1107 OLED, and 3 LEDs. Protocol and design are in `docs/ARCHITECTURE.md`.

## Current focus

Architecture drafted and design questions answered (2026-11-16). Now walking through each
file's structure with the user. **The user may write the code themselves: don't hand tasks
to the implementer until the user says so.** Pin assignments: wait for the user to provide
them.

## Tasks

| # | Task | Depends on | Status | Tester? | Notes |
|---|---|---|---|---|---|
| 1 | `output_fpga_pkg` + `spi_slave_rx` + tb | — | planned | yes | CDC / oversampling |
| 2 | `cmd_decoder` + `mcu_spi_bfm` + tb | 1 | planned | yes | all 4 codes, CS framing, abort |
| 3 | `image_ram` (4× SPRAM, arbiter) + tb | 1 | planned | yes | can run parallel to 2 and 4 |
| 4 | `spi_master_tx` + tb | 1 | planned | no | can run parallel to 2 and 3 |
| 5 | `sh1107_ctrl` + `sh1107_model` + tb | 3, 4 | planned | yes | pixel mapping in one function |
| 6 | `top` + pin constraints + `tb_top`; synth, check utilization and 24 MHz timing | 2, 5 | planned | yes | end-to-end |
| 7 | MCU `fpga_link` driver | 1 (constants) | deferred | no | decide after FPGA side is done |
| 8 | Hardware bring-up: J1 jumper, init sequence, fix orientation/bit order | 6, 7 | planned | — | on the bench |
| 9 | (optional) `tools/img2bits.py` | — | planned | no | PNG → image bytes |

## Decision log

| Date | Decision | Why |
|---|---|---|
| 2026-11-16 | One message per CS-low transaction; extra bits ignored | Resyncs on every message; lets the MCU send whole bytes (LED msg padded to 8 bits) |
| 2026-11-16 | MSB first, SPI mode 0 | Standard; matches MCU byte order |
| 2026-11-16 | Images in the 4 SPRAM blocks, addr = `{image, y, x[6:4]}` | 64 × 16 Kbit = exactly 1 Mbit of SPRAM; image index bits pick the block |
| 2026-11-16 | Single 24 MHz clock domain, MCU SPI oversampled (SCK ≤ 3 MHz) | Avoids a second clock domain; bandwidth is far more than needed |
| 2026-11-16 | Display SCLK 3 MHz (sysclk/8) | SH1107 limit is 4 MHz at 3.3 V |
| 2026-11-16 | Image select takes effect at the next frame boundary | No mixed frames |
| 2026-11-16 | RAM write port has priority over display reads | Writes can't stall the SPI slave; display reads have lots of slack |
| 2026-11-16 | Toolchain: Lattice Radiant + Questa | User choice |
| 2026-11-16 | Display stays blank (init + clear) until the first `10` select message | SPRAM powers up with garbage |
| 2026-11-16 | LEDs external on GPIO, active low (1 bit in message → pin driven low) | User choice; reset = all off (pins high) |
| 2026-11-16 | MCU driver (task 7) deferred until the FPGA side is done | User choice |
| 2026-11-16 | Image format changed to **column-major** (was row-major) | SH1107 bytes are 8 vertical pixels; column-major gives one RAM read per display byte instead of 8. Whoever makes the images must output columns |
| 2026-11-16 | Board: UPduino v3.1, iCE40UP5K-SG48 | User's hardware |

## Open questions

- Pin assignments on the UPduino v3.1 (user will provide). (Needed by task 6.)
- Exact Radiant/Questa commands (scripted vs GUI) for the Commands table. (Needed by task 1.)

## Known issues / risks

- SH1107 command bytes confirmed from the SH1107 V2.1 datasheet. Mirroring and the `D3`
  offset still need a hardware check (task 8).
- Init sequence values (DC-DC `AD 8A`, offset `D3 60`) come from the bare module datasheet;
  the Adafruit board may need different ones.
- Protocol constants are duplicated in SV and C (unavoidable across languages).

## Consolidation candidates

- None yet.
