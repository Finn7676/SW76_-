H1 safeportal implementation complete and frozen for root scheduling.

Candidate: `tyhh_test/tyhh_qos_counter_safeportal`
Main SHA-256 at my freeze: `e1ad101e4d06d8eb7d81173b5b262ce450d1e14202b7509bdd88048bb2cfdcbf`.
Root subsequently added equivalent unresolved/fallback guards in the shared
candidate; current working-tree main SHA-256 is `3bc4c82b080574f7691325751c4fec874f09a200cb303ae82a1791889d399e86`
and it still compiles natively.
The focused ASan/UBSan boundary test was rerun on the current tree and passed
(binary SHA-256 `b6223b6dae1f96c9a5c0a87040b718090ec17c46bf652f702d88e102cb142a72`).
Native compile and focused ASan/UBSan safeportal boundary test passed.
No competitive matches run by this agent.
