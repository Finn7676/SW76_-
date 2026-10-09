"""Single-game read-only reconstruction using the existing verified replay reader."""
import hashlib
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "experiment_results/protect_local_20260927"))
import analyze_replays as ar

PATH = ROOT / "experiment_results/relay_dual_20260929/mature_dev/0008_V3_portal_portals_seed73001_B.replay"
raw = PATH.read_bytes()
replay = ar.Message(raw).structure(0, 0)
width, height, bodies, teams = ar.parse_map(replay.text(0))
terrain, portals = {}, defaultdict(list)

def geo(c, d):
    dx, dy = [(0, -1), (1, 0), (0, 1), (-1, 0)][d]
    return ((c[0] + dx) % width, (c[1] + dy) % height)

def canon(c, d):
    return (c, d) if d in (1, 2) else (geo(c, d), d ^ 2)

for line in replay.text(0).splitlines():
    words = line.split()
    if not words or words[0] != "EDGE":
        continue
    edge, kind, pid = map(int, words[1:])
    by, x = divmod(edge, width + 1)
    c, d = ((x, (by // 2 - 1) % height), 2) if by % 2 == 0 else (((x - 1) % width, (by - 1) // 2), 1)
    terrain[c, d] = (kind, pid)
    if kind == 2:
        portals[pid].append((c, d))

def step(c, d):
    key = canon(c, d)
    kind, pid = terrain.get(key, (0, -1))
    if kind == 1:
        return None, None
    if kind != 2:
        return geo(c, d), None
    other = [e for e in portals[pid] if e != key]
    assert len(other) == 1
    b, orientation = other[0]
    assert orientation in (d, d ^ 2)
    return (geo(b, orientation) if orientation == d else b), pid

actions, lastmove, splits = {}, {}, {}
history = defaultdict(list)
deaths, counts = [], Counter()
round_num = -1
for event in replay.structs(3):
    kind, data = event.number(0, "H"), event.child(0)
    if kind == 0:
        round_num = data.number(0)
    elif kind == 4:
        ident, action = data.number(0), data.child(0)
        action_type = action.number(0, "H")
        dirs = action.uint16s(0) if action_type == 0 else []
        request = "".join("NESW"[d] for d in dirs) if action_type == 0 else "SPLIT " + str(action.number(4)) if action_type == 1 else "NONE"
        record = dict(round=round_num, request=request, dirs=dirs, steps=0, body_before=list(bodies[ident]))
        actions[ident] = record
        history[ident].append(record)
    elif kind == 9:
        ident = data.number(0)
        head, tail = ar.point(data.child(0)), ar.point(data.child(1))
        if round_num < 0:
            assert head == bodies[ident][0] and tail == bodies[ident][-1]
            continue
        action = actions[ident]
        direction = action["dirs"][action["steps"]]
        dest, pid = step(bodies[ident][0], direction)
        assert dest == head, (round_num, ident, dest, head)
        lastmove[ident] = dict(round=round_num, direction="NESW"[direction], origin=bodies[ident][0], destination=head, portal=pid, length_before=len(bodies[ident]))
        action["steps"] += 1
        counts["validated_success_steps"] += 1
        bodies[ident].insert(0, head)
        while len(bodies[ident]) > 1 and bodies[ident][-1] != tail:
            bodies[ident].pop()
        lastmove[ident]["length_after"] = len(bodies[ident])
    elif kind == 10:
        parent, child = data.number(0), data.number(4)
        bodies[parent] = [ar.point(p) for p in data.structs(0)]
        bodies[child] = [ar.point(p) for p in data.structs(1)]
        teams[child] = "AB"[data.number(8, "H")]
        splits[parent] = dict(round=round_num, parent=parent, child=child, role="parent")
        splits[child] = dict(round=round_num, parent=parent, child=child, role="child")
    elif kind == 11:
        ident, reason = data.number(0), data.number(4, "H")
        if reason == 1 and teams[ident] == "B":
            body, action = bodies[ident], actions[ident]
            assert action["steps"] == 0 and len(action["dirs"]) == 1
            dest, pid = step(body[0], action["dirs"][0])
            assert dest == body[1]
            occupancy = {c: (other, i == 0) for other, cells in bodies.items() for i, c in enumerate(cells)}
            exits = []
            for direction in range(4):
                q, portal = step(body[0], direction)
                owner = occupancy.get(q)
                exits.append(dict(direction="NESW"[direction], destination=q, portal=portal, occupant=owner, empty=q is not None and owner is None))
            static = sum(e["destination"] is not None for e in exits)
            free = sum(e["empty"] for e in exits)
            prior = lastmove.get(ident)
            immediate_return = pid is not None and prior and prior["portal"] == pid and prior["round"] == round_num - 1
            counts["self_deaths"] += 1
            counts["self_death_length"] += len(body)
            counts["via_portal" if pid is not None else "via_ordinary_edge"] += 1
            counts["static_degree_" + str(static)] += 1
            counts["free_exits_" + str(free)] += 1
            counts["length_" + str(len(body))] += 1
            counts["immediate_same_portal_return"] += bool(immediate_return)
            counts["same_round_as_split"] += ident in splits and splits[ident]["round"] == round_num
            counts["length_at_least4_population_below64"] += len(body) >= 4 and sum(t == "B" and d in bodies for d, t in teams.items()) < 64
            deaths.append(dict(round=round_num, id=ident, body=list(body), action=action, prior_actions=history[ident][-3:-1], last_move=prior, recent_split=splits.get(ident), portal=pid, static_degree=static, free_exits=free, exits=exits))
        bodies.pop(ident, None)

result = replay.child(4)
for i, team in enumerate("AB"):
    lengths = [len(body) for ident, body in bodies.items() if teams[ident] == team]
    actual = (len(lengths), max(lengths, default=0), sum(lengths))
    standing = result.child(i)
    assert actual == (standing.number(0), standing.number(4), standing.number(8))
payload = dict(replay=str(PATH), replay_sha256=hashlib.sha256(raw).hexdigest(), counts=dict(counts), final_standings_ok=True, events=deaths)
Path(__file__).with_name("SELF_COLLISION.json").write_text(json.dumps(payload, indent=2) + chr(10))
print(json.dumps(dict(counts)))
print(json.dumps([d for d in deaths if d["free_exits"] > 0][:3]))
print(json.dumps([d for d in deaths if d["static_degree"] != 1][:3]))
