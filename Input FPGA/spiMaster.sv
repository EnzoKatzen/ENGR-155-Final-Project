//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: sends a stream of configurable length at configurable speed over SPI

module spiMaster #(
	parameter DATAWIDTH = 18,
    parameter DATAWIDTHWIDTH = 5,
	parameter SCLKCOUNTER = 23,
    parameter SCLKCOUNTERWIDTH = 5
)(
	input  logic clk,
    input  logic reset,
	input  logic [DATAWIDTH-1:0] data,
    input  logic spiFlag,
    output logic spiLoad,
    output logic sclk,
    output logic ce,
    output logic mosi
);
    logic enable;
    logic [SCLKCOUNTERWIDTH-1:0] sclk_count;
	counter #(.WIDTH(SCLKCOUNTERWIDTH), .MAX(SCLKCOUNTER)) sclkCounter(
		.clk(clk),
		.reset(reset),
		.enable(enable),
        .count(sclk_count)
	    );
    
    logic done;
    typedef enum logic [2:0] {IDLE = 3'b000, SETUP = 3'b001, SHIFT = 3'b010, HOLD = 3'b011, GAP = 3'b100} statetype;
    statetype state, nextstate;

    always_comb
        case (state)
            IDLE:  nextstate = spiFlag ? SETUP : IDLE;
            SETUP: nextstate = (sclk_count == SCLKCOUNTER) ? SHIFT : SETUP;
            SHIFT: nextstate = done ? HOLD : SHIFT;
            HOLD:  nextstate = (sclk_count == SCLKCOUNTER) ? GAP : HOLD;
            GAP:   nextstate = (sclk_count == SCLKCOUNTER) ? IDLE : GAP;
            default: nextstate = IDLE;
        endcase

    logic [DATAWIDTH-1:0] stream;
    logic [DATAWIDTHWIDTH-1:0] toSend;
    always_ff @(posedge clk, negedge reset)
        if (!reset) begin
            state <= IDLE;
            enable <= 1'b0;
            mosi <= 1'b0;
            stream <= 'b0;
            toSend <= 'b0;
            sclk <= 1'b0;
            spiLoad <= 1'b0;
            ce <= 1'b1;
            done <= 1'b0;
        end else begin
        if (state == IDLE) begin
            ce <= 1'b1;
            sclk <= 1'b0;
            if(spiFlag) begin
                done <= 1'b0;
                stream <= data;
                toSend <= DATAWIDTH;
                spiLoad <= 1'b1;
                enable <= 1'b1;
            end else enable <= 1'b0;
        end else begin
            spiLoad <= 1'b0;
            if (state == SETUP) begin
                mosi <= stream[toSend-1]; 
                if(sclk_count == SCLKCOUNTER) ce <= 1'b0;
            end else if(state == SHIFT && sclk_count == SCLKCOUNTER) begin
                if(!sclk) toSend <= toSend -1;
                else if(toSend > 0) mosi <= stream[toSend-1];
                else done <= 1'b1;
                sclk <= ~sclk;
            end else if (state == GAP) begin
                mosi <= 1'b0;
                ce <= 1'b1;
            end
        end
        state <= nextstate;
    end

endmodule
