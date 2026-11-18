//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 9/20/2026
//Purpose: Debounces inputs by confirming
//that they have stayed the same for
//the last 10 ms.

module debouncer #(
	parameter N = 10,
	parameter WIDTH = 18,
	parameter COUNTERWIDTH = 16,
	parameter COUNTERMAX = 16'd47999
)(
	input  logic clk,
	input  logic reset, 
	input  logic [COUNTERWIDTH-1:0] counter,
	input  logic [WIDTH-1:0] sin,
	output logic [WIDTH-1:0] sout
);
	logic [(WIDTH*N)-1:0] q;
	always_ff @ (posedge clk, negedge reset)
		if (!reset) q <= '0;
		else if(counter == COUNTERMAX) q <= {q[(WIDTH*N)-1-WIDTH:0], sin};
		
	always_comb begin
    sout = '1;
    for (int i = 0; i < N; i++)
      sout &= q[i*WIDTH +: WIDTH];
    end
endmodule
