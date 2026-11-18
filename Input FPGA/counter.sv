//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 9/2/2026
//Purpose: Uses a inputted clk to blink
//		   an LED every 1/5e6 clk cycles 
//		   (adjustable using parameters).

module counter #(
	parameter WIDTH = 23,
	parameter MAX = 23'd5000000
)(
	input logic reset,
	input logic clk,
	input logic enable,
	output logic [WIDTH-1:0] count,
	output logic blink
);
		
	always_ff @(posedge clk, negedge reset)
	begin
		if (!reset) //the reset buttons on the board are high by default, so check when they become low
		begin
			count  <= 'b0;
			blink <= 1'b0;
		end 
		else if (count >= MAX) 
		begin			
			count  <= 'b0;
			blink <= ~blink; 
		end 
		else 
		begin
			if(enable) count  <= count + 1;
		end
	end
endmodule
