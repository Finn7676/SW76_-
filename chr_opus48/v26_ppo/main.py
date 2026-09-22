# v26 "ppo": a SMALL reinforcement-learning bot trained with PPO.
#
# The movement policy is a LOG-LINEAR SOFTMAX over the (<=4) legal moves:
#     pi(a|s) = softmax_a( THETA . phi(s,a) )
# with only 9 weights (THETA). Inference is a handful of dot products per turn, so
# there is zero timeout risk. This is the entire "model" -- deliberately tiny.
#
# Training (train_ppo.py) is genuine PPO: play games, log (features, action,
# old-logprob, reward) per decision, compute discounted returns + normalised
# advantages, and take a few clipped-surrogate policy-gradient steps. The
# log-linear softmax has a closed-form score  d/dTHETA log pi = phi_a - E_pi[phi],
# so no autodiff framework is needed.
#
# Modes (env BC_PPO_MODE):
#   "train"  -> SAMPLE actions from pi and append trajectory rows to BC_PPO_TRAJ.
#   else     -> DEPLOY: take argmax(THETA.phi); no logging (what the arena runs).
# Weights load from BC_PPO_THETA (default /tmp/bc_ppo_theta.txt) if present, else
# the baked warm-start defaults below -> the uploaded bot is fully self-contained.
import os
import math
import helper as unswbc
from helper import Direction, EdgeType, Position
from collections import deque

ct: unswbc.Controller
game: unswbc.Game

DIRS = Direction.get_direction_list()
NF = 9  # number of features

# PPO-TRAINED weights (train_ppo.py, best-by-validation: 0.500 vs 0.167 warm-start).
# Small magnitudes on purpose -> argmax (deploy) is scale-invariant but the
# training-time softmax explored, so PPO had a non-zero gradient to follow.
# Training nudged the policy toward more open space / escape routes and stronger
# enemy-head avoidance, and away from blindly chasing remembered pearls.
THETA = [0.0, 1.1878, -0.791, 0.3283, -0.3074, -0.5264, 0.08, 0.2264, 0.179]

# Mode & training I/O use FILE sentinels under /tmp, not env vars: unswbc runs the
# bot in a sanitised environment, so env vars do not reach it, but /tmp is readable.
#   /tmp/bc_ppo_train.flag  present  -> TRAIN (sample + log); absent -> DEPLOY (argmax)
#   /tmp/bc_ppo_theta.txt            -> weights override (else baked warm-start)
#   /tmp/bc_ppo_seed.txt             -> episode seed for exploration variety
TRAIN_FLAG = "/tmp/bc_ppo_train.flag"
TRAJ = "/tmp/bc_ppo_traj.txt"
THETA_FILE = "/tmp/bc_ppo_theta.txt"
SEED_FILE = "/tmp/bc_ppo_seed.txt"
MODE_TRAIN = False

W = H = 0
TARGET = 8
SEEN = set()            # positions (x,y) ever seen by this process
PEARL_MEM = {}          # (x,y) -> last round observed with a pearl
_round = 0

# deterministic per-process RNG (no numpy dependency for the bot)
_rng = 12345
def _rand():
    global _rng
    _rng = (_rng * 1103515245 + 12345) & 0x7FFFFFFF
    return _rng / 0x7FFFFFFF


def load_theta():
    global THETA
    for path in (os.environ.get("BC_PPO_THETA"), THETA_FILE):
        if not path:
            continue
        try:
            with open(path) as f:
                vals = [float(x) for x in f.read().split()]
            if len(vals) >= NF:
                THETA = vals[:NF]
                return
        except OSError:
            continue


def wrap1(a, m):
    a = abs(a)
    return min(a, m - a)


def manh(a, b):
    return wrap1(a[0] - b[0], W) + wrap1(a[1] - b[1], H)


def cheb(a, b):
    return max(wrap1(a[0] - b[0], W), wrap1(a[1] - b[1], H))


def tkey(p):
    return (p.x, p.y)


def edge_empty(tile, d):
    return tile is not None and tile.get_edge(d).get_edge_type() == EdgeType.EMPTY


def safe_step(here_tile, here, d):
    if not edge_empty(here_tile, d):
        return False
    nt = ct.get_tile(here.add_dir(d))
    return nt is not None and nt.get_dragon() is None


def free_neighbors(pos):
    t = ct.get_tile(pos)
    if t is None:
        return 0
    n = 0
    for d in DIRS:
        if safe_step(t, pos, d):
            n += 1
    return n


def bounded_flood(start, cap=20):
    seen = {tkey(start)}
    q = deque([start])
    n = 0
    while q and n < cap:
        p = q.popleft()
        n += 1
        t = ct.get_tile(p)
        if t is None:
            continue
        for d in DIRS:
            if not edge_empty(t, d):
                continue
            np = p.add_dir(d)
            nt = ct.get_tile(np)
            if nt is None or nt.get_dragon() is not None:
                continue
            k = tkey(np)
            if k not in seen:
                seen.add(k)
                q.append(np)
    return n


def features(here, here_tile, d, friends, enemy_heads, nearest_vis_pearl, nearest_mem_pearl):
    """9-dim feature vector for taking step `d` from `here`."""
    dest = here.add_dir(d)
    dt = ct.get_tile(dest)
    dk = tkey(dest)
    eats = 1.0 if (dt is not None and dt.has_pearl()) else 0.0

    if nearest_vis_pearl is not None:
        pdist = manh(dk, nearest_vis_pearl)
    else:
        pdist = 12
    pdist_norm = min(pdist, 12) / 12.0

    onward = free_neighbors(dest) / 4.0
    crowd = sum(1 for f in friends if cheb(dk, f) <= 2) / 4.0
    danger = float(sum(1 for e in enemy_heads if cheb(dk, e) <= 1))
    unseen = 1.0 if dk not in SEEN else 0.0
    space = bounded_flood(dest, 20) / 20.0

    toward = 0.0
    if nearest_mem_pearl is not None:
        if manh(dk, nearest_mem_pearl) < manh(tkey(here), nearest_mem_pearl):
            toward = 1.0

    return [1.0, eats, pdist_norm, onward, crowd, danger, unseen, space, toward]


def dot(theta, phi):
    return sum(theta[i] * phi[i] for i in range(NF))


def execute_turn():
    global _round
    here = ct.get_position()
    here_tile = ct.get_tile(here)
    length = ct.get_length()
    pop = ct.get_unit_count()
    me = ct.get_team()
    _round = game.get_round_num()

    friends, enemy_heads = [], []
    nearest_vis_pearl = None
    best_pd = 999
    for t in ct.get_tiles():
        p = t.get_position()
        k = tkey(p)
        SEEN.add(k)
        if t.has_pearl():
            PEARL_MEM[k] = _round
            d = manh(tkey(here), k)
            if d < best_pd:
                best_pd = d
                nearest_vis_pearl = k
        dr = t.get_dragon()
        if dr is not None:
            if dr.get_team() == me:
                if p.x != here.x or p.y != here.y:
                    friends.append(k)
            elif dr.is_head():
                enemy_heads.append(k)

    # age out stale remembered pearls
    if len(PEARL_MEM) > 400:
        for k in [k for k, r in PEARL_MEM.items() if _round - r > 60]:
            PEARL_MEM.pop(k, None)
    nearest_mem_pearl = None
    if PEARL_MEM:
        nearest_mem_pearl = min(PEARL_MEM.keys(), key=lambda k: manh(tkey(here), k))

    # structural rule (not learned): expand the swarm.
    if pop < TARGET and length >= 4 and ct.can_split(2):
        ct.do_split(2)
        return

    # candidate legal moves + their features
    legal = [d for d in DIRS if safe_step(here_tile, here, d)]
    if not legal:
        ct.make_move(Direction.NORTH)
        return

    phis = [features(here, here_tile, d, friends, enemy_heads, nearest_vis_pearl, nearest_mem_pearl) for d in legal]
    logits = [dot(THETA, phi) for phi in phis]

    if MODE_TRAIN:
        m = max(logits)
        exps = [math.exp(l - m) for l in logits]
        z = sum(exps)
        probs = [e / z for e in exps]
        # sample
        r = _rand()
        acc = 0.0
        ci = len(probs) - 1
        for i, pr in enumerate(probs):
            acc += pr
            if r <= acc:
                ci = i
                break
        old_logp = math.log(max(probs[ci], 1e-9))
        # log a trajectory row: gid round len chosen nlegal NF phi_0..phi_{k-1} old_logp
        row = [ct.get_id(), _round, length, ci, len(legal), NF]
        for phi in phis:
            row.extend(phi)
        row.append(old_logp)
        try:
            with open(TRAJ, "a") as f:
                f.write(" ".join(repr(x) for x in row) + "\n")
        except OSError:
            pass
        ct.make_move(legal[ci])
    else:
        # deploy: argmax
        bi = max(range(len(legal)), key=lambda i: logits[i])
        ct.make_move(legal[bi])


def main():
    global ct, game, W, H, TARGET, MODE_TRAIN, _rng
    load_theta()
    MODE_TRAIN = os.path.exists(TRAIN_FLAG)
    # exploration variety across episodes/dragons without relying on env vars
    seed = 1
    try:
        seed = int(open(SEED_FILE).read().strip())
    except (OSError, ValueError):
        pass
    _rng = (seed * 2654435761 + os.getpid() * 97 + 1) & 0x7FFFFFFF
    ct, game = unswbc.init()
    W, H = game.get_map_size()
    TARGET = max(8, min(62, (W * H) // 10))
    while unswbc.update(ct, game):
        try:
            execute_turn()
        except Exception:
            try:
                ct.make_move(Direction.NORTH)
            except Exception:
                pass
        unswbc.end_turn()


if __name__ == "__main__":
    main()
