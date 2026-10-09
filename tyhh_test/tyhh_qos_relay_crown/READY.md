# Crown H2 — implementation frozen

2026-09-29. Base: unchanged `tyhh_qos_protect_relay`. Production source
is frozen for `crown_dev`; do not tune this directory using its results.
The concrete hypothesis was registered in EXPERIMENT.md before matches.

## Source manifest

| File | SHA256 |
|---|---|
| main.cpp | 4df511688cdbaada34e710e46b4f039114f4daa810e73bbd789847b8ccba431e |
| crown.hpp | fa3308146738dfb1d0f50af27d1ae135e18ded27631b0f09331c31f0f790b67d |
| helper.hpp | 6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc |
| bot.toml | d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5 |

## Executed checks

- Native C++20 O2 build passed; binary `/tmp/tyhh-qos-relay-crown`, SHA256
  `c28610516e1e6d2ce8fbbe5329edce97bf43c8e5959c12c174bb31d7158930ca`.
- `verification/crown_tests.cpp.txt`: ASan/UBSan passed; output
  `crown donation/collection/protection checks passed`.
- Inherited `verification/relay_tests.cpp.txt`: ASan/UBSan passed; output
  `relay room/topology assertions passed`.
- `verification/death_engine.py`: six deterministic EngineModule fixtures
  passed. No-action deaths of length 2/3/4/5/6 and a length-6 self-collision
  leave pearls at head-index 0, 2, 4; the receiver collects the head pearl
  in the same round and grows from eight to nine. Engine SHA256
  `12e58ad88650da698a4b70c25f4d2c1b0341898787eca60fa95d219fc91ab566`.
- LSan disabled for sandbox; inherited unused `enemyEscapes` warning
  remains. The included-main test also warns about the renamed main's
  implicit return. No zero-warning claim is made.
- First crown test attempt failed because its fixture changed MYID without
  changing the represented donor body's ID. The fixture was corrected;
  production policy was not changed in response to that assertion.

Commands: `clang++ -std=c++20 -O2 -Wall -Wextra main.cpp -o /tmp/tyhh-qos-relay-crown`;
tests use `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
-Wall -Wextra -x c++` and `ASAN_OPTIONS=detect_leaks=0`. Run from this
directory, substituting each verification source. Engine fixture uses
`/home/tyhh/miniconda3/bin/python3 verification/death_engine.py`.

## Handoff and scope

`CROWN_DONATE` marks intentional no-action death; `CROWN_APPROACH` and
`CROWN_COLLECT` mark rendezvous and observed friendly death-pearl recovery.
Code 4 is not automatically a process failure. Donation proves only a
locally visible short path to the head pearl and an onward exit, not that
all donor pearls will be recovered or that every subsequent move is safe.

The root agent owns competitive runs and verdicts. Current source is not
modified after development results. Merge boundary: `crown.hpp`, its one
observe call, endgame action/expansion gates, and explicit no-action
preservation in projection/emission/protection. Portal logic is unchanged.
