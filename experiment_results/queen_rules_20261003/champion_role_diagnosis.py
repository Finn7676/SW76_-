#!/usr/bin/env python3
"""Read-only H14 champion assignment diagnosis on three audited development batches."""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
from statistics import mean

from analyze import Message, parse_map, point

HERE = Path(__file__).resolve().parent
TARGETS = (250, 350, 450)
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"


def balanced_slot(dragon_id):
    """Counterfactual 1/8 hash slot only; not executed by the frozen H14 bot."""
    value = (dragon_id ^ 0x9E3779B9) & 0xFFFFFFFF
    value ^= value >> 16
    value = (value * 0x85EBCA6B) & 0xFFFFFFFF
    value ^= value >> 13
    value = (value * 0xC2B2AE35) & 0xFFFFFFFF
    value ^= value >> 16
    return value & 7 == 0


def role_snapshot(dragons, teams, side):
    population = [(did, len(body)) for did, body in dragons.items() if teams[did] == side]
    queen = 0 if side == "A" else 1
    nonqueens = sorted((did, length) for did, length in population if did != queen)
    raw = [(did, length) for did, length in nonqueens if did % 8 == 0]
    natural = [(did, length) for did, length in nonqueens if length >= 12]
    raw_only = [(did, length) for did, length in raw if length < 12]
    actual = [(did, length) for did, length in nonqueens if did % 8 == 0 or length >= 12]
    hashed = [(did, length) for did, length in nonqueens if balanced_slot(did)]
    hashed_only = [(did, length) for did, length in hashed if length < 12]
    return {
        "population": len(population), "nonqueen_population": len(nonqueens),
        "longest": max((length for _, length in population), default=0),
        "nonqueen_longest": max((length for _, length in nonqueens), default=0),
        "queen_length": len(dragons.get(queen, [])),
        "total_length": sum(length for _, length in population),
        "raw_mod8_champions": len(raw), "natural_len12_champions": len(natural),
        "raw_only_champions": len(raw_only), "actual_nonqueen_champions": len(actual),
        "hash_slot_counterfactual": len(hashed), "hash_only_counterfactual": len(hashed_only),
        "mod8_counts": dict(sorted(Counter(did % 8 for did, _ in nonqueens).items())),
        "raw_ids_lengths": raw, "natural_ids_lengths": natural,
        "hash_ids_lengths_counterfactual": hashed,
    }


def scan(game):
    path = Path(game["replay"])
    packed = path.read_bytes()
    digest = hashlib.sha256(packed).hexdigest()
    if digest != game["replay_sha256"]:
        raise ValueError(f"Replay changed after audit: {path}")
    root = Message(packed).structure(0, 0)
    _, _, dragons, teams = parse_map(root.text(0))
    initial = {s: sorted(did for did in dragons if teams[did] == s) for s in "AB"}
    birth_ids = defaultdict(list)
    snapshots = {}
    round_index = -1

    def capture(completed):
        if completed in TARGETS:
            snapshots[str(completed)] = {s: role_snapshot(dragons, teams, s) for s in "AB"}

    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            if round_index >= 0:
                capture(round_index + 1)
            round_index = data.number(0)
        elif kind == 9:
            did = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            body = dragons[did]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            if body[-1] != tail:
                raise ValueError(f"Tail mismatch: {path} dragon {did}")
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            birth_ids[teams[child]].append(child)
        elif kind == 11:
            dragons.pop(data.number(0), None)
    capture(round_index + 1)
    for side in "AB":
        lengths = [len(body) for did, body in dragons.items() if teams[did] == side]
        actual = {"population": len(lengths), "longest": max(lengths, default=0),
                  "total": sum(lengths), "queen": len(dragons.get(0 if side == "A" else 1, []))}
        if actual != game["standings"][side]:
            raise ValueError(f"Reconstruction disagrees with audited final standings: {path}")
    return {
        "replay": str(path), "replay_sha256": digest,
        "map": game["map"], "seed": int(game["seed"]),
        "h14_side": game["bot1_side"], "outcome": game["outcome"],
        "initial_ids": initial,
        "split_births": {s: {"count": len(birth_ids[s]),
            "even": sum(did % 2 == 0 for did in birth_ids[s]),
            "odd": sum(did % 2 == 1 for did in birth_ids[s]),
            "raw_mod8": sum(did % 8 == 0 for did in birth_ids[s])} for s in "AB"},
        "snapshots": snapshots,
    }


def aggregate(games):
    groups = defaultdict(list)
    for game in games:
        for completed, snapshots in game["snapshots"].items():
            for side, snapshot in snapshots.items():
                owner = "H14" if side == game["h14_side"] else "xxqztest"
                groups[(int(completed), owner, side)].append(snapshot)
    rows = []
    for (completed, owner, side), snapshots in sorted(groups.items()):
        summed = {key: sum(s[key] for s in snapshots) for key in (
            "nonqueen_population", "raw_mod8_champions", "natural_len12_champions",
            "raw_only_champions", "actual_nonqueen_champions",
            "hash_slot_counterfactual", "hash_only_counterfactual")}
        count = summed["nonqueen_population"]
        rows.append({"completed_rounds": completed, "bot": owner, "side": side,
            "games_reaching_round": len(snapshots), **summed,
            "raw_fraction": summed["raw_mod8_champions"] / count if count else None,
            "hash_fraction_counterfactual": summed["hash_slot_counterfactual"] / count if count else None,
            "games_zero_raw": sum(s["raw_mod8_champions"] == 0 for s in snapshots),
            "mean_population": mean(s["population"] for s in snapshots),
            "mean_longest": mean(s["longest"] for s in snapshots),
            "mean_nonqueen_longest": mean(s["nonqueen_longest"] for s in snapshots),
            "mean_natural_champions": mean(s["natural_len12_champions"] for s in snapshots),
        })
    return rows


def markdown(payload):
    lines = ["# Champion Role Diagnosis", "", "Read-only reconstruction of H14 versus unchanged xxqztest from three audited",
             "ranked17 development batches (82001, 83001, 84001), both seats. Snapshots",
             "are after 250/350/450 complete rounds. Counts exclude fixed queens.", "",
             "| Round | Bot | Seat | Games | Units Mean | Longest Mean | Raw ID%8 | Natural LEN12 | Raw Fraction | Zero-Raw Games |",
             "|---:|---|---|---:|---:|---:|---:|---:|---:|---:|"]
    for row in payload["aggregates"]:
        fraction = "NA" if row["raw_fraction"] is None else f'{100 * row["raw_fraction"]:.1f}%'
        lines.append(f'| {row["completed_rounds"]} | {row["bot"]} | {row["side"]} | '
            f'{row["games_reaching_round"]} | {row["mean_population"]:.2f} | '
            f'{row["mean_longest"]:.2f} | {row["raw_mod8_champions"]} | '
            f'{row["natural_len12_champions"]} | {fraction} | {row["games_zero_raw"]} |')
    lines += ["", "Raw and natural pools overlap; summing them would double count. Longest",
              "includes the queen; nonqueen longest and full ID/length lists are in JSON.",
              "A completed early-elimination match is not silently carried forward as a",
              "250/350/450 snapshot. Its omission is explicit in games-reaching-round.", "",
              "## Scope", "", "Both H14 and xxqztest use the raw global ID modulo-eight criterion after",
              "round 250, alongside LEN>=12; H14 additionally always protects its queen.",
              "The raw role changes splitting, combat willingness and scoring, but this",
              "diagnosis does not establish that the ID imbalance causes any loss.",
              "Actual split births are recorded rather than assuming every later ID keeps",
              "the initial team parity. Survival itself can also bias residue frequency.", "",
              "## Single-Factor Candidate", "", "Replace only H14's MYID%8==0 slot with a fixed 32-bit avalanche hash of",
              "global MYID selecting low-three-bit zero. Preserve ROUND>=250, LEN>=12,",
              "queen protection and every other policy. This decouples selection from",
              "initial even/odd allocation without depending on hidden team population or",
              "assuming split IDs alternate by team. The proposed mixer is explicitly",
              "recorded in balanced_slot() in this script; its counterfactual counts on",
              "the existing trajectories are not actual changed-policy results.",
              "Changing the protected cohort can still reduce exploration or cause poor",
              "growth; only paired same-seed frozen matches can determine its benefit.",
              "No bots, opponents or match artifacts were modified. No matches were run.", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-prefix", type=Path, default=HERE / "champion_role_diagnosis")
    args = parser.parse_args()
    json_path, md_path = args.output_prefix.with_suffix(".json"), args.output_prefix.with_suffix(".md")
    if json_path.exists() or md_path.exists():
        parser.error("Outputs exist; choose a new prefix to preserve evidence")
    games, audits = [], []
    for base in (82001, 83001, 84001):
        suite = HERE / f"robust_h14_{base}"
        audit_path = suite / "verified_audit.json"
        audit = json.loads(audit_path.read_text())
        config = json.loads((suite / "run.json").read_text())
        if not audit["summary"]["audit_passed"]:
            raise ValueError(f"Suite audit did not pass: {suite}")
        if config["sources"][0]["files"]["main.cpp"] != H14_MAIN:
            raise ValueError(f"Candidate is not frozen H14: {suite}")
        selected = [game for game in audit["games"] if game["opponent"] == "xxqztest"]
        if len(selected) != 34:
            raise ValueError(f"Expected ranked17 x both seats versus xxqztest: {suite}")
        audits.append({"suite": str(suite), "audit_sha256": hashlib.sha256(audit_path.read_bytes()).hexdigest()})
        for game in selected:
            games.append(scan(game))
        print(f"Reconstructed {suite.name}: {len(selected)} games", flush=True)
    payload = {
        "scope": "H14 vs xxqztest, ranked17, both seats, three development batches",
        "snapshot_definition": "After 250/350/450 complete rounds; missing if match ended earlier",
        "h14_main_sha256": H14_MAIN,
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "audits": audits, "games": games, "aggregates": aggregate(games),
        "counterfactual_warning": "Hash slots counted on unchanged H14 trajectories; not a policy evaluation",
    }
    with json_path.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with md_path.open("x") as handle:
        handle.write(markdown(payload))
    print(json.dumps({"games": len(games), "outputs": [str(json_path), str(md_path)],
                      "aggregates": payload["aggregates"]}, indent=2))


if __name__ == "__main__":
    main()
