import helper as unswbc

import decide


def main() -> None:
    ct, game = unswbc.init()
    while unswbc.update(ct, game):
        try:
            decide.act(ct, game)
        except Exception:
            try:
                import sense
                ct.make_move(sense.last_resort(ct))
            except Exception:
                ct.make_move(unswbc.Direction.NORTH)
        unswbc.end_turn()


if __name__ == "__main__":
    main()
