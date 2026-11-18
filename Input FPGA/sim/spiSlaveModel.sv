//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: Testbench-only SPI slave (mode 0, MSB first). Records every
//         transfer seen while ce is low and checks the bus timing
//         the STM32 slave depends on. Shared by all SPI testbenches.

`timescale 1ns/1ps

module spiSlaveModel #(
    parameter int      DATAWIDTH     = 18,
    parameter realtime HALFPERIOD_NS = 500.0
)(
    input logic sclk,
    input logic ce,
    input logic mosi
);
    // Every edge lands on a whole clk cycle, so this only absorbs rounding.
    localparam realtime TOLERANCE_NS = 1.0;

    // One entry per transfer (complete or aborted), read by the testbenches.
    logic [DATAWIDTH-1:0] words[$];
    int                   bitCounts[$];
    int                   wordCount    = 0;
    int                   timingErrors = 0;

    logic [DATAWIDTH-1:0] shiftIn;
    int      bitsReceived       = 0;
    bit      inTransfer         = 1'b0;
    realtime ceFallTime         = 0.0;
    realtime ceRiseTime         = -1.0; // negative = no transfer has ended yet
    realtime lastRiseTime       = 0.0;
    realtime lastFallTime       = 0.0;
    realtime lastMosiChangeTime = 0.0;

    function automatic void timingError(input string message);
        timingErrors++;
        $display("[%0t] SPI TIMING ERROR: %s", $realtime, message);
    endfunction

    function automatic realtime absTime(input realtime value);
        return (value < 0.0) ? -value : value;
    endfunction

    always @(negedge ce) begin
        if (ceRiseTime >= 0.0 && $realtime - ceRiseTime < HALFPERIOD_NS - TOLERANCE_NS)
            timingError("ce was high for less than half an sclk period between transfers");
        ceFallTime   = $realtime;
        bitsReceived = 0;
        shiftIn      = '0;
        inTransfer   = 1'b1;
    end

    always @(posedge sclk) begin
        if (ce !== 1'b0) timingError("sclk rose while ce was not low");
        else begin
            if (bitsReceived == 0) begin
                if ($realtime - ceFallTime < HALFPERIOD_NS - TOLERANCE_NS)
                    timingError("first sclk rise came too soon after ce fell");
            end else if (absTime(($realtime - lastRiseTime) - 2.0 * HALFPERIOD_NS) > TOLERANCE_NS)
                timingError($sformatf("sclk period was %0.1f ns, expected %0.1f ns",
                                      $realtime - lastRiseTime, 2.0 * HALFPERIOD_NS));
            if ($realtime - lastMosiChangeTime < HALFPERIOD_NS / 2.0)
                timingError("mosi changed too close before sclk rose (setup)");
            shiftIn      = {shiftIn[DATAWIDTH-2:0], mosi};
            bitsReceived++;
            lastRiseTime = $realtime;
        end
    end

    always @(negedge sclk) lastFallTime = $realtime;

    always @(mosi) begin
        if (ce === 1'b0 && bitsReceived > 0 && $realtime - lastRiseTime < HALFPERIOD_NS / 2.0)
            timingError("mosi changed too soon after sclk rose (hold)");
        lastMosiChangeTime = $realtime;
    end

    always @(posedge ce) if (inTransfer) begin
        // A reset aborts mid-word and drops sclk in the same instant, so the
        // end-of-transfer checks only apply to complete words.
        if (bitsReceived == DATAWIDTH) begin
            if (sclk !== 1'b0) timingError("ce rose while sclk was high");
            if ($realtime - lastFallTime < HALFPERIOD_NS - TOLERANCE_NS)
                timingError("ce rose too soon after the last sclk fall");
        end
        words.push_back(shiftIn);
        bitCounts.push_back(bitsReceived);
        inTransfer = 1'b0;
        ceRiseTime = $realtime;
        wordCount++; // updated last so a testbench waiting on it sees the new entry
    end
endmodule
