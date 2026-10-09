# R26 Three-Seed Independent Result and Provenance Audit

Audit date: 2026-10-09, Asia/Singapore. This audit received no prior review
verdict. Its conclusion is derived from the six registered development suites,
their raw CSVs, finished official replay audits, frozen manifests, live source
files, `PLAN.md`, and `r26_three_seed.json/md`.

## Verdict

No result-integrity or aggregation discrepancy was found within the inspected
scope. R26 has a verified development gain of two xxqztest wins across 102
matched games, accompanied by three fewer V3 wins. R26 does not satisfy the
unchanged robustness requirements: xx batch wins are 17/34, 21/34, and 16/34;
pooled xx wins are 54/102 (52.94%); pooled xx B-seat wins are 23/51 (45.10%).
The strict per-batch, pooled, and B-seat thresholds fail. The V3 regression
threshold passes. This is a development result, not held-out confirmation.

## Coverage and Pairing

Inspected labels are `robust_h14_82001/83001/84001` and
`robust_headtrade_82001/83001/84001` under
`experiment_results/queen_rules_20261003`.

Each suite has 68 CSV rows, 68 unique game IDs, 68 unique
`(opponent, map, seed, bot1_side)` keys, and exactly matching registered
schedule and official-audit keys. Equivalently, each suite has 34 A/B pairs:
17 maps for each of two opponents, with 34 games per opponent and 17 games
per opponent-seat slice. Each version has 204 games; the report covers 408.
All candidate keys have identical H14 opponent/map/seed/seat counterparts.

The ranked map order is australia, autarky, default, devil, portals, dilemma,
queen_of_spades, schooltime, slithery_fight, trauma, trophy, islands, maze,
stripes, tower_defense, unsw, weakhold. Seeds equal the registered base plus
the zero-based position in that order: 82001--82017, 83001--83017, and
84001--84017. Both seats and both opponents share each map seed. Candidate
is bot A for A-seat games and bot B for B-seat games; official audit bot
ordering agrees with the frozen prepared paths. No confirmation-seed
directory or result for 91001, 92001, or 93001 was opened by this audit.

## Source and Runtime Provenance

All six manifests declare unswbc 1.2.7 and `sandbox=false`. Every frozen
engine/module tree matches its manifest; all corresponding installed module
files and the current CLI wrapper match their recorded hashes. All six
engine/module manifests are equal, as are the 17 ordered map hashes.
Every frozen map and live original map matches its recorded digest.

| Item | SHA256 |
|---|---|
| Engine WASM | `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546` |
| CLI wrapper | `46757bce2ea7df2457f6d3deb29c989e1fa286776e90fed5bdec42e23de5e448` |
| R26 main.cpp | `660363800c8cce16b1717d0f9dbbdf9feba6e02083380ccb1b3e1a5e92cc06d4` |
| R26 headtrade.hpp | `69bf74738cbf99f655eb9b715eec10f41b8946d184808b41fac28417c04ce4da` |
| R26 native binary | `72a799b31a4fd26a67eebea5fa90fa1de8f692ae996d10d68a89e50e1223e1fa` |
| H14 main.cpp | `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc` |
| H14 native binary | `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f` |
| V3 native binary | `149f787af8b6ee01eeba1f37abaa466f0d179e083c2b4a2de6618519ab2c1b2c` |
| xxqztest native binary | `7b4de70054dcb5bbba49c087370699506ec4f6d40b0e126d80d3388fb3adca81` |

All three R26 batches have identical executable source/configuration hashes
and identical prepared binary hashes. Their main and module equal the live
`tyhh_test/tyhh_qos_robust_headtrade_v127` production files. All frozen bot
trees, including documentation and verification artifacts, match their own
recorded full manifests. The only R26 full-tree difference between the first
screen and expansion snapshots is `RESULTS.md`; executable code is unchanged.

The frozen H14/R26 main diff contains only the `headtrade.hpp` include and
final `reviewShorterHeadTrade(ct)` call. `helper.hpp`, `queen_feed.hpp`,
`consolidate.hpp`, and `bot.toml` are byte-identical to H14 in all batches.
Both opponents' complete frozen trees and binaries are equal across all six
suites; live opponent code matches the recorded source hashes. Frozen
`run_suite.py` and `benchmark.py` snapshots equal their current live files.
Official audit tool and packed-parser hashes match all six audit summaries
and the current local files.

## Official Audit and Failure Checks

All six official audits report `audit_passed=true`, 68/68 games, and empty
`audit_failures`, `manifest_failures`, and `failed_matches`. All six raw CSVs
contain only wins/losses and empty error fields. There are no draws.

This independent pass checked the recorded raw log and replay hashes for all
408 games, with no mismatch. CSV and official-audit game fields, completed
rounds, and replay digests agree for every match. Log inspection found no bot
exit, timeout, or points/memory-limit evidence. The official audits account
for every invalid-action death as an explicit SUICIDE on both teams;
unexplained invalid-action deaths are zero. SUICIDE counts are not crashes.

The official parser reconstructs final bodies and checks standings, seeds,
map contents, winner ordering, and bot ordering. This audit inspected its
finished output and checked provenance; it did not rerun the official audit
or independently reparse replay events. That boundary remains explicit.

## Recomputed Results

| Version | Base | V3 W/L/D | xx W/L/D | xx A Wins | xx B Wins |
|---|---:|---:|---:|---:|---:|
| H14 | 82001 | 15/19/0 | 16/18/0 | 10/17 | 6/17 |
| R26 | 82001 | 16/18/0 | 17/17/0 | 10/17 | 7/17 |
| H14 | 83001 | 19/15/0 | 20/14/0 | 11/17 | 9/17 |
| R26 | 83001 | 15/19/0 | 21/13/0 | 11/17 | 10/17 |
| H14 | 84001 | 19/15/0 | 16/18/0 | 9/17 | 7/17 |
| R26 | 84001 | 19/15/0 | 16/18/0 | 10/17 | 6/17 |

Sample standard deviation uses the three batch win fractions and denominator
n-1. The pooled fraction equals the batch mean because all denominators are
34. Spread is reported in percentage points, not as a confidence interval.

| Version / Opponent | Pooled W/L/D | Mean +/- Sample Std | Batch Range | A Wins | B Wins |
|---|---:|---:|---:|---:|---:|
| H14 / V3 | 53/49/0 | 51.96% +/- 6.79 pp | 44.12--55.88% | 25/51 (49.02%) | 28/51 (54.90%) |
| R26 / V3 | 50/52/0 | 49.02% +/- 6.12 pp | 44.12--55.88% | 24/51 (47.06%) | 26/51 (50.98%) |
| H14 / xx | 52/50/0 | 50.98% +/- 6.79 pp | 47.06--58.82% | 30/51 (58.82%) | 22/51 (43.14%) |
| R26 / xx | 54/48/0 | 52.94% +/- 7.78 pp | 47.06--61.76% | 31/51 (60.78%) | 23/51 (45.10%) |

Paired xx changes are 8 gained wins and 6 lost wins (+2/102, +1.96 pp).
Paired V3 changes are 4 gained wins and 7 lost wins (-3/102, -2.94 pp).
The xx batch gains relative to H14 are +1, +1, and 0 wins; the V3 differences
are +1, -4, and 0. Equal 84001 win counts still contain outcome changes:
xx gained/lost is 2/2 and V3 gained/lost is 1/1.

The legacy10 subset remains adverse: xx 34/60 -> 32/60, V3 28/60 -> 26/60,
combined 62/120 -> 58/120. On xx, the pooled map improvements are australia
1/6 -> 4/6 and islands 0/6 -> 1/6; losses are autarky 5/6 -> 4/6 and
slithery_fight 3/6 -> 2/6. Persistent xx weaknesses include default 1/6,
portals 1/6, queen_of_spades 0/6, and stripes 0/6.

For descriptive terminal standings, candidate-side values are:

| Version / Opponent | Queen Alive | Mean Queen | Mean Longest | Mean Total |
|---|---:|---:|---:|---:|
| H14 / V3 | 21/102 | 2.098 | 12.500 | 73.343 |
| R26 / V3 | 18/102 | 1.520 | 13.265 | 72.078 |
| H14 / xx | 14/102 | 1.431 | 15.775 | 93.206 |
| R26 / xx | 16/102 | 1.922 | 16.314 | 94.951 |

These standings do not isolate the causal contribution of local head-trade
avoidance and must not be presented as a global safety guarantee.

## Fixed Qualification Gates

`PLAN.md` requires strict >50% xx in each of three held-out batches,
pooled >55%, both pooled seats individually >50%, and no pooled V3
regression exceeding five percentage points against matched H14.

| Requirement | R26 Development Observation | Assessment |
|---|---|---|
| Each batch >50% xx | 50.00%, 61.76%, 47.06%; minimum 18/34 needed | Fails analogous development gate on 82001 and 84001 |
| Pooled xx >55% | 54/102 = 52.94%; minimum 57/102 needed | Fails |
| Both xx seats >50% | A 31/51 = 60.78%; B 23/51 = 45.10%; minimum 26/51 each | A passes, B fails |
| V3 regression <=5 pp | 50/102 versus 53/102; -2.94 pp | Passes |
| Held-out confirmation | No reserved confirmation results inspected | Not performed |

The small positive xx development difference supports reporting a verified
gain with remaining weaknesses. It does not support robust superiority,
leaderboard generalization, a significance claim, or successful qualification.
No significance test was run. The three reused development bases are not
held-out evidence; native runs do not enforce judge points/memory or bot
random seeding, as stated in each registered manifest.

## Report Consistency and Evidence Fingerprints

Every JSON group label, seed, code hash, game count, old10 count, opponent
count, batch rate, mean, sample standard deviation, range, seat count, map
count, and paired gain/loss entry matches independent arithmetic from the
finished audit records. The Markdown table and map breakdown agree with
the JSON, including its one-decimal rounding. No source, recorded result,
existing official audit, or aggregate report was changed by this audit.

| Artifact | SHA256 |
|---|---|
| r26_three_seed.json | `2c4ca8d18ad48011924e98468498fa91b34ad0b12921a63912a49a92057f7594` |
| r26_three_seed.md | `35f63c52084486eedba922ea8337e6f4b5def88239a92c94d9c4ad074e7b7ca0` |
| analyze.py | `12518d888b85a2eaf71263b048d1820ff51849fad91b013a320554e1284781cf` |
| summarize.py | `443001eda6fd093c4b648d97bc1eafa8e1a2cee04696925c1a3ccbb449ac455f` |
