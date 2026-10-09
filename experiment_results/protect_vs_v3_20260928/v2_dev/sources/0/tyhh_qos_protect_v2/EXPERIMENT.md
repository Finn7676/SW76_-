# protect_v2: Scout + relay module combination

This directory is a frozen composition of the independently checked
`tyhh_qos_protect_scout` and `tyhh_qos_protect_relay` variants. The source
under both original directories remains unchanged.

The Scout module is unchanged: length 2–8 dragons with team population at least six
may follow a known route to an unexplored portal for six steps (round 220 on
compact maps, round 300 on maps with at least 1200 cells). A safe visible
pearl can interrupt the task; routine splitting is deferred. Local portal
crossings and body orientation are the only explored evidence.

The relay module is unchanged: complete, checksummed portal endpoint pairs are
sent on a rotating protocol-3 beam; received pairs are validated against
existing topology. The proven single-portal resource-room claim is sent as a
separate legacy-compatible payload, while ordinary pearl/enemy/frontier
messages and post-action teammate status retain their independent beams.

No cross-dragon explored-state bit was added. A relay pair communicates
topology only, so each dragon's Scout target set can remain locally stale until
it observes a crossing or body evidence itself. This is an intentional
interface limitation of this two-module composition and a potential follow-up
experiment, not a hidden variable in this version.

No movement score, Scout threshold, relay threshold, protection rule, or
communication constant was retuned. The only behavioral change relative to
`protect_scout` is the relay module. Focused Scout and relay tests are copied
into `verification/`; both compiled and passed with ASan/UBSan.
