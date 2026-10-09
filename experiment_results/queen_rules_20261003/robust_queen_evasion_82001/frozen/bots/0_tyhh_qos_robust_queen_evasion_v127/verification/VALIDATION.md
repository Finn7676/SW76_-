# R6 Focused Validation

Native C++20 main build passed with g++-10 and O2. The same included-main
fixture was compiled for R6 and unchanged H14 control. Renaming the native
main emits the inherited no-return warning in the test builds; the normal
main build has no warning. Both fixture executables completed successfully.

Coverage:

- H14 accepts the living east proposal when a visible enemy is nearer east.
  R6 replaces it with a surviving continuation whose certified static
  ordinary-edge distance is strictly greater.
- Projected own neck and visible static enemy body each block direct BFS
  approach paths; original own occupancy is not reused as stale blocking.
- An unknown edge near the enemy invalidates risk certification and leaves
  the original proposal unchanged. A hidden frontier invalidates the
  putative safer endpoint rather than earning an unreachable bonus.
- No enemy-head and nonqueen turns preserve the proposal. Team B uses ID1
  as its fixed queen and applies the same queen-only review.
- projectAction accepts a length3 two-step move ending length2 but rejects
  a length2 paid second step. Three pearls on a length5 three-step path
  end length7, verifying the initial two free steps remain fixed.
- A replacement that blocks a protected friend's only exit is rejected.
  An unseen portal landing is rejected by unchanged H14 queenActionSafe;
  R6 search uses only ordinary open edges.

Exact assertion success does not establish competitive benefit or judge
resource compliance. No competitive or sandbox match was launched here.

Commands:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_queen_evasion_v127/main.cpp -o /tmp/queen_evasion_bot_v127
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_queen_evasion_v127/verification/evasion_tests.cpp.txt -o /tmp/queen_evasion_tests_v127
/tmp/queen_evasion_tests_v127
g++-10 -std=c++20 -O2 -DH14_CONTROL -x c++ tyhh_test/tyhh_qos_robust_queen_evasion_v127/verification/evasion_tests.cpp.txt -o /tmp/queen_evasion_control_v127
/tmp/queen_evasion_control_v127
```
