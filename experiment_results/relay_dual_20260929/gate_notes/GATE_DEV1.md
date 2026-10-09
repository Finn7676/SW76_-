# Gate development seed 73001: four-game mechanism check

Date: 2026-09-29. Read-only reconstruction; no new matches. Each comparison is
against V3_portal, with the candidate on side B. The gate source remains frozen
at SHA256 `7fa190fd8354140b878540c027b4b528cb2810a0299308df5f3cbba4d4886d9e`.

## Retained raw paths

All paths below are relative to `experiment_results/relay_dual_20260929/`.

- `baseline_dev/0004_V3_portal_default_seed73001_B.replay`
- `gate_dev/0004_V3_portal_default_seed73001_B.replay`
- `baseline_dev/0008_V3_portal_portals_seed73001_B.replay`
- `gate_dev/0008_V3_portal_portals_seed73001_B.replay`

Each directory's `results.csv` supplies opponent, seed, side, outcome and replay
identity. Both Default games are losses: baseline ends by elimination at round
428, gate by length at round 500. Both Portals games are length losses at 500.
The unequal Default terminal horizons must not be treated as a matched final
cumulative-resource comparison. The checkpoints below use equal completed rounds.

## Matched checkpoint values: baseline → gate, candidate side only

| Map | Checkpoint | Positive body growth | Splits | Death-loss length | Total live length | Population | Successful move steps |
|---|---:|---:|---:|---:|---:|---:|---:|
| Default | 50 | 18 → 22 | 11 → 11 | 19 → 24 | 15 → 14 | 7 → 5 | 270 → 243 |
| Default | 100 | 31 → 33 | 15 → 16 | 35 → 33 | 12 → 16 | 5 → 6 | 598 → 567 |
| Default | 200 | 52 → 81 | 25 → 38 | 45 → 82 | 23 → 15 | 10 → 7 | 1319 → 1253 |
| Portals | 50 | 45 → 51 | 17 → 18 | 36 → 40 | 18 → 20 | 8 → 8 | 218 → 259 |
| Portals | 100 | 151 → 162 | 57 → 56 | 132 → 136 | 28 → 35 | 12 → 12 | 654 → 696 |
| Portals | 200 | 415 → 443 | 159 → 156 | 343 → 393 | 81 → 59 | 32 → 23 | 2920 → 2260 |

Positive body growth is not gross pearl intake: movement cost can offset growth
within a movement event. The diagnostic skips pre-round-0 position notices,
checks split and total-length conservation, and verifies final standings.

## Successful portal crossings before round 80

| Map | First crossing round | Crossings | Same-dragon/pair repeats | Repeats within 8 rounds | Within 8 rounds with no positive growth |
|---|---:|---:|---:|---:|---:|
| Default | 25 → 25 | 8 → 24 | 2 → 1 | 1 → 0 | 1 → 0 |
| Portals | 27 → 27 | 29 → 32 | 7 → 6 | 7 → 6 | 1 → 0 |

Default's first crossing is dragon 5, portal 0, `(24,8) → (4,3)`, length 2
in both versions. A child dragon 25 returns through portal 0 at round 29 in
both versions. These are actual successful movement events, not portal-pair
information. They establish entry and a later return but do not establish the
cause or safety of the intervening split. A repeat is a descriptive proxy, not
proof that a crossing was unproductive.

## Supported conclusion and stopping boundary

The checked gate games do not show reduced cumulative resource growth at rounds
100 or 200. More portal activity and fewer zero-growth rapid repeats did not
produce higher live length at round 200: death-loss length increased. Default
does not show a lower cumulative split count; Portals has nearly the same split
count. These observations do not support blaming the regression solely on
delayed expansion, or expecting an additional repeat filter to solve it.

No per-split, time-ordered evidence of a child/parent lacking a legal room exit
has been established. The aggregate losses do not identify room congestion as
their cause. Therefore no `tyhh_qos_relay_room` candidate is created on this
evidence, and the failed gate module is not recommended for combination. A
room-capacity intervention would require that missing event-level evidence.

## Exact read-only reproduction command

Run from the repository root. It prints the four checkpoint records and early
portal counts; it does not write results or run matches.

```sh
python3 - <<'PY'
from pathlib import Path
import sys, csv
sys.path.insert(0, 'experiment_results/protect_vs_v3_20260928')
from diagnose_economy import analyze
from diagnose_portal_repeats import analyze as portals
for run in ['baseline_dev', 'gate_dev']:
    p = Path('experiment_results/relay_dual_20260929') / run
    for row in csv.DictReader((p / 'results.csv').open()):
        if (row['opponent'] != 'V3_portal' or
            row['map'] not in ['default', 'portals'] or
            row['bot1_side'] != 'B'):
            continue
        a = analyze(p / row['replay'], 'B', {50, 100, 200})
        print('GAME', run, row['map'])
        for x in a['checkpoints']:
            print(x['owner'], x['checkpoint'], {k: x[k] for k in
                ['population', 'total_length', 'growth', 'splits', 'steps',
                 'death_length', 'max_length']})
        q = portals(p / row['replay'], row, run)
        print('early_portals', {k: q[k] for k in
            ['crossings', 'repeats', 'rapid8', 'rapid8_zero_growth']},
            'first', q['all_crossings'][:2])
PY
```
