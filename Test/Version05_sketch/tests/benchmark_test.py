import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'tools'))
from benchmark_v05 import parse_run, aggregate_stats, first_difference, validate_profile

FRAME = '''round 1: bot 4 (team A) stdout:
MOVE E
SONAR 42
{stats}ENDTURN

team A wins after 2 rounds (by elimination) (0.1s)
'''
COUNTERS = {'tiles_scanned':147,'memory_tiles_scanned':49,'sense_tiles_scanned':49,
            'broadcast_tiles_scanned':49,'sonar_received':2,'sonar_accepted':1,'sonar_sent':1}
LINE = 'LOG V05_STATS round=1 id=4 ' + ' '.join(f'{k}={v}' for k,v in COUNTERS.items()) + '\n'

class BenchmarkTests(unittest.TestCase):
    def test_trace_excludes_only_telemetry(self):
        a=parse_run(FRAME.format(stats=''))
        b=parse_run(FRAME.format(stats=LINE))
        self.assertEqual(a['turns'],b['turns'])
        validate_profile(b,'A',COUNTERS)
        self.assertEqual(b['stats'][0]['counters']['tiles_scanned'],147)
    def test_detects_action_change(self):
        a=parse_run(FRAME.format(stats=''))
        b=parse_run(FRAME.format(stats='').replace('MOVE E','MOVE N'))
        self.assertIsNotNone(first_difference(a['turns'],b['turns']))
    def test_failed_bot_not_a_valid_win(self):
        text=FRAME.format(stats='').replace('team A wins','round 1: bot 4 (team A) died: no valid action\nteam A wins')
        self.assertEqual(len(parse_run(text)['failures']),1)
    def test_crash_and_timeout(self):
        for reason in ['exited with code 1','ran out of time','hit memory limit','hit points limit']:
            text=FRAME.format(stats='').replace('team A wins',f'round 1: bot 4 (team A) {reason}\nteam A wins')
            self.assertEqual(len(parse_run(text)['failures']),1)
    def test_missing_result(self):
        with self.assertRaises(ValueError): parse_run('round 1: bot 4 (team A) stdout:\nMOVE E\n')
    def test_identity_mismatch(self):
        with self.assertRaises(ValueError): parse_run(FRAME.format(stats=LINE.replace('id=4','id=5')))
    def test_duplicate_record(self):
        parsed=parse_run(FRAME.format(stats=LINE+LINE))
        with self.assertRaises(ValueError): validate_profile(parsed,'A',COUNTERS)
    def test_missing_coverage(self):
        with self.assertRaises(ValueError): validate_profile(parse_run(FRAME.format(stats='')),'A',COUNTERS)
    def test_bad_values(self):
        for bad in ['tiles_scanned=-1','tiles_scanned=1 tiles_scanned=2','bad']:
            with self.assertRaises(ValueError): parse_run(FRAME.format(stats='LOG V05_STATS round=1 id=4 '+bad+'\n'))
    def test_aggregation(self):
        rows=[{'counters':{'nodes':1}},{'counters':{'nodes':3}}]
        self.assertEqual(aggregate_stats(rows)['counters']['nodes'],{'total':4,'mean_per_turn':2.0,'max':3,'p95':3})

if __name__=='__main__': unittest.main()
