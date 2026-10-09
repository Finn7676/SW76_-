# READY — gate candidate frozen for root's paired matches

Date: 2026-09-29. No full matches have been run by this agent.

- Candidate: `tyhh_test/tyhh_qos_relay_gate`
- Frozen main.cpp SHA256: `7fa190fd8354140b878540c027b4b528cb2810a0299308df5f3cbba4d4886d9e`
- Parent main.cpp remains `c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961`.
- Native match build: `Project.from_dir(...).compile()` with `/tmp/clang-toolchain` on PATH, exit 0; binary in `.unswbc-build/bot`.
- Targeted gate assertions: compiled C++20 with AddressSanitizer and UndefinedBehaviorSanitizer, then passed.
- Inherited relay room/topology assertions: same sanitizer build, passed.
- Both successful sanitizer runs use `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1`; leak checking is excluded because this sandbox's ptrace causes LeakSanitizer to fail independently after assertions.

The targeted test covers a known pair without actual traversal, repeated-pair
suppression, a 25-cell resource room without changing the 24-cell claim, local
harvest instead of a portal return, unknown-boundary non-certification, no
unknown-space approach, a terminating unresolved exit, another occupant's claim,
both portal exit faces blocked by a longer friend, length/population/round/enemy
bounds, preserved harvesting split, postponed acquisition split, and restored
ordinary splitting after round 220.

One initial targeted assertion expected no portal route when only one landing
face was blocked; it failed because the other face is a legal different entry.
The test was corrected to block both destination faces. No strategy code was
changed to make that test pass. The initial inherited run passed assertions but
its default LeakSanitizer shutdown failed under ptrace; the successful rerun
used the explicitly documented leak-check exclusion above.

Compiler warnings are inherited/test-harness related: renaming the included
bot main makes its implicit main return warn, and disabled feature code leaves
enemyEscapes unused. Assertions did not invoke the renamed bot main.

No claim of competitive improvement is made. Root owns the 73001/73002
development games and frozen holdout protocol. Do not mutate this candidate
while its snapshot is being evaluated.
