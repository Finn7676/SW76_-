# Frozen counter source: final regression check

2026-09-29, performed while the prespecified final match suites ran.
No production file was changed. Counter main.cpp remains
ceaa0183760c064b25141cb8f7cbe2d73213050a760942db0768419572529873.

Both commands compile C++20 with Clang, O1, debug information,
-fsanitize=address,undefined and -fno-omit-frame-pointer.
Execution uses ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1.

- verification/counter_tests.cpp.txt: PASS, exit0;
  stdout: counter proactive-trade boundary assertions passed.
- verification/relay_tests.cpp.txt: PASS, exit0;
  stdout: relay room/topology assertions passed.

The tests include production main.cpp under a renamed entry point. Each
compilation emitted the existing missing-return warning in that renamed,
unused function. LSan is disabled for sandbox compatibility. No sanitizer
finding or assertion failure occurred. Native competitive binaries are
separately compiled by run_suite.py from frozen source snapshots; these
sanitizer fixtures are not the binaries used in the matches.
