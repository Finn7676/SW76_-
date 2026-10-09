# R19 Ready

R10-child interaction candidate. Production g++-10 C++20 O2 compilation,
source-bound candidate fixture and exact frozen H14 control passed. Relative to
R10, the only source change is the R17 ordinary-growth gate:
`ROUND>=350 && POP>=8 && no current visible enemy part` suppresses ordinary
expansion. Queen-hunt target value, split/attack order, donation, portal,
protection and emergency branches remain inherited from R10.

The fixture covers A/B, round boundaries, population refill, current enemy
part/head, sonar-only sighting, body-trap fallback and queen bootstrap. The
included-main test has the inherited renamed-main warning; production build is
clean. This is an interaction screen, not an additive-gain claim.

Commands from this directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/queen_mature_bot
g++-10 -std=c++20 -O2 -DLOCAL_MATURE=1 -x c++ verification/local_mature_tests.cpp.txt -o /tmp/queen_mature_tests
/tmp/queen_mature_tests
g++-10 -std=c++20 -O2 -DLOCAL_MATURE=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/local_mature_tests.cpp.txt -o /tmp/queen_mature_control
/tmp/queen_mature_control
```

| Artifact | SHA-256 |
|---|---|
| main.cpp | 264ae06134fab1959a108b2792d5e9e74af35e452f3bcbafaafe798b807e747c |
| source-bound tests | 068e3780dfe6aa2868e7f14ca6f939406d6c6ee124d77ebd9b0fdf995b50c29e |
| R10 parent main.cpp | de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba |
| frozen H14 main.cpp | ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc |

The remaining production files are byte-identical to R10/H14 as applicable;
no opponents, maps, engine, reserved seeds or H14 source were modified.
