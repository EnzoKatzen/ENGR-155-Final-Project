//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 9/20/2026
//Purpose: Uses two flipflops to sync
//		   the cols input to prevent 
//		   encountering metastability.

module twoFlopSynchronizer #(
	parameter WIDTH = 18
)(
  input  logic clk,
  input  logic reset,
  input  logic [WIDTH-1:0] inputs,
  output logic [WIDTH-1:0] syncedInputs
);

  logic [WIDTH-1:0] n1;
  always_ff @(posedge clk, negedge reset) begin
      if (!reset) begin
        n1 <= '0;
        syncedInputs <= '0;
      end else begin
        n1 <= inputs;
        syncedInputs <= n1;
      end
  end
endmodule
