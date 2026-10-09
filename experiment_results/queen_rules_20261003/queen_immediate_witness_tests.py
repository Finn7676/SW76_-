#!/usr/bin/env python3
"""Focused bounded-projector checks; no bot changes or competitive games."""
from queen_immediate_witness import project


def main():
    visible = {(x, y) for x in range(8) for y in range(8)}
    body = [(2, 2), (1, 2)]
    args = (set(), set(), visible, {}, 8, 8)
    no_pearl = project(body, (1, 1), *args)
    assert no_pearl["reason"] == "payment" and len(no_pearl["trace"]) == 1
    too_late = project(body, (1, 1), set(), {(4, 2)}, visible, {}, 8, 8)
    assert too_late["reason"] == "payment"
    first_pearl = project(body, (1, 1), set(), {(3, 2)}, visible, {}, 8, 8)
    assert first_pearl["survives"] and len(first_pearl["body"]) == 2
    assert [step["paid"] for step in first_pearl["trace"]] == [False, True]
    wall = project(body, (1,), set(), set(), visible, {((2, 2), 1): (1, -1)}, 8, 8)
    assert wall["known"] and wall["reason"] == "wall"
    portal = project(body, (1,), set(), set(), visible, {((2, 2), 1): (2, 0)}, 8, 8)
    assert not portal["known"]
    collision = project([(2, 2), (2, 3), (3, 3), (4, 3)], (1, 2),
                        set(), set(), visible, {}, 8, 8)
    assert collision["reason"] == "self" and len(collision["trace"]) == 1
    hidden = project(body, (0,), set(), set(), {(2, 2), (1, 2)}, {}, 8, 8)
    assert not hidden["known"]
    print("passed: payment-before-pickup, pearl-growth, fixed-free-steps, wall-known, portal-unknown, collision-before-tail, hidden-unknown")


if __name__ == "__main__":
    main()
