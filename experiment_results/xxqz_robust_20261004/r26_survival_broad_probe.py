"""Read-only H14 nonqueen current-view hostile head-risk development scan."""
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path('/home/tyhh/SW76_-')
RUNS = ROOT / 'experiment_results/queen_rules_20261003'
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, sha, REASONS
from queen_immediate_witness import topology, visible_cells, project, ordinary_complete, continuation
from queen_immediate_engine import action

LETTERS = 'NESW'
counts = Counter()
games_counts = Counter()
findings = []
raw_deaths = []

for base in (82001, 83001, 84001):
    run = RUNS / f'robust_h14_{base}'
    config = json.loads((run / 'run.json').read_text())
    audit = json.loads((run / 'verified_audit.json').read_text())
    assert audit['summary']['audit_passed'] and config['unswbc'] == '1.2.7'
    assert config['sources'][0]['files']['main.cpp'] == 'ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc'
    selected = [g for g in audit['games'] if g['opponent'] == 'xxqztest']
    assert len(selected) == 34
    for game in selected:
        path = Path(game['replay'])
        assert sha(path) == game['replay_sha256']
        root = Message(path.read_bytes()).structure(0, 0)
        w, h, bodies, teams = parse_map(root.text(0))
        _, _, edges = topology(root.text(0))
        own = game['bot1_side']
        queen = 'AB'.index(own)
        pearls = set()
        last_action = {}
        turn = -1
        current_action = None
        round_index = -1
        had = set()

        def witnesses(post, other, view, shown, enemies):
            found = []
            for enemy_id, enemy_body in enemies:
                occupancy = (other - set(enemy_body)) | set(post['body'])
                remaining = shown - set(post['eaten'])
                for direction in range(4):
                    result = project(enemy_body, [direction], occupancy, remaining, view, edges, w, h, target=post['body'][0])
                    if result.get('reason') == 'hit_target':
                        gap = len(post['body']) - len(enemy_body)
                        found.append({'id': enemy_id, 'enemy_length': len(enemy_body), 'own_post_length': len(post['body']), 'trade_gap': gap, 'enemy_body': enemy_body, 'attack': 'MOVE ' + LETTERS[direction], 'proof': result})
                        break
            return found

        for index, event in enumerate(root.structs(3)):
            kind, data = event.number(0, 'H'), event.child(0)
            if kind == 0:
                round_index = data.number(0)
            elif kind == 1:
                turn = data.number(0)
                current_action = None
            elif kind == 3:
                cell = point(data.child(0))
                pearls.add(cell) if data.number(0, 'B') & 1 else pearls.discard(cell)
            elif kind == 4:
                did = data.number(0)
                item = data.child(0)
                akind = item.number(0, 'H')
                dirs = item.uint16s(0) if akind == 0 else []
                body = list(bodies[did])
                current_action = {'id': did, 'round': round_index, 'event': index, 'action': action(data), 'directions': dirs, 'body_before': body}
                if teams[did] != own or did == queen:
                    continue
                last_action[did] = current_action.copy()
                if akind != 0 or not dirs:
                    continue
                counts['nonqueen_movement_actions'] += 1
                view = visible_cells(body[0], w, h)
                complete = ordinary_complete(body, view, edges, w, h)
                last_action[did]['own_complete'] = complete
                if not complete:
                    counts['partial_or_portal_own_movement'] += 1
                    continue
                counts['complete_ordinary_own_movement'] += 1
                other = {cell for eid, eb in bodies.items() if eid != did for cell in eb if cell in view}
                shown = pearls & view
                post = project(body, dirs, other, shown, view, edges, w, h)
                if not post['known'] or not post['survives']:
                    continue
                counts['known_living_ordinary_movement'] += 1
                last_action[did]['known_living_ordinary'] = True
                enemies = [(eid, list(eb)) for eid, eb in bodies.items() if teams[eid] != own and ordinary_complete(eb, view, edges, w, h)]
                danger = witnesses(post, other, view, shown, enemies)
                if not danger:
                    continue
                counts['complete_enemy_firststep_exposure'] += 1
                had.add('exposure')
                shorter = [d for d in danger if d['trade_gap'] > 0]
                gap3 = [d for d in danger if d['trade_gap'] >= 3]
                counts['shorter_enemy_exposure'] += bool(shorter)
                counts['gap3_enemy_exposure'] += bool(gap3)
                counts['pearl_exposure'] += bool(post['eaten'])
                alts = []
                for direction in range(4):
                    alternative = project(body, [direction], other, shown, view, edges, w, h)
                    if not alternative['known'] or not alternative['survives']:
                        continue
                    onward = continuation(alternative['body'], other, view, edges, w, h)
                    if onward is None:
                        continue
                    alt_danger = witnesses(alternative, other, view, shown, enemies)
                    alt_short = [d for d in alt_danger if d['trade_gap'] > 0]
                    if len(alt_short) >= len(shorter):
                        continue
                    alts.append({'action': 'MOVE ' + LETTERS[direction], 'projection': alternative, 'continuation': onward, 'firststep_witnesses': alt_danger})
                if alts:
                    counts['shorter_exposure_with_reduced_alternate'] += 1
                    had.add('alternate')
                record = {**current_action, 'base': base, 'game': game['game'], 'map': game['map'], 'seed': int(game['seed']), 'side': own, 'outcome': game['outcome'], 'replay': str(path), 'replay_sha256': game['replay_sha256'], 'population': sum(teams[x] == own for x in bodies), 'own_complete': True, 'visible_pearls': sorted(shown), 'visible_other_cells': sorted(other), 'original_projection': post, 'firststep_witnesses': danger, 'alternatives': alts}
                last_action[did]['risk_record'] = record
            elif kind == 9:
                did = data.number(0)
                head, tail = point(data.child(0)), point(data.child(1))
                before = bodies[did]
                if turn < 0 and head == before[0] and tail == before[-1]:
                    continue
                body = [head] + before
                while len(body) > 1 and body[-1] != tail:
                    body.pop()
                assert body[-1] == tail
                bodies[did] = body
            elif kind == 10:
                parent, child = data.number(0), data.number(4)
                bodies[parent] = [point(p) for p in data.structs(0)]
                bodies[child] = [point(p) for p in data.structs(1)]
                teams[child] = 'AB'[data.number(8, 'H')]
            elif kind == 11:
                did, reason = data.number(0), REASONS[data.number(4, 'H')]
                if teams[did] == own and did != queen and reason == 'head' and current_action and teams[current_action['id']] != own:
                    counts['all_hostile_nonqueen_head_deaths'] += 1
                    previous = last_action.get(did)
                    if previous is not None:
                        raw = {'base': base, 'map': game['map'], 'outcome': game['outcome'], 'seed': int(game['seed']), 'side': own, 'id': did, 'length_at_death': len(bodies[did]), 'round': round_index, 'death_event': index, 'previous_action': {key: value for key, value in previous.items() if key != 'risk_record'}, 'attacker_actual': current_action.copy()}
                        raw_deaths.append(raw)
                        if 'risk_record' in previous:
                            record = previous['risk_record']
                            responsible = [d for d in record['firststep_witnesses'] if d['id'] == current_action['id']]
                            if responsible:
                                record = {**record, 'death_round': round_index, 'death_event': index, 'head_at_death': bodies[did][0], 'attacker_actual': current_action.copy(), 'actual_attacker_witness': responsible}
                                findings.append(record)
                                counts['actual_complete_firststep_witness_deaths'] += 1
                                counts['actual_shorter_firststep_witness_deaths'] += any(d['trade_gap'] > 0 for d in responsible)
                                counts['actual_gap3_firststep_witness_deaths'] += any(d['trade_gap'] >= 3 for d in responsible)
                                counts['actual_witness_deaths_with_alternate'] += bool(record['alternatives'])
                                had.add('actual_witness_death')
                                if game['outcome'] == 'loss' and record['alternatives'] and any(d['trade_gap'] >= 3 for d in responsible):
                                    print('LOSS_WITNESS ' + json.dumps(record), flush=True)
                bodies.pop(did, None)
                last_action.pop(did, None)
        standing = {side: {'population': sum(teams[did] == side for did in bodies), 'total': sum(len(body) for did, body in bodies.items() if teams[did] == side), 'longest': max((len(body) for did, body in bodies.items() if teams[did] == side), default=0), 'queen': len(bodies.get('AB'.index(side), []))} for side in 'AB'}
        assert standing == game['standings']
        for label in had:
            games_counts[label] += 1
    print('DONE_BASE ' + str(base), flush=True)

payload = {'counts': counts, 'game_counts': games_counts, 'findings': findings, 'raw_hostile_deaths': raw_deaths, 'raw_deaths_by_action': Counter(x['previous_action']['action'].split()[0] if x['previous_action']['action'] else 'SUICIDE' for x in raw_deaths), 'raw_deaths_by_own_complete': Counter(str(x['previous_action'].get('own_complete')) for x in raw_deaths), 'strict_deaths_by_original_steps': Counter(len(x['directions']) for x in findings), 'strict_deaths_by_base': Counter(x['base'] for x in findings), 'strict_deaths_by_map': Counter(x['map'] for x in findings)}
output = ROOT / 'experiment_results/xxqz_robust_20261004/r26_survival_broad_probe.json'
with output.open('x') as handle:
    json.dump(payload, handle, indent=2)
print(json.dumps({key: value for key, value in payload.items() if key not in ('findings', 'raw_hostile_deaths')}, indent=2))
print('OUTPUT ' + str(output))
