import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'tools'))
from observed_run import Observer, decode_turn


def block(id, round, head):
    x,y=head
    cells='\n'.join(f'{(x+dx)%20} {(y+dy)%20} 0 -1' for dx in range(-3,4) for dy in range(-3,4))
    return f'ROUND {round}\nDIR E\nLENGTH 3\nUNIT_COUNT 2\nNUM_MSGS 1\n123\n{cells}\nDRAGON_BODIES 1\nA {id} {x} {y} E 1\n'.encode()


def spawn(observer,id):
    observer.spawn(id,f'ID {id}\nTEAM A\nMAP 20 20\nUNIT_LIMIT 64\n'.encode())


class ObserverTests(unittest.TestCase):
    def test_death_and_growth(self):
        o=Observer();spawn(o,3)
        o.observe(3,block(3,1,(0,0)))
        o.observe(3,block(3,2,(1,0)).replace(b'LENGTH 3',b'LENGTH 4'))
        self.assertEqual(o.surviving_growth['A'],1)
        o.observe(3,block(3,3,(2,0)).replace(b'LENGTH 3',b'LENGTH 2'))
        self.assertEqual(o.surviving_growth['A'],1)
        o.death(3,3,'H')
        self.assertNotIn(3,o.latest)
        self.assertEqual(o.deaths[0]['last_observed_length'],2)
        self.assertEqual(o.deaths[0]['reason'],'H')

    def test_decode(self):
        t=decode_turn(3,block(3,7,(0,0)))
        self.assertEqual((t['round'],t['length'],t['head']),(7,3,(0,0)))
        self.assertEqual(len(set(t['cells'])),49)
        self.assertIn((19,19),t['cells'])
    def test_missing_head(self):
        with self.assertRaises(ValueError):decode_turn(4,block(3,7,(0,0)))
    def test_coverage_overlap_and_spacing(self):
        o=Observer();spawn(o,3);spawn(o,5)
        o.observe(3,block(3,0,(0,0)));o.observe(5,block(5,0,(1,0)));o.finish_round()
        a=next(r for r in o.rounds if r['team']=='A')
        self.assertEqual(a['coverage'],56)
        self.assertAlmostEqual(a['vision_overlap'],1-56/98)
        self.assertEqual(a['nearest_ally_distance'],1)
        self.assertEqual(a['observed_team_length'],6)
    def test_cycle_and_gap(self):
        o=Observer();spawn(o,3)
        for r,p in enumerate([(0,0),(1,0),(1,1),(0,1),(0,0),(1,0)]):o.observe(3,block(3,r,p))
        self.assertEqual(o.oscillations['A'],1)
        o.observe(3,block(3,9,(1,1)))
        self.assertEqual(len(o.history[3]),1)
    def test_monotonic_team_coverage(self):
        o=Observer();spawn(o,3)
        for r in range(5):o.observe(3,block(3,r,(r,0)))
        o.finish_round()
        values=[r['coverage'] for r in o.rounds if r['team']=='A']
        self.assertEqual(values,sorted(values))
        self.assertEqual(len(values),5)


if __name__=='__main__':unittest.main()
