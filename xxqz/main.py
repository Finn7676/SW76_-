import helper as unswbc
from helper import Direction, EdgeType, Position
from collections import deque

ct: unswbc.Controller
game: unswbc.Game

DIRS = Direction.get_direction_list()
DIR_INDEX = {d: i for i, d in enumerate(DIRS)}
OPPOSITE = {d: d.get_opposite() for d in DIRS}
OFFSET = {d: d.get_offset() for d in DIRS}

LOCAL_BFS_DEPTH = 12
LOCAL_BFS_NODES = 220


class Memory:
    """Persistent per-dragon map, rebuilt only from this process's vision."""

    __slots__ = (
        "w", "h", "seen", "edge", "portal_obs", "pearls", "eta",
        "body", "last_round", "last_head",
    )

    def __init__(self) -> None:
        self.w = 0
        self.h = 0
        self.seen: set[tuple[int, int]] = set()
        self.edge: dict[tuple[int, int, int], tuple[EdgeType, int]] = {}
        self.portal_obs: dict[int, set[tuple[int, int, int]]] = {}
        self.pearls: set[tuple[int, int]] = set()
        self.eta: dict[tuple[int, int], int] = {}
        self.body: list[tuple[int, int]] = []
        self.last_round = 0
        self.last_head: tuple[int, int] | None = None

    def configure(self, width: int, height: int) -> None:
        self.w = width
        self.h = height

    def step(self, x: int, y: int, direction: Direction) -> tuple[int, int]:
        dx, dy = OFFSET[direction]
        return (x + dx) % self.w, (y + dy) % self.h

    def canon_edge(self, x: int, y: int, direction: Direction) -> tuple[int, int, int]:
        if direction is Direction.EAST or direction is Direction.SOUTH:
            return x, y, DIR_INDEX[direction]
        nx, ny = self.step(x, y, direction)
        return nx, ny, DIR_INDEX[OPPOSITE[direction]]

    def observe(self) -> set[tuple[int, int]]:
        occupied: set[tuple[int, int]] = set()
        visible: set[tuple[int, int]] = set()
        my_id = ct.get_id()
        my_parts: list[tuple[int, int, Direction, bool]] = []

        for tile in ct.get_tiles():
            pos = tile.get_position()
            x, y = pos.x, pos.y
            visible.add((x, y))
            self.seen.add((x, y))
            if tile.has_pearl():
                self.pearls.add((x, y))
            else:
                self.pearls.discard((x, y))
            self.eta[x, y] = tile.get_pearl_time()
            part = tile.get_dragon()
            if part is not None:
                occupied.add((x, y))
                if part.get_id() == my_id:
                    my_parts.append((x, y, part.get_dir(), part.is_head()))
            for direction in DIRS:
                edge = tile.get_edge(direction)
                kind = edge.get_edge_type()
                pid = edge.get_portal_id()
                key = (x, y, DIR_INDEX[direction])
                self.edge[key] = (kind, pid)
                if kind is EdgeType.PORTAL:
                    self.portal_obs.setdefault(pid, set()).add(key)

        self._tick_unseen_eta(visible)
        self._sync_body(my_parts)
        return occupied

    def _tick_unseen_eta(self, visible: set[tuple[int, int]]) -> None:
        round_num = game.get_round_num()
        delta = round_num - self.last_round if self.last_round else 0
        self.last_round = round_num
        if delta <= 0:
            return
        stale = []
        for pos, eta in self.eta.items():
            if pos in visible or eta < 0:
                continue
            nxt = eta - delta
            if nxt < 0:
                stale.append(pos)
            else:
                self.eta[pos] = nxt
        for pos in stale:
            self.eta.pop(pos, None)

    def _sync_body(self, my_parts: list[tuple[int, int, Direction, bool]]) -> None:
        head = (ct.get_position().x, ct.get_position().y)
        length = ct.get_length()
        if self.body and self.body[0] == head and len(self.body) == length:
            self.last_head = head
            return

        facing_at = {head: ct.get_dir()}
        for x, y, direction, _is_head in my_parts:
            facing_at[x, y] = direction

        chain = [head]
        pos = head
        facing = ct.get_dir()
        for _ in range(length - 1):
            nxt = self.step(*pos, OPPOSITE[facing])
            chain.append(nxt)
            if nxt in facing_at:
                facing = facing_at[nxt]
                pos = nxt
                continue
            if self.body and pos in self.body:
                idx = self.body.index(pos)
                if idx + 1 < len(self.body) and self.body[idx + 1] == nxt:
                    pos = nxt
                    continue
            break

        if len(chain) < length and self.body and self.body[0] == head:
            for cell in self.body[len(chain):]:
                if cell in chain:
                    continue
                chain.append(cell)
                if len(chain) == length:
                    break
        self.body = chain[:length]
        self.last_head = head

    def transit(self, x: int, y: int, direction: Direction) -> tuple[str, tuple[int, int] | None]:
        live = ct.get_tile(Position(x, y))
        if live is not None:
            edge = live.get_edge(direction)
            kind, pid = edge.get_edge_type(), edge.get_portal_id()
        else:
            info = self.edge.get((x, y, DIR_INDEX[direction]))
            if info is None:
                return "unknown", self.step(x, y, direction)
            kind, pid = info
        if kind is EdgeType.KELP:
            return "kelp", None
        if kind is EdgeType.PORTAL:
            dest = self.portal_dest(x, y, direction, pid)
            if dest is None:
                return "unknown_portal", None
            return "portal", dest
        return "empty", self.step(x, y, direction)

    def portal_dest(
        self, x: int, y: int, direction: Direction, pid: int
    ) -> tuple[int, int] | None:
        here_canon = self.canon_edge(x, y, direction)
        for ox, oy, od in self.portal_obs.get(pid, ()):
            other_dir = DIRS[od]
            other_canon = self.canon_edge(ox, oy, other_dir)
            if other_canon == here_canon:
                continue
            if other_dir is direction:
                return self.step(ox, oy, other_dir)
            if other_dir is OPPOSITE[direction]:
                return (ox, oy)
        return None

    def apply_moves(self, moves: list[Direction], occupied_others: set[tuple[int, int]]) -> None:
        body = list(self.body)
        pearls = set(self.pearls)
        for i, direction in enumerate(moves):
            if not body:
                break
            kind, dest = self.transit(*body[0], direction)
            if dest is None or kind == "kelp":
                break
            if i > 0 and len(body) <= 2:
                break
            ate = dest in pearls
            body.insert(0, dest)
            if not ate and len(body) > 1:
                body.pop()
            if i > 0 and len(body) > 2:
                body.pop()
            pearls.discard(dest)
        self.body = body
        self.pearls = pearls
        self.last_head = body[0] if body else None


mem = Memory()


def torus_delta(a: int, b: int, n: int) -> int:
    d = (b - a) % n
    if d > n - d:
        return d - n
    return d


def torus_dist(a: tuple[int, int], b: tuple[int, int]) -> int:
    return abs(torus_delta(a[0], b[0], mem.w)) + abs(torus_delta(a[1], b[1], mem.h))


def simulate(
    moves: list[Direction],
    occupied_others: set[tuple[int, int]],
) -> tuple[bool, list[tuple[int, int]], int, bool]:
    """Return (ok, body, eats, used_unknown_portal)."""
    body = list(mem.body)
    pearls = set(mem.pearls)
    eats = 0
    unknown_portal = False
    occupied_own = set(body)
    for i, direction in enumerate(moves):
        if not body:
            return False, body, eats, unknown_portal
        kind, dest = mem.transit(*body[0], direction)
        if kind == "kelp" or dest is None:
            return False, body, eats, unknown_portal
        if kind in ("unknown_portal", "unknown"):
            unknown_portal = True
            if i > 0 or kind == "unknown_portal":
                return False, body, eats, unknown_portal
        if i > 0 and len(body) <= 2:
            return False, body, eats, unknown_portal
        if dest in occupied_others:
            return False, body, eats, unknown_portal
        if dest in occupied_own:
            return False, body, eats, unknown_portal
        ate = dest in pearls
        occupied_own.add(dest)
        body.insert(0, dest)
        if not ate:
            tail = body.pop()
            occupied_own.discard(tail)
        if i > 0:
            if len(body) <= 2:
                return False, body, eats, unknown_portal
            extra = body.pop()
            occupied_own.discard(extra)
        if ate:
            pearls.discard(dest)
            eats += 1
    return True, body, eats, unknown_portal


def local_bfs(
    occupied_others: set[tuple[int, int]],
    goals: set[tuple[int, int]],
    allow_unseen: bool,
) -> list[Direction] | None:
    start = mem.body[0]
    if start in goals:
        return []
    q = deque([start])
    prev: dict[tuple[int, int], tuple[tuple[int, int], Direction]] = {}
    dist = {start: 0}
    found = None
    while q and len(dist) < LOCAL_BFS_NODES:
        x, y = q.popleft()
        d0 = dist[x, y]
        if d0 >= LOCAL_BFS_DEPTH:
            continue
        for direction in DIRS:
            kind, dest = mem.transit(x, y, direction)
            if dest is None or kind in ("kelp", "unknown_portal"):
                continue
            if dest in dist:
                continue
            if dest in occupied_others or dest in mem.body:
                continue
            if not allow_unseen and dest not in mem.seen and kind != "portal":
                continue
            dist[dest] = d0 + 1
            prev[dest] = ((x, y), direction)
            if dest in goals:
                found = dest
                q.clear()
                break
            if dest not in mem.seen:
                continue
            q.append(dest)
        else:
            continue
        break
    if found is None:
        return None
    path: list[Direction] = []
    cur = found
    while cur != start:
        parent, direction = prev[cur]
        path.append(direction)
        cur = parent
    path.reverse()
    return path


def nearest_goal(goals: set[tuple[int, int]]) -> tuple[int, int] | None:
    if not goals or not mem.body:
        return None
    head = mem.body[0]
    return min(goals, key=lambda p: (torus_dist(head, p), p))


def heading_towards(target: tuple[int, int]) -> list[Direction]:
    head = mem.body[0]
    dx = torus_delta(head[0], target[0], mem.w)
    dy = torus_delta(head[1], target[1], mem.h)
    order: list[Direction] = []
    if abs(dx) >= abs(dy):
        if dx > 0:
            order.append(Direction.EAST)
        elif dx < 0:
            order.append(Direction.WEST)
        if dy > 0:
            order.append(Direction.SOUTH)
        elif dy < 0:
            order.append(Direction.NORTH)
    else:
        if dy > 0:
            order.append(Direction.SOUTH)
        elif dy < 0:
            order.append(Direction.NORTH)
        if dx > 0:
            order.append(Direction.EAST)
        elif dx < 0:
            order.append(Direction.WEST)
    for direction in DIRS:
        if direction not in order:
            order.append(direction)
    return order


def neighbor_open_count(pos: tuple[int, int], occupied: set[tuple[int, int]]) -> int:
    n = 0
    x, y = pos
    for direction in DIRS:
        kind, dest = mem.transit(x, y, direction)
        if dest is None or kind in ("kelp", "unknown_portal"):
            continue
        if dest in occupied or dest in mem.body:
            continue
        n += 1
    return n


def visible_pocket(
    start: tuple[int, int], blocked: set[tuple[int, int]]
) -> tuple[set[tuple[int, int]], bool]:
    """Visible empty tiles reachable from start, and whether a passable edge leaves vision."""
    if start in blocked:
        return set(), False
    tiles = {start}
    q = deque([start])
    escapes = False
    while q:
        x, y = q.popleft()
        for direction in DIRS:
            kind, dest = mem.transit(x, y, direction)
            if dest is None or kind in ("kelp", "portal", "unknown_portal"):
                continue
            if dest in blocked or dest in tiles:
                continue
            if ct.get_tile(Position(*dest)) is None:
                escapes = True
                continue
            tiles.add(dest)
            q.append(dest)
    return tiles, escapes


def is_visible_dead_end(dest: tuple[int, int], occupied: set[tuple[int, int]]) -> bool:
    """True when the destination is a closed pocket fully inside the 7x7 window."""
    if ct.get_tile(Position(*dest)) is None:
        return False
    head = ct.get_position()
    blocked = set(occupied) | set(mem.body) | {(head.x, head.y)}
    blocked.discard(dest)
    tiles, escapes = visible_pocket(dest, blocked)
    if escapes or not tiles:
        return not tiles
    max_deg = 0
    for x, y in tiles:
        deg = 0
        for direction in DIRS:
            kind, nxt = mem.transit(x, y, direction)
            if nxt in tiles:
                deg += 1
        if deg > max_deg:
            max_deg = deg
    if max_deg <= 2:
        return True
    return len(tiles) < ct.get_length()


def score_step(
    direction: Direction,
    dest: tuple[int, int],
    kind: str,
    occupied: set[tuple[int, int]],
    target: tuple[int, int] | None,
) -> int:
    score = opens = neighbor_open_count(dest, occupied)
    score *= 4
    if dest in mem.pearls:
        score += 800
    if dest not in mem.pearls and opens <= 1:
        score -= 70
    eta = mem.eta.get(dest, -1)
    if 0 <= eta <= 6:
        score += 40 - eta * 4
    if dest not in mem.seen:
        score += 18
    if kind == "unknown_portal":
        score -= 80
    elif kind == "portal":
        score -= 8
    if direction is OPPOSITE[ct.get_dir()]:
        score -= 25
    if target is not None:
        score -= torus_dist(dest, target) * 6
        score += torus_dist(mem.body[0], target) * 3
    for other in occupied:
        if torus_dist(dest, other) == 1:
            score -= 6
    return score


def legal_steps() -> list[tuple[Direction, tuple[int, int], str]]:
    """One-step moves that are not kelp and not a visible body."""
    here = ct.get_position()
    here_tile = ct.get_tile(here)
    options: list[tuple[Direction, tuple[int, int], str]] = []
    if here_tile is None:
        return options
    for direction in DIRS:
        edge = here_tile.get_edge(direction)
        if edge.get_edge_type() in (EdgeType.KELP, EdgeType.PORTAL):
            continue
        kind, dest = mem.transit(here.x, here.y, direction)
        if dest is None or kind in ("kelp", "portal", "unknown_portal"):
            continue
        dest_tile = ct.get_tile(here.add_dir(direction))
        if dest_tile is not None and dest_tile.get_dragon() is not None:
            continue
        if dest in mem.body:
            continue
        options.append((direction, dest, kind))
    reverse = OPPOSITE[ct.get_dir()]
    without_reverse = [item for item in options if item[0] is not reverse]
    if without_reverse:
        return without_reverse
    return options


def classify_steps(
    occupied: set[tuple[int, int]],
) -> tuple[list[tuple[Direction, tuple[int, int], str]], list[tuple[Direction, tuple[int, int], str]]]:
    open_opts: list[tuple[Direction, tuple[int, int], str]] = []
    dead_opts: list[tuple[Direction, tuple[int, int], str]] = []
    for item in legal_steps():
        if is_visible_dead_end(item[1], occupied):
            dead_opts.append(item)
        else:
            open_opts.append(item)
    return open_opts, dead_opts


def pick_from_options(
    options: list[tuple[Direction, tuple[int, int], str]],
    occupied: set[tuple[int, int]],
    preferred: list[Direction],
    target: tuple[int, int] | None,
    path: list[Direction] | None,
) -> list[Direction]:
    reverse = OPPOSITE[ct.get_dir()]
    if not options:
        return fallback_move()
    ranked: list[tuple[int, Direction]] = []
    for direction, dest, kind in options:
        ok, _, eats, unknown = simulate([direction], occupied)
        score = score_step(direction, dest, kind, occupied, target)
        if path and path[0] is direction:
            score += 40
        if direction in preferred:
            score += 12 - preferred.index(direction) * 3
        if eats:
            score += 200
        if unknown or not ok:
            score -= 80
        if direction is reverse:
            score -= 35
        ranked.append((score, direction))
    ranked.sort(key=lambda item: (-item[0], DIR_INDEX[item[1]]))
    return [ranked[0][1]]


def fallback_move() -> list[Direction]:
    here = ct.get_tile(ct.get_position())
    reverse = OPPOSITE[ct.get_dir()]
    ranked: list[tuple[int, Direction]] = []
    for direction in DIRS:
        penalty = 0
        if here is not None and here.get_edge(direction).get_edge_type() == EdgeType.KELP:
            penalty += 1000
        ahead = ct.get_tile(ct.get_position().add_dir(direction))
        if ahead is not None and ahead.get_dragon() is not None:
            penalty += 400
        if direction is reverse:
            penalty += 30
        ranked.append((penalty, direction))
    ranked.sort(key=lambda item: (item[0], DIR_INDEX[item[1]]))
    return [ranked[0][1]]


def split_to_small() -> bool:
    """One split: parent keeps 2, the tail becomes the child and can split again next."""
    if not ct.can_split(2):
        return False
    child = ct.get_length() - 2
    if not ct.can_split(child):
        child = 2
    if not ct.can_split(child):
        return False
    ct.do_split(child)
    if len(mem.body) > child:
        mem.body = mem.body[:-child]
        mem.last_head = mem.body[0]
    ct.set_indicator_string(f"split small {child}")
    return True


def unit_cap() -> int:
    area = mem.w * mem.h
    return 4 if area <= 200 else 5 if area <= 900 else 8


def want_split(occupied: set[tuple[int, int]], pearl_path: list[Direction] | None) -> bool:
    length = ct.get_length()
    units = ct.get_unit_count()
    cap = unit_cap()
    min_len = 6 if mem.w * mem.h <= 200 else 8
    if not ct.can_split(2) or length < min_len or units >= cap:
        return False
    if pearl_path is not None and len(pearl_path) == 1:
        return False
    safe = 0
    for direction in DIRS:
        ok, _, _, _ = simulate([direction], occupied)
        if ok:
            safe += 1
    if safe < 2:
        return False
    return True


def choose_target() -> tuple[set[tuple[int, int]], tuple[int, int] | None, str]:
    head = mem.body[0]
    visible_pearls = set()
    for tile in ct.get_tiles():
        if tile.has_pearl():
            visible_pearls.add((tile.get_position().x, tile.get_position().y))
    if visible_pearls:
        goal = min(visible_pearls, key=lambda p: (torus_dist(head, p), p))
        return visible_pearls, goal, "pearl"

    if mem.pearls:
        goal = nearest_goal(mem.pearls)
        return set(mem.pearls), goal, "memory-pearl"

    soon = {pos for pos, eta in mem.eta.items() if 0 <= eta <= 8}
    if soon:
        goal = nearest_goal(soon)
        return soon, goal, "soon"

    unseen_frontier = set()
    for x, y in mem.seen:
        for direction in DIRS:
            kind, dest = mem.transit(x, y, direction)
            if dest is None or kind in ("kelp", "unknown_portal"):
                continue
            if dest not in mem.seen:
                unseen_frontier.add(dest)
    if unseen_frontier:
        goal = nearest_goal(unseen_frontier)
        return unseen_frontier, goal, "explore"

    stale = None
    best_eta = 10**9
    for pos, eta in mem.eta.items():
        if eta < 0:
            continue
        if eta < best_eta:
            best_eta = eta
            stale = pos
    if stale is not None:
        return {stale}, stale, "respawn"
    return set(), None, "wander"


def emergency() -> None:
    moves = fallback_move()
    ct.make_move(moves[0])


def execute_turn() -> None:
    if mem.w == 0:
        mem.configure(*game.get_map_size())
    mem.observe()
    my_id = ct.get_id()
    occupied_others = set()
    for tile in ct.get_tiles():
        part = tile.get_dragon()
        if part is None or part.get_id() == my_id:
            continue
        occupied_others.add((tile.get_position().x, tile.get_position().y))

    head = ct.get_position()
    if not mem.body or mem.body[0] != (head.x, head.y):
        mem.body = [(head.x, head.y)] + [cell for cell in mem.body if cell != (head.x, head.y)]
        mem.body = mem.body[: ct.get_length()] or [(head.x, head.y)]
        mem.last_head = mem.body[0]

    goals, target, reason = choose_target()
    path = local_bfs(occupied_others, goals, allow_unseen=True) if goals else None
    preferred = heading_towards(target) if target is not None else list(DIRS)
    occupied = occupied_others | set(mem.body)
    open_opts, dead_opts = classify_steps(occupied)
    if path and not any(item[0] is path[0] for item in open_opts):
        path = None

    if open_opts:
        if want_split(occupied_others, path):
            child = 3 if ct.can_split(3) and ct.get_length() >= 10 else 2
            if ct.can_split(child):
                ct.do_split(child)
                if len(mem.body) > child:
                    mem.body = mem.body[:-child]
                    mem.last_head = mem.body[0]
                ct.set_indicator_string(f"split {child}")
                return
        moves = pick_from_options(open_opts, occupied_others, preferred, target, path)
    elif split_to_small():
        return
    else:
        reason = "dead-end"
        moves = pick_from_options(dead_opts, occupied_others, preferred, target, path)

    if not moves:
        moves = fallback_move()

    if target is not None:
        ct.draw_indicator_dot(Position(*target), 255, 210, 40)
    ct.set_indicator_string(f"{reason} {moves[0].value}")
    ct.make_move(moves[0])
    mem.apply_moves(moves, occupied_others)


def main() -> None:
    global ct, game
    ct, game = unswbc.init()
    while unswbc.update(ct, game):
        try:
            execute_turn()
        except Exception:
            try:
                emergency()
            except Exception:
                ct.make_move(Direction.NORTH)
        unswbc.end_turn()


if __name__ == "__main__":
    main()
