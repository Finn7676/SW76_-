# R26 First-Screen Parser Audit

Date: 2026-10-08. Scope: read-only audit of the frozen base82001 R26/H14 mechanism scanner and its completed outputs. No full scanner rerun, match launch, production edit, scanner edit, or result-output edit was performed. The only execution of the scanner was `--self-check-only`. This audit file was written after explicit authorization.

The source audit was delegated to a zero-context `gpt-6-astra` reviewer (`/root/r26_design_review/fresh_scanner_review`, high reasoning), supplied the source/data objects rather than prior reviews. The parent reviewer independently ran the existing self-checks, inspected the code, and recomputed output aggregates and provenance checks. This follows the paper-verification and cross-model-review separation of implementation and verdict.

## Verdict

No blocking defect was found in the R26 scanner's active execution path. The reviewed evidence supports committed-action accounting, reconstruction of necessary ordinary current-visible geometry, and descriptive downstream actor histories. It does not establish complete bot-input equivalence, independent source-level equivalence to the native WASM rules, global safety, cross-seed robustness, statistical significance, or win causality.

## Findings

1. An unused imported function retains an initial-position defect. `experiment_results/queen_rules_20261003/queen_immediate_witness.py:257` prepends every position event without a `round_index < 0` branch; its standalone `scan()` can duplicate initial heads. R26 imports only `ordinary_complete`, `project`, and `visible_cells`, never that `scan()`. Its active paths correctly check and skip repeated initial positions in `r26_firstscreen_mechanism.py:262` and `r21_firstscreen_mechanism.py:214`. This finding does not invalidate the R26 outputs and has not been fixed in this task.

2. Replay geometry is a necessary-admission check, not reconstruction of all source gates. `h14_terminal_body_probe.py:21` and `queen_immediate_witness.py:54` check unique cells, current visibility, and existence of ordinary predecessor adjacency. Actual production admission additionally uses segment-facing metadata, predecessor uniqueness, visible input membership, `LEN`, and private `ownSeg`/trail state (`headtrade.hpp:14` and `:90` in the frozen candidate). The scanner declares these omissions at `r26_firstscreen_mechanism.py:117` and `:457`. Raw score values, friend hard/soft SONAR protection, and the private donation flag are also not reconstructed. Preserve these qualifications; do not label the replay test as full native admission verification.

3. The 10 existing self-checks pass, but they are synthetic geometry/log controls rather than exhaustive parser or rule certification. They cover growth-sensitive shorter-head witnesses, a living continuation, log parsing, queen exclusions, portal rejection, partial-body rejection, added partial-head exposure, and split-action exclusion. They do not directly synthesize initial replay events, split-event orientation, or paid multi-step parser cases. Those paths were inspected against the source and corroborating viewer decoder, not certified by these 10 checks.

## Reconstruction Checks

- Initial repeated position events are verified against the initial map head/tail and skipped, avoiding body duplication in both active scanners.
- Split reconstruction explicitly checks `parent_body + reversed(child_body) == prebody`, as well as the active actor and requested child length (`r26_firstscreen_mechanism.py:287`; `r21_firstscreen_mechanism.py:231`).
- The projector fixes free steps from initial length, checks payment eligibility, tests collision before removing tails, applies pearl growth, and then paid shortening (`queen_immediate_witness.py:70`). This matches frozen candidate `main.cpp:786` for the reviewed ordinary paths. The complete output also records exact projected-versus-actual replacement bodies.
- A present-to-absent pearl event must be immediately followed by the same actor's landing update to count as a confirmed pickup (`r26_firstscreen_mechanism.py:273`). Pickup count and positive net length gain are separate, so paid movement does not manufacture growth.
- Every `SHORTER_HEAD_TRADE` log is bound to the next official action and checked for matching actor, round, and direction. Multiple or uncommitted logs fail assertions (`:210`, `:224`, `:248`, `:309`). These are committed replacements, not rejected proposals or all opportunities.
- Canonical prefixes include earlier actions, physical events, and SONAR events; the current differing action is excluded. Strict comparison also requires equal canonical event count, actor, round, body, and population (`r21_firstscreen_mechanism.py:76`, `:320`). Diagnostic LOG events are intentionally excluded.
- Own-body completeness omits the enemy tail-boundary proof because own length is known. Enemy completeness requires every possible ordinary tail extension to be current-visible, except kelp-blocked edges (`queen_immediate_witness.py:63`), matching the production distinction. Partial/equal/longer visible heads remain in the no-added-first-step-exposure comparison.

## Viewer Corroboration

The fresh reviewer read the installed official viewer archive at:

`/home/tyhh/SW76_-/.venv-battlecode/lib/python3.13/site-packages/unswbc/replay-viewer.vsix`

Archive member: `extension/dist/webview/webview.js`.

SHA256: `3d822ee45f4b570d6111bbd294897f196bb298ec9f538efdce75a8f67e22344d`.

The viewer decoder corroborates prepend-head/trim-to-tail position updates, split parent/child body fields, immediate pearl-removal/update interpretation, and displayed free steps `ceil(length/4)`. Its format-2 SONAR parser uses id offset 0, direction 4, union selector 6, hit id 12, `value64` 16, hit kind 24, and origin/end pointers. The omitted legacy uint32 value at offset 8 is selected only for formats below 2, outside the scanner's explicitly checked format-2 scope.

The viewer archive is supplementary local evidence and was not part of the supplied engine snapshot manifest. Native movement/payment/collision source was unavailable. Matching frozen candidate code and the viewer decoder must not be presented as independent source-level verification of the WASM engine.

## Output And Provenance Audit

Independently checked without replay parsing or a duplicate full scan:

- All 68 output pairing keys are unique and match the two official audit files: 17 maps per opponent/seat.
- All output outcomes, final standings, replay paths, and recorded replay hashes match the audited records.
- Rehashed all 136 replay files and 131 frozen source-tree files referenced by the two run manifests; every checked hash matched. Provenance run/audit hashes, scanner/dependency hashes, and the frozen native engine hash matched.
- Recomputed every `metrics` field and every opponent/seat/map summary from the detailed JSON records; all matched. Each game's committed-log count equals its detailed record count.

| Mechanism Evidence | Independently Recomputed |
| --- | ---: |
| Games | 68 |
| Identical action and physical/SONAR games | 22 |
| Committed replacement records | 705 |
| Games with committed replacements | 46 |
| Strict-prefix first action changes | 46 |
| Necessary-geometry-consistent strict first changes | 46 |
| Other first action divergences | 0 |
| Replacement necessary geometry / actual projection / logged zero witnesses / complete-head count matches | 705 each |
| H14/R26 first actor death before next own action | 45 / 0 |
| H14/R26 first actor same-or-next-round head death | 45 / 3 |
| H14/R26 first actor later-only confirmed pickups | 2 / 122 |
| H14/R26 first actor alive at end | 0 / 2 |

The 46 strict-prefix first changes independently compare actual H14 proposals and R26 actions under a matched prior observable trajectory. Original private proposals are not logged for later activations; the other 659 committed replacements remain descriptive divergent trajectories. The 45-to-0 before-next-action death difference is a local first-actor observation, not an estimate of all-game safety or win causality. Against xxqztest, own hostile head deaths across complete trajectories increase from 740 to 753, despite the first-actor observation. Keep both results visible.

Paired wins match the official audit: V3 15/34 to 16/34; xxqztest 16/34 to 17/34; seven win-status promotions and five regressions. This is one development base and cannot independently qualify robustness or justify a significance claim.

## Bound Artifacts

- `r26_firstscreen_mechanism.py`: `6c63c734d8670a031070ea4a0e8b0e8af7846dc05cbc83a6b54ae6db1237f27e`.
- `r26_firstscreen_mechanism.json`: `6800709b9c6d4c835679d89de5da943b5333c928e93b417a284073de10523010`.
- `r26_firstscreen_mechanism.md`: `b42a12cfa85063556b3d3d0434d68dc593981ce3c9206a66da179c5f08412359`.

Verification command executed: `python3 experiment_results/xxqz_robust_20261004/r26_firstscreen_mechanism.py --self-check-only`, returning `passed=true`, `assertions=10`, and `no_replays_or_outcomes_read=true`. Additional read-only structured JSON/hash checks were executed inline and produced no files. No policy or result artifact was changed.
