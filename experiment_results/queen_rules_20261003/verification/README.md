# Rule Verification

Run with `.venv-battlecode/bin/python experiment_results/queen_rules_20261003/verification/run_tests.py`.

`candidate_tests.cpp` directly includes the isolated `tyhh_qos_counter_queen_v127` source. It checks movement projection for lengths 4, 5, 8, 9 and 12, fixed free allowances after eating, paid-step prevalidation, collision before tail removal, empty-action suicide, both fixed queen IDs, one-shot scout bootstrapping, and portal landing/continuation/space guards. `candidate_tests.json` records the tested source hashes and compiler output.

`engine_fixtures.py` executes 27 deterministic microfixtures against the installed 1.2.7 WASM engine. Synthetic maps are created in memory and are not competitive or ranked games. Queen death/split cases verify fixed IDs and absence of succession through the engine result and replay standings. Sprint cases verify free allowances, extra-tail costs, overbudget death, food timing, and collision timing. Matches terminate after at most three rounds. `engine_fixtures.json` records fixture/parser/engine hashes and detailed observations.

These checks establish local rule semantics and selected candidate guards. They do not establish win-rate improvement, full-game queen scoring priority, sandbox resource compliance, or all strategy branches.
