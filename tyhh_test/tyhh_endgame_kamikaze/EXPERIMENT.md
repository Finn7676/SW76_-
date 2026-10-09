# Endgame kamikaze trial

## Hypothesis

After the late-game switch, sacrificing a short dragon without local enemy
evidence should transfer its death pearls to a visible friendly long dragon.
When an enemy head is reachable immediately or by a two-step sprint, a short
dragon can instead trade both heads, preserving the friendly long dragon as
the team's scoring candidate. The mechanism must not change the `tyhh`
baseline source.

## Fixed policy

| Parameter | Value |
|---|---:|
| Start round | 400 |
| Maximum donor length | 8 |
| Required visible friendly lead | 1 segment |
| Last surviving dragon may self-sacrifice | No |
| Two-step sprint through portal | No |

The observed segment count is treated as a lower bound. No enemy segment in
the current 7x7 vision is required for the feed branch; sonar history is not
used as proof that the local view is globally safe.

## Smoke checks

Commands used `clang++` through a temporary `c++` toolchain shim because the
system GCC 9.4 does not accept `-std=c++20`.

| Match | Seed | Outcome | Mechanism signal |
|---|---:|---|---|
| `tyhh_endgame_kamikaze` vs `xxqz_cpp`, `arena` | random | Team A, round 18 elimination | build/run succeeds; game ends before late phase |
| `tyhh` vs `xxqz_cpp`, `big_empty` | 1337 | Team A, round 500 by length | baseline control; no new no-action wave |
| `tyhh_endgame_kamikaze` vs `xxqz_cpp`, `big_empty` | 1337 | Team A, round 500 by length | repeated `no valid action` deaths begin at round 400, confirming donation branch execution |
| `tyhh_endgame_kamikaze` vs `tyhh`, `big_empty` | 1337 | Team A, round 500 by length | direct variant-vs-baseline smoke match completes |

An additional matched-side benchmark used two maps and one game per side
(`arena`, `default_small`; four games total) against the unchanged `tyhh`
directory. The result was 2-2 (50.0%) for the trial, with both wins on Team A
and both losses on Team B. This small run shows no reliable performance gain;
the replay and CSV are in
`replays/20260927-092945-n8q_s66w/`.

These are implementation checks only. They do not establish a win-rate or
length advantage; the next comparison should use matched seeds, both sides,
and at least three seeds per map before drawing a research conclusion.

## Full official-scene comparison

The complete bundled scene set was run with `benchmark.py` on 15 maps, two
games per map, with the trial and baseline swapping Team A/B. This produced
30 completed games with no runtime failures:

| Metric | Result |
|---|---:|
| Trial wins | 15 / 30 (50.0%) |
| Baseline wins | 15 / 30 (50.0%) |
| Trial Team A | 10 / 15 (66.7%) |
| Trial Team B | 5 / 15 (33.3%) |
| Draws | 0 |

Per-map trial win rates were: `Colosseum` 0%, `arena` 50%, `autarky` 50%,
`big_empty` 0%, `default` 50%, `default_small` 50%, `devil` 100%,
`dilemma` 50%, `portals` 50%, `queen_of_spades` 100%, `schooltime` 50%,
`slithery_fight` 50%, `stronghold` 100%, `trauma` 50%, and `trophy` 0%.

The run generated 853 variant-side `no valid action` deaths, concentrated on
`big_empty`, `slithery_fight`, `autarky`, `portals`, `stronghold`, and
`trauma`; the baseline had none. This confirms the donation branch is active,
but the mechanism has no aggregate win-rate gain in this 30-game comparison
and introduces a substantial additional death mode.

Artifacts: `replays/20260927-151956-gbq27s0t/results.csv` and the adjacent
replay/log files.

## Colosseum early-death diagnosis

The new branch cannot run before round 400 (`ROUND < KAMIKAZE_START_ROUND`
returns immediately), so the Colosseum deaths at rounds 13--84 are baseline
behavior. The two benchmark games used different random seeds. Replaying with
fixed seeds showed deterministic, seed-dependent outcomes:

| Seed | Bot order | Winner | End |
|---|---|---|---:|
| `0xcc5431204578c298` | trial A, baseline B | B | 34 rounds |
| `0xcc5431204578c298` | baseline A, trial B | B | 34 rounds |
| `0x35570f39a9acb213` | trial A, baseline B | A | 84 rounds |
| `0x35570f39a9acb213` | baseline A, trial B | A | 84 rounds |

The same seed gives the same winner after swapping bot order. The apparent
side advantage in the 30-game benchmark is therefore confounded by the
benchmark's random per-game seeds: `benchmark.py` does not pass `--seed` to
`unswbc run`. A fixed-seed, multi-seed comparison is required for causal
claims.
