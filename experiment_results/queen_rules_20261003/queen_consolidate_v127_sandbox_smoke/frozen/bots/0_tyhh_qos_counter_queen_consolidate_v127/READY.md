# H5 Freeze

Main SHA-256:
`366943de3d798d10d3238f715a8abfd5ec794015dee643479cc32be0e3d9e6b2`.
Consolidation module SHA-256:
`bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`.
Queen-feed module is unchanged from parent:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
Helper is unchanged from queen_v127.

Eight focused checks passed after `g++-10 -std=c++20 -O2 -x c++`
compilation of verification/consolidate_tests.cpp.txt. Independent normal
main compilation passed. Test SHA-256:
`5de377877b7bbada6b7f8f2197d8db9544c2bec26b213251ca3bb7bbacdfa3e4`.

Tests cover equal recipient orientation, pickup path, queen exclusion,
population reserve, unknown pickup exclusion and temporary recipient hold.
This does not verify actual competitive collection or sandbox point limits.
Sources frozen before competitive schedule.
