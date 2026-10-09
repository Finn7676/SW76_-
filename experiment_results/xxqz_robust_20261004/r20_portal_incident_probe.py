#!/usr/bin/env python3
"""Action-tape input evidence for queen r32 incoming-portal risk, not a bot."""
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys

from unswbc.engine import EngineModule, WASM_PATH

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0,str(RUNS))
import queen_immediate_engine as tape
from queen_immediate_witness import Message, topology, landing, project, continuation
from analyze import point, sha

REPLAY = RUNS / "robust_queen_witness_82001/0058_xxqztest_islands_seed82012_B.replay"
REPLAY_SHA = "7106610fddda33580282fcb5eb69a94f011df1a43d6152d8aba8bd64f813ca2a"
LETTERS = "NESW"


def read_source():
    assert sha(REPLAY) == REPLAY_SHA
    root = Message(REPLAY.read_bytes()).structure(0,0)
    tape.TARGET_ROUND,tape.QUEEN,tape.ATTACKER = 32,1,12
    source = tape.replay_data(root)
    assert source["tape"][(32,1)]["action"] == "MOVE S"
    return root.text(0).encode(), source


def raw_arm(map_bytes, source, replacement):
    engine = EngineModule()
    observed, spawns, deaths = None,{},[]
    def spawn(did,block):
        spawns[did] = block.decode()
    def reply(did,block):
        nonlocal observed
        r = int(block.decode().splitlines()[0].split()[1])
        if r > 32 or (r == 32 and did > 12):
            return b"ENDTURN\n"
        item = source["tape"][(r,did)]
        move = item["action"]
        if r == 32 and did == 1:
            observed = tape.observation(block)
            if replacement is not None:
                move = "MOVE " + replacement
        return ("\n".join(([move] if move else []) + item["sonar"] + ["ENDTURN"]) + "\n").encode()
    engine.run(map_bytes,reply,lambda d,r,why: deaths.append([d,r,why]),spawn,lambda _:None,seed=82012)
    packed = engine.replay("R20 action-tape diagnosis A","R20 action-tape diagnosis B")
    root = Message(packed).structure(0,0)
    regenerated = tape.replay_data(root)
    assert regenerated["prefix"] == source["prefix"]
    assert regenerated["target"]["bodies"] == source["target"]["bodies"]
    assert observed is not None
    return {"replacement": replacement,"observed": observed,"spawn":spawns[1],
            "prefix_canonical_events":len(source["prefix"]),"prefix_equal":True,
            "r32_queen_or_friend12_deaths":[d for d in deaths if d[1]==32 and d[0] in (1,12)],
            "fixture_replay_sha256":hashlib.sha256(packed).hexdigest()},packed


def alternatives(source,observed,map_bytes):
    w,h,edges = topology(map_bytes.decode())
    snapshot = source["target"]
    own = snapshot["bodies"][1]
    visible = {(x,y) for x,y,_,_ in observed["tiles"]}
    own_shown = {(int(p[2]),int(p[3])) for p in observed["parts"] if p[0]=="B" and int(p[1])==1}
    assert set(own)==own_shown and len(own)==observed["length"]==6
    other = {cell for did,body in snapshot["bodies"].items() if did!=1 for cell in body if cell in visible}
    pearls = snapshot["pearls"] & visible
    original = project(own,(2,),other,pearls,visible,edges,w,h)
    assert original["known"] and original["survives"]
    assert edges.get((original["body"][0],2),(0,-1)) == (2,1)
    free = (len(own)+3)//4
    rows=[]
    stack=[()]
    while stack:
        path=stack.pop()
        if len(path)>=free: continue
        for d in range(4):
            trial=path+(d,)
            post=project(own,trial,other,pearls,visible,edges,w,h)
            if not post["known"] or not post["survives"]: continue
            stack.append(trial)
            head=post["body"][0]
            if len(post["body"])!=6 or any(edges.get((head,e),(0,-1))[0]==2 for e in range(4)): continue
            onward=continuation(post["body"],other,visible,edges,w,h)
            if onward is None: continue
            adjacent_enemy = any(abs(x-head[0])+abs(y-head[1])==1 for team,did,x,y,face,is_head in observed["parts"]
                                 if team=="A" and int(is_head) for x,y in [(int(x),int(y))])
            rows.append({"action":"".join(LETTERS[e] for e in trial),"projection":post,
                         "ordinary_continuation":onward,"portal_incident":False,
                         "enemy_head_geometrically_adjacent":adjacent_enemy})
    rows.sort(key=lambda a:(a["enemy_head_geometrically_adjacent"],len(a["action"]),a["action"]))
    return {"original":original,"candidate_alternatives":rows,
            "current_visible_enemy_parts":[p for p in observed["parts"] if p[0]=="A"],
            "current_visible_other_friend_parts":[p for p in observed["parts"] if p[0]=="B" and int(p[1])!=1],
            "origin_friend12_visible":snapshot["bodies"][12][0] in visible,
            "original_final_head_portal_edges":[{"dir":LETTERS[d],"edge":edges.get((original["body"][0],d),(0,-1))} for d in range(4)],
            "limitations":["Enemy geometric adjacency is descriptive, not an enemy legal-reach certificate.",
                           "No absent portal at endpoint proves incoming portal is impossible through that endpoint, not overall safety.",
                           "Known ordinary same-length survival/continuation uses the actual current view; later actors and hidden threats are unknown."]}


def main():
    output=HERE/"r20_portal_incident_probe.json"
    assert not output.exists()
    assert importlib.metadata.version("unswbc")=="1.2.7" and sha(WASM_PATH)==tape.ENGINE_SHA
    map_bytes,source=read_source()
    actual,packed=raw_arm(map_bytes,source,None)
    analysis=alternatives(source,actual["observed"],map_bytes)
    chosen=analysis["candidate_alternatives"][0]["action"]
    alternate,otherpacked=raw_arm(map_bytes,source,chosen)
    assert alternate["observed"]==actual["observed"]
    paths=[]
    for name,body in (("actual",packed),("alternate",otherpacked)):
        p=HERE/f"r20_portal_incident_probe_{name}.replay"
        with p.open("xb") as f:f.write(body)
        paths.append(str(p))
    payload={"passed":True,"scope":"Existing development replay action-tape diagnosis, not competition or candidate implementation",
             "source_sha256":sha(__file__),"replay":str(REPLAY),"replay_sha256":REPLAY_SHA,
             "engine_sha256":sha(WASM_PATH),"seed":82012,"round":32,
             "analysis":analysis,"arms":[actual,alternate],"fixture_replays":paths,
             "limitations":["All actions/SONAR before target are recorded R20 actions; no bot is called.",
                             "The alternate is an offline local proof and scripted counterfactual, not a bot policy or win-rate claim.",
                             "Recorded friend12 action is held fixed at32; it may change under a live policy's alternate.",
                             "ENDTURN cleanup after id12 is fixture-only and not bot behavior."]}
    with output.open("x") as f:json.dump(payload,f,indent=2);f.write("\n")
    print("actual",actual["r32_queen_or_friend12_deaths"],"alternate",chosen,alternate["r32_queen_or_friend12_deaths"])
    print("raw_input_sha",actual["observed"]["sha256"],"visiblefriends",analysis["current_visible_other_friend_parts"])
    print("alternatives",[(r["action"],r["projection"]["body"][0],r["ordinary_continuation"]) for r in analysis["candidate_alternatives"]])
    print("output",output)


if __name__=="__main__":main()
