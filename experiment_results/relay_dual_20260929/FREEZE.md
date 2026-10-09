# Final selection freeze — 2026-09-29

Written before launching or inspecting any final seed74001/74002/74003 game.
Selected candidate: `tyhh_test/tyhh_qos_relay_counter`. This is a new experimental
directory; original relay and both opponents remain unchanged.

## Selection, without a success claim

Both development seeds, official ten maps and swapped sides:

| Version | V3 W/L/D | xxqztest W/L/D | Weaker-opponent wins | Total wins |
|---|---:|---:|---:|---:|
| original relay |16/24/0|15/25/0|15|31|
| mature350 |19/20/1|18/22/0|18|37|
| counter |19/21/0|20/20/0|19|39|
| crown_mature |18/22/0|20/20/0|18|38|

Counter is selected under PLAN.md's weaker-opponent-first rule. Development
performance itself does NOT satisfy the double >50% objective. The other
first-seed candidates failed screening: gate5/6, mature3008/7, crown6/7,
counter_mature8/7, dual9/7, strike9/6, escape6/6 wins out of20 per opponent.
All failed runs remain archived. No escape/counter combination is pursued
because the final bounded repair produced no aggregate gain over baseline.

Development comprises600 complete games across11 versions including baseline.
The full table is reproducible with `python3 development_table.py`.

## Production freeze

Main.cpp SHA256:
- relay: c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961
- counter: ceaa0183760c064b25141cb8f7cbe2d73213050a760942db0768419572529873
- V3_portal: 3323482722c3c01e85d3fe7d7366e02a46ee1047ebd1d7c24bd07b109f246632
- xxqztest: 83f85b056b482d7488762bf73dcd07de09cd1f1485dc9f61e4e8b991f43a815c

Complete production manifest hashes, including every production header and
bot.toml (SHA256 of json.dumps(evaluate.old.production(path), sort_keys=True)):

| Source | Files | Manifest SHA256 |
|---|---:|---|
| relay |3|bc17533408b781019adf1cd2128e0e284dd0b482accb6272bb8178692c6b3ef2|
| counter |3|b6997ce15649203ad4254e622ef9510136afb468831c758acad232e0f0b1c7f3|
| V3_portal |15|b5ff885fdebd3bcc3a30b2c986d5e02dafecccca98ca3ac15ddfc2e53ad3e856|
| xxqztest |16|7846b40b8f734889fd014b5f361f4f719f1898835c546488c172f878dcb40822|

The source snapshots, binary paths, map hashes and actual CLI seeds are saved
by the runner and checked by evaluate.py. Any failure is reported separately;
in-game no-action death is never used alone to exclude a complete match.

## Final protocol

Baseline and selected candidate each run120 matches: two opponents, ten maps,
three seeds74001/74002/74003, both sides. Use new directories baseline_final
and counter_final. Maximum four match processes total. Both win rates must
exceed50% separately; report seed mean and sample SD descriptively without
p-values or claims of stable dominance. No production edits or further
selection based on these final outcomes. If the objective fails, report that
explicitly rather than returning to tuning on these held-out seeds.
