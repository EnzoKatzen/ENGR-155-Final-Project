//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 9/20/2026
//Purpose: Debounces both sides of an
//         input for switches / buttons

module edgeDebouncer #(
	parameter N = 10,
	parameter WIDTH = 18,
	parameter COUNTERWIDTH = 16,
	parameter COUNTERMAX = 16'd47999
)(
	input  logic clk,
	input  logic reset, 
	input  logic [COUNTERWIDTH-1:0] counter,
	input  logic [WIDTH-1:0] sold,
	input  logic [WIDTH-1:0] sin,
	output logic [WIDTH-1:0] sout
);
	logic [WIDTH-1:0] debouncedHigh;
    debouncer #(.COUNTERMAX(COUNTERMAX)) highDebouncer( //debounce the high inputs. If something is low, then it is either low or bouncing
        .clk(clk),
        .reset(reset),
        .counter(counter),
        .sin(sin),
        .sout(debouncedHigh)
    );
	
	logic [WIDTH-1:0] sinInverted;
	assign sinInverted = ~sin; //to make low inputs be high
	logic [WIDTH-1:0] debouncedLow;
	debouncer #(.COUNTERMAX(COUNTERMAX)) lowDebouncer( //debounce the low inputs. If something is low, then it is either high or bouncing
        .clk(clk),
        .reset(reset),
        .counter(counter),
        .sin(sinInverted),
        .sout(debouncedLow)
    );

	//If a value is low on both debouncedLow and debouncedHigh, then it is bouncing
	//In that case, set it to be the last value
	assign sout = debouncedHigh | (sold & ~debouncedLow);

endmodule