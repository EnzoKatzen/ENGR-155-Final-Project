module inputFPGA_top(
    input  logic reset,
    input  logic [2:0] dial1,
    input  logic [2:0] dial2,
    input  logic [3:0] switches,
    input  logic [3:0] buttons,
    input  logic [3:0] wires,
    output logic ce,
    output logic sclk,
    output logic mosi
);
    logic enable = 1'b1;
    logic clk;
    //Using template from iCE40 Oscillator Usage Guide
	HSOSC 
	#(
		.CLKHF_DIV ("0b00") //Setting it to 48 MHz
	) OSCInst0 (
		.CLKHFPU(1'b1), 
		.CLKHFEN(1'b1), 
		.CLKHF(clk) //Setting clk to be the 48 MHz signal
	);

    //create 1ms counter
    logic [15:0] msCount;
	counter #(.WIDTH(16), .MAX(16'd47999)) msCounter(
		.clk(clk),
		.reset(reset),
		.enable(enable),
        .count(msCount)
	    );

    logic [17:0] inputs, syncedInputs, debouncedInputs, lastInput;
    assign inputs = {wires, switches, buttons, dial2, dial1}; //bit 17[wires[3:0], switches[3:0], buttons[3:0], dial2[2:0], dial1[2:0]] bit 0

    twoFlopSynchronizer inputSynchronizer(
		.clk(clk),
		.reset(reset),
		.inputs(inputs),
		.syncedInputs(syncedInputs)
	);

    edgeDebouncer #(.COUNTERMAX(16'd4799)) inputDebouncer( //debounce at 0.1ms, 1.1ms, etc
        .clk(clk),
        .reset(reset),
        .counter(msCount),
        .sold(lastInput),
        .sin(syncedInputs),
        .sout(debouncedInputs)
    );
    
    logic spiFlag, spiLoad;
    spiFlagger #(.COUNTERMAX(16'd47999)) inputFlagger( //check inputs at 1ms, 2ms, so does not overlap with debouncing.
        .clk(clk),
        .reset(reset),
        .counter(msCount),
        .sin(debouncedInputs),
        .sold(lastInput),
        .spiFlag(spiFlag),
        .spiLoad(spiLoad)
    );

    spiMaster spiConnection(
        .clk(clk),
        .reset(reset),
        .data(lastInput),
        .spiFlag(spiFlag),
        .spiLoad(spiLoad),
        .sclk(sclk),
        .ce(ce),
        .mosi(mosi)
    );
    
endmodule
