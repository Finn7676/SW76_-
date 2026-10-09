# R26 Read-only Incoming Head-Trade Diagnosis

Scope: 102 audited frozen H14 vs unchanged xxqztest native 1.2.7 games,
bases82001/83001/84001, 17maps A/B. No competitive launch or reserved-seed
access. Each source replay SHA256 and reconstructed final standings checked.
H14 main SHA256:
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.

| Observation | Count |
| --- | ---: |
| Nonqueen movement actions | 990836 |
| Complete ordinary current-view own movements | 895933 |
| Known living ordinary movements | 872062 |
| Complete enemy first-step head exposures | 4335 |
| Shorter-enemy exposures | 795 |
| Hostile nonqueen head deaths | 2312 |
| Actual complete-current-view first-step witness deaths | 1077 |
| Actual shorter-enemy witness deaths | 779 |
| Actual shorter-enemy deaths with lower-witness ordinary alternate | 581 |
| Those alternate events in losses | 239 |

The exact actual witness deaths cover81games. All1077 were separately
checked for exact actual attacker action, attacker body and victim posthead
agreement with the source first-step witness; no mismatches were found.
The parser skips initial repeated position events, checks split arrays,
and uses unique ordinary full body chains plus all possible tail-neighbor
visibility for complete bodies. Official raw-input fixtures independently
validate predecessor-facing chains for two actual loss witnesses.

Of581 shorter deaths with a lower-witness alternate,565 originals are
single steps and16are two steps. Own prelength distribution:LEN2 84,
LEN3 449,LEN4 37,LEN5 6,LEN6 2,LEN7 2,LEN8 1. Only140 ate a pearl,
including69inlosses. A pearl-onlyLEN>=5 design misses dominant exposure.
A postlength gap>=3 restriction leaves38actualdeaths,14alternate cases
and4alternate events inlosses; no such restriction is justified here.

A stricter subset excludes actual enemy queen attackers and requires an
alternate with zero witnesses from ANY complete enemy:556events in73games,
226inlosses. Of these,540original actions are single steps,220in32losses.
Their map coverage is15maps; by base original single-step counts are
178/199/168 before enemy-queen exclusion and at most those counts afterward.
See R26_NEXT_DESIGN.md for the exact registered policy and witnesses.

The broad count is geometric opportunity, not candidate activation.
It omits private branch state, protection hard/soft risks and newly exposed
partial enemy-head rejection. Shorter-mask reduction alone may add a
different enemy witness, so the final policy is stricter. Original two-step
movements can arise from contested-pearl sprints or freeHarvest and must
not all be called scorer decisions. Unknown body, portal and hidden/longer
attacks remain unknown. A first-step witness is possible legal action,
not proof of the next global actor or unchanged occupancy.

Reproduce with:
`.venv-battlecode/bin/python experiment_results/xxqz_robust_20261004/r26_survival_broad_probe.py`
The script refuses to overwrite its JSON; preserve or choose a new output
path for an independent rerun. ScriptSHA256:
`650a5b4c014dd21e14a675eb7d4d35314e73aabe5660c99a66cb53fc1778146f`.
JSONSHA256:
`0bedbf71b8e05471986065ab6997aae2e578cc5465dbd74ed4667910998733eb`.

Actual source-bound fixtures, fresh frozen H14 controls and official W/N
action arms passed for default82003A id2r20 and australia83001A id341r352.
Fresh R26 candidate also emitsN for both exact inputs; native actions match
the already evaluated official alternateN arms. This proves local rule
behavior under held-fixed recorded actor replies, not competitive wins.
Candidate-native reproduction:
`.venv-battlecode/bin/python experiment_results/xxqz_robust_20261004/r26_candidate_native_fixture.py --candidate /tmp/r26_headtrade_bot`

Evidence files: r26_survival_broad_probe.py/json, r26_headtrade_fixture.py,
r26_headtrade_actual_state.json, r26_candidate_native_fixture.py,
r26_candidate_actual_native.json and four r26_default/australia rule
replays. No production or opponent source was edited by this diagnosis.
