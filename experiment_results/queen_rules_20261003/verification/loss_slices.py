"""Read-only replay extraction of fixed-queen decisions and death context."""
import argparse
import csv
import json
import sys
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE))
from analyze import Message, point, parse_map, REASONS


def extract(path, side):
    root = Message(path.read_bytes()).structure(0, 0)
    _, _, dragons, teams = parse_map(root.text(0))
    queen = 0 if side == "A" else 1
    history = []
    turn = -1
    for ev in root.structs(3):
        kind, data = ev.number(0, "H"), ev.child(0)
        if kind == 0:
            turn = data.number(0)
        elif kind == 4:
            did = data.number(0)
            if did != queen:
                continue
            action = data.child(0)
            action_kind = action.number(0, "H")
            value = action.uint16s(0) if action_kind == 0 else action.number(4) if action_kind == 1 else None
            history.append({"round": turn, "kind": "action", "type": action_kind,
                            "value": value, "body": list(dragons.get(queen, []))})
        elif kind == 9:
            did = data.number(0)
            body = dragons[did]
            head, tail = point(data.child(0)), point(data.child(1))
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            did = data.number(0)
            if did == queen:
                return {"round": turn, "reason": REASONS[data.number(4, "H")],
                        "body": dragons[queen], "last_actions": history[-8:]}
            dragons.pop(did, None)
    return {"survived": True, "length": len(dragons.get(queen, [])), "last_actions": history[-8:]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("suite", type=Path)
    parser.add_argument("--maps", nargs="+", default=["default", "portals"])
    args = parser.parse_args()
    rows = list(csv.DictReader((args.suite / "results.csv").open()))
    result = [{"opponent": r["opponent"], "map": r["map"], "side": r["bot1_side"],
               "outcome": r["outcome"], "replay": r["replay"],
               "queen": extract(args.suite / r["replay"], r["bot1_side"])}
              for r in rows if r["map"] in args.maps and r["outcome"] in ("win", "loss", "draw")]
    (args.suite / "queen_loss_slices.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
