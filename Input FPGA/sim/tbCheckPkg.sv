//Author: Enzo Katzen - ekatzen@hmc.edu
//Date: 11/17/2026
//Purpose: Pass/fail bookkeeping shared by the Input FPGA testbenches.

package tbCheckPkg;
    int failures = 0;

    function automatic void check(input bit passed, input string description);
        if (passed) $display("[%0t] pass: %s", $realtime, description);
        else begin
            failures++;
            $display("[%0t] FAIL: %s", $realtime, description);
        end
    endfunction

    function automatic void reportResults(input string testbenchName, input int timingErrors);
        check(timingErrors == 0, $sformatf("no SPI timing errors (%0d seen)", timingErrors));
        if (failures == 0) $display("==== %s: ALL TESTS PASSED ====", testbenchName);
        else               $display("==== %s: %0d FAILURE(S) ====", testbenchName, failures);
    endfunction
endpackage
