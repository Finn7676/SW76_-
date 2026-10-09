# R12 Ready

Production g++-10 -std=c++20 -O2 build passed cleanly. Source-bound candidate
and H14 control fixtures both passed. No competitive matches were launched.

Focused positive cases: quiet short dragon leaves an adjacent verified pearl
for an 8-segment nonqueen or a visible scoring queen while retaining a proven
living continuation; accepted current visible status can prove receiver length;
scored freeHarvest cannot reclaim the reserved pearl through a two-step detour.

Negative controls preserve H14 pickup when no receiver exists, receiver is
too short or unseen, the report is missing for a partial receiver, round is
early, population is low, quiet history is insufficient, a current enemy part
is present, the receiver route is blocked/unknown/unresolved portal, or the
pearl is the only non-neck escape. Unscored prior actions retain freeHarvest.
Routine split, adjacent enemy-queen attack and live emergency movement were
also verified unchanged. Parent/feed/consolidation/sprint/portal code is not
modified. A proven pickup route is not a commitment by the receiver to use it.

Commands from this directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_pearl_reserve_bot
g++-10 -std=c++20 -O2 -x c++ verification/pearl_reserve_tests.cpp.txt -o /tmp/robust_pearl_reserve_tests
/tmp/robust_pearl_reserve_tests
g++-10 -std=c++20 -O2 -DPEARL_RESERVE=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/pearl_reserve_tests.cpp.txt -o /tmp/robust_pearl_reserve_control
/tmp/robust_pearl_reserve_control
```

Included-main fixtures have the inherited renamed-main no-return warning in
both candidate and control; the production build does not.

Main SHA-256: `2a49f95382df699038ecb267c8694b7beff69febb5d0fae4570766e0beedeb82`.
Header SHA-256: `3fa9d132e302afe0b89096c2effe46a00b86b0628c4a6b318dce64094bb84f01`.
Focused source SHA-256: `88beb56aac8e2a1cdec58cf53235482736c79e670978e5f9093ec87186e9bdd2`.
H14 parent main remains `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
Helper/queen_feed/consolidate/bot.toml remain byte-identical to the parent.
Opponent main/planner hashes were rechecked unchanged after implementation.
