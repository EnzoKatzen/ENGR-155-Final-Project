//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: sets a flag when values have changed
//         so they can send via SPI.

module spiFlagger #(
	parameter WIDTH = 18,
	parameter COUNTERWIDTH = 16,
	parameter COUNTERMAX = 16'd47999
)(
	input  logic clk,
    input  logic reset,
	input  logic [COUNTERWIDTH-1:0] counter,
	input  logic [WIDTH-1:0] sin,
	output logic [WIDTH-1:0] sold,
    output logic             spiFlag,
    input  logic             spiLoad
);
    logic spiLoadOld;
	always_ff @ (posedge clk, negedge reset) begin
        if(!reset) begin 
            spiFlag <= 1'b0;
            spiLoadOld <= 1'b0;
            sold <= 'b0;
        end else begin
            if (spiLoad && !spiLoadOld) begin //if spiLoad is high, but spiLoadOld is not, then it is the rising edge of spiLoad
                spiFlag <= 1'b0; //so SPI just started. Drop the flag to run SPI.
            end
            if(counter == COUNTERMAX && sin != sold) begin //if sin is different than sold, it has changed
                spiFlag <= 1'b1; //so raise the flag
                sold <= sin;     //and set sold to the current sin.
            end
            spiLoadOld <= spiLoad;
        end
    end
endmodule
