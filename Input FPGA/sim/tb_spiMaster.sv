//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: Tests spiMaster on its own: bit order, sclk timing, the
//         spiLoad handshake, back-to-back transfers, and reset mid-transfer.

`timescale 1ns/1ps

module tb_spiMaster;
    import tbCheckPkg::*;

    localparam int      DATAWIDTH         = 18;
    localparam int      DATAWIDTHWIDTH    = 5;
    localparam int      SCLKCOUNTER       = 23; // counts 0..23 -> 24 clk per half sclk period
    localparam int      SCLKCOUNTERWIDTH  = 5;
    localparam int      HALFPERIOD_CLKS   = SCLKCOUNTER + 1;
    localparam realtime CLK_PERIOD_NS     = 1000.0 / 48.0; // HSOSC at 48 MHz
    localparam realtime HALFPERIOD_NS     = HALFPERIOD_CLKS * CLK_PERIOD_NS;
    localparam int      IDLE_CHECK_CYCLES = 2000;
    localparam int      ABORT_AFTER_BITS  = 5;
    localparam int      RESET_HOLD_CYCLES = 10;
    localparam int      BACK_TO_BACK      = 3;
    localparam realtime TIMEOUT_NS        = 2_000_000.0;

    logic                 clk     = 1'b0;
    logic                 reset   = 1'b1;
    logic                 spiFlag = 1'b0;
    logic [DATAWIDTH-1:0] data    = '0;
    logic                 spiLoad, sclk, ce, mosi;

    always #(CLK_PERIOD_NS / 2.0) clk = ~clk;

    spiMaster #(
        .DATAWIDTH(DATAWIDTH),
        .DATAWIDTHWIDTH(DATAWIDTHWIDTH),
        .SCLKCOUNTER(SCLKCOUNTER),
        .SCLKCOUNTERWIDTH(SCLKCOUNTERWIDTH)
    ) dut (
        .clk(clk),
        .reset(reset),
        .data(data),
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

    // spiFlagger relies on spiLoad being exactly one clk wide.
    int   loadPulses = 0;
    logic spiLoadLastCycle = 1'b0;
    always @(posedge clk) begin
        if (spiLoad === 1'b1) loadPulses++;
        if (spiLoad === 1'b1 && spiLoadLastCycle === 1'b1)
            check(1'b0, "spiLoad was high for more than one clk cycle");
        spiLoadLastCycle = spiLoad;
    end

    // Raise spiFlag and drop it the cycle after spiLoad, the way spiFlagger does.
    task automatic startTransfer(input logic [DATAWIDTH-1:0] value);
        @(negedge clk);
        data    = value;
        spiFlag = 1'b1;
        wait (spiLoad === 1'b1);
        @(negedge clk);
        spiFlag = 1'b0;
    endtask

    task automatic sendAndCheck(input logic [DATAWIDTH-1:0] value, input string name);
        int wordsBefore = slave.wordCount;
        int loadsBefore = loadPulses;
        startTransfer(value);
        wait (slave.wordCount == wordsBefore + 1);
        check(slave.words[wordsBefore] === value && slave.bitCounts[wordsBefore] == DATAWIDTH,
              $sformatf("%s: sent %b, slave got %b (%0d bits)", name, value,
                        slave.words[wordsBefore], slave.bitCounts[wordsBefore]));
        check(loadPulses == loadsBefore + 1, $sformatf("%s: exactly one spiLoad pulse", name));
    endtask

    initial begin
        #(TIMEOUT_NS);
        check(1'b0, "timeout: a transfer never finished");
        reportResults("tb_spiMaster", slave.timingErrors);
        $stop;
    end

    initial begin
        logic [DATAWIDTH-1:0] streamValues[BACK_TO_BACK];
        int wordsBefore, loadsBefore, idleViolations;

        // Reset and idle behaviour
        #1 reset = 1'b0;
        repeat (RESET_HOLD_CYCLES) @(negedge clk);
        check(ce === 1'b1 && sclk === 1'b0 && mosi === 1'b0 && spiLoad === 1'b0,
              "outputs idle during reset (ce=1, sclk=0, mosi=0, spiLoad=0)");
        reset = 1'b1;

        idleViolations = 0;
        repeat (IDLE_CHECK_CYCLES) begin
            @(negedge clk);
            if (ce !== 1'b1 || sclk !== 1'b0) idleViolations++;
        end
        check(idleViolations == 0 && slave.wordCount == 0 && loadPulses == 0,
              "no bus activity while spiFlag is low");

        // Bit patterns: catch stuck bits, swapped order, and a missing MSB or LSB
        sendAndCheck('0,                    "all zeros");
        sendAndCheck('1,                    "all ones");
        sendAndCheck(18'b10_1010_1010_1010_1010, "alternating, MSB = 1");
        sendAndCheck(18'b01_0101_0101_0101_0101, "alternating, MSB = 0");
        sendAndCheck(18'b10_0000_0000_0000_0000, "only the MSB set");
        sendAndCheck(18'b00_0000_0000_0000_0001, "only the LSB set");
        sendAndCheck(18'h2A5C3,             "mixed value");

        // spiFlag held high: back-to-back transfers. Data changes right after
        // each load, so a word corrupted mid-transfer would show up here.
        streamValues = '{18'h0F0F0, 18'h30303, 18'h1C71C};
        wordsBefore  = slave.wordCount;
        loadsBefore  = loadPulses;
        @(negedge clk);
        data    = streamValues[0];
        spiFlag = 1'b1;
        for (int i = 0; i < BACK_TO_BACK; i++) begin
            wait (spiLoad === 1'b1);
            @(negedge clk);
            if (i < BACK_TO_BACK - 1) data = streamValues[i + 1];
            else begin
                data    = ~streamValues[i];
                spiFlag = 1'b0;
            end
        end
        wait (slave.wordCount == wordsBefore + BACK_TO_BACK);
        for (int i = 0; i < BACK_TO_BACK; i++)
            check(slave.words[wordsBefore + i] === streamValues[i] &&
                  slave.bitCounts[wordsBefore + i] == DATAWIDTH,
                  $sformatf("back-to-back word %0d: sent %b, slave got %b", i,
                            streamValues[i], slave.words[wordsBefore + i]));
        check(loadPulses == loadsBefore + BACK_TO_BACK, "back-to-back: one spiLoad per transfer");
        repeat (4 * HALFPERIOD_CLKS) @(negedge clk);
        check(slave.wordCount == wordsBefore + BACK_TO_BACK, "no extra transfer after spiFlag dropped");

        // Reset in the middle of a word (while sclk is high)
        wordsBefore = slave.wordCount;
        startTransfer(18'h3FFFF);
        repeat (ABORT_AFTER_BITS) @(posedge sclk);
        #(HALFPERIOD_NS / 2.0) reset = 1'b0;
        #1;
        check(ce === 1'b1 && sclk === 1'b0 && mosi === 1'b0 && spiLoad === 1'b0,
              "reset mid-transfer returns outputs to idle immediately");
        check(slave.wordCount == wordsBefore + 1 && slave.bitCounts[wordsBefore] == ABORT_AFTER_BITS,
              $sformatf("aborted transfer stopped after %0d bits", ABORT_AFTER_BITS));
        repeat (RESET_HOLD_CYCLES) @(negedge clk);
        reset = 1'b1;
        repeat (HALFPERIOD_CLKS) @(negedge clk);
        sendAndCheck(18'h2A5C3, "first word after mid-transfer reset");

        reportResults("tb_spiMaster", slave.timingErrors);
        $stop;
    end
endmodule
