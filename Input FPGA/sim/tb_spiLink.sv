//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: Tests spiFlagger + spiMaster together, wired as in inputFPGA_top:
//         one transfer per change, none without a change, and the newest
//         value still gets sent when an input changes mid-transfer.

`timescale 1ns/1ps

module tb_spiLink;
    import tbCheckPkg::*;

    localparam int      DATAWIDTH        = 18;
    localparam int      DATAWIDTHWIDTH   = 5;
    localparam int      SCLKCOUNTER      = 23;
    localparam int      SCLKCOUNTERWIDTH = 5;
    localparam int      COUNTERWIDTH     = 16;
    // The flagger checks every 480 clk (10 us) here instead of every 1 ms,
    // so at least one check lands inside each ~20 us SPI transfer.
    localparam int      TICK_MAX         = 479;
    localparam int      QUIET_TICKS      = 5;
    localparam int      RESET_HOLD_CYCLES = 10;
    localparam int      CHANGE_SPACING_CYCLES = 10;
    localparam realtime CLK_PERIOD_NS    = 1000.0 / 48.0; // HSOSC at 48 MHz
    localparam realtime HALFPERIOD_NS    = (SCLKCOUNTER + 1) * CLK_PERIOD_NS;
    localparam realtime TIMEOUT_NS       = 2_000_000.0;

    localparam logic [DATAWIDTH-1:0] VALUE_A = 18'h2A5A5;
    localparam logic [DATAWIDTH-1:0] VALUE_B = 18'h3C0F0;
    localparam logic [DATAWIDTH-1:0] VALUE_C = 18'h00F0F;
    localparam logic [DATAWIDTH-1:0] VALUE_D = 18'h12345;
    localparam logic [DATAWIDTH-1:0] VALUE_E = 18'h3FFFF;

    logic                    clk   = 1'b0;
    logic                    reset = 1'b1;
    logic [DATAWIDTH-1:0]    sin   = '0; // stands in for debouncedInputs
    logic [COUNTERWIDTH-1:0] msCount;
    logic [DATAWIDTH-1:0]    lastInput;
    logic                    spiFlag, spiLoad, sclk, ce, mosi;

    always #(CLK_PERIOD_NS / 2.0) clk = ~clk;

    counter #(.WIDTH(COUNTERWIDTH), .MAX(TICK_MAX)) tickCounter(
        .clk(clk),
        .reset(reset),
        .enable(1'b1),
        .count(msCount),
        .blink()
    );

    spiFlagger #(.WIDTH(DATAWIDTH), .COUNTERWIDTH(COUNTERWIDTH), .COUNTERMAX(TICK_MAX)) flagger(
        .clk(clk),
        .reset(reset),
        .counter(msCount),
        .sin(sin),
        .sold(lastInput),
        .spiFlag(spiFlag),
        .spiLoad(spiLoad)
    );

    spiMaster #(
        .DATAWIDTH(DATAWIDTH),
        .DATAWIDTHWIDTH(DATAWIDTHWIDTH),
        .SCLKCOUNTER(SCLKCOUNTER),
        .SCLKCOUNTERWIDTH(SCLKCOUNTERWIDTH)
    ) master (
        .clk(clk),
        .reset(reset),
        .data(lastInput),
        .spiFlag(spiFlag),
        .spiLoad(spiLoad),
        .sclk(sclk),
        .ce(ce),
        .mosi(mosi)
    );

    spiSlaveModel #(.DATAWIDTH(DATAWIDTH), .HALFPERIOD_NS(HALFPERIOD_NS)) slave (
        .sclk(sclk),
        .ce(ce),
        .mosi(mosi)
    );

    task automatic waitTicks(input int ticks);
        repeat (ticks) @(posedge clk iff msCount == TICK_MAX);
    endtask

    initial begin
        #(TIMEOUT_NS);
        check(1'b0, "timeout: an expected transfer never finished");
        reportResults("tb_spiLink", slave.timingErrors);
        $stop;
    end

    initial begin
        int wordsBefore;

        #1 reset = 1'b0;
        repeat (RESET_HOLD_CYCLES) @(negedge clk);
        reset = 1'b1;

        // Inputs equal the reset value of sold: nothing to send
        waitTicks(QUIET_TICKS);
        check(slave.wordCount == 0, "no transfer while inputs match their reset value");

        // One change -> exactly one transfer
        @(negedge clk) sin = VALUE_A;
        wait (slave.wordCount == 1);
        waitTicks(QUIET_TICKS);
        check(slave.wordCount == 1 && slave.words[0] === VALUE_A && slave.bitCounts[0] == DATAWIDTH,
              $sformatf("single change: one transfer of %b (got %0d, first = %b)",
                        VALUE_A, slave.wordCount, slave.words[0]));

        // Change during a transfer: B goes out, then C must follow
        wordsBefore = slave.wordCount;
        @(negedge clk) sin = VALUE_B;
        @(negedge ce);
        @(negedge clk) sin = VALUE_C;
        wait (slave.wordCount == wordsBefore + 2);
        waitTicks(QUIET_TICKS);
        check(slave.wordCount == wordsBefore + 2, "change mid-transfer: exactly two transfers");
        check(slave.words[wordsBefore] === VALUE_B,
              $sformatf("change mid-transfer: first word %b (got %b)", VALUE_B, slave.words[wordsBefore]));
        check(slave.words[wordsBefore + 1] === VALUE_C,
              $sformatf("change mid-transfer: newest value %b sent next (got %b)",
                        VALUE_C, slave.words[wordsBefore + 1]));

        // Two changes between flagger checks: only the later one is sent
        wordsBefore = slave.wordCount;
        waitTicks(1);
        @(negedge clk) sin = VALUE_D;
        repeat (CHANGE_SPACING_CYCLES) @(negedge clk);
        sin = VALUE_E;
        wait (slave.wordCount == wordsBefore + 1);
        waitTicks(QUIET_TICKS);
        check(slave.wordCount == wordsBefore + 1 && slave.words[wordsBefore] === VALUE_E,
              $sformatf("two changes between checks: one transfer of %b (got %b)",
                        VALUE_E, slave.words[wordsBefore]));

        reportResults("tb_spiLink", slave.timingErrors);
        $stop;
    end
endmodule
