"""Black-box HTTP contract tests. All prices and amounts come from C++ responses."""
import json
import socket
import subprocess
import sys
import time
import unittest
import urllib.request
import urllib.error
from decimal import Decimal

class ApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0))
            cls.port = sock.getsockname()[1]
        cls.server = subprocess.Popen([sys.argv[1], str(cls.port)], stdout=subprocess.DEVNULL)
        for _ in range(100):
            try:
                cls.request('/health')
                return
            except OSError:
                time.sleep(.05)
        cls.server.terminate()
        raise RuntimeError('Backend did not start')

    @classmethod
    def tearDownClass(cls):
        cls.server.terminate()
        cls.server.wait(timeout=5)

    @classmethod
    def request(cls, path, body=None, expected=200):
        data = None if body is None else json.dumps(body).encode()
        req = urllib.request.Request(f'http://127.0.0.1:{cls.port}/api{path}', data=data,
                                     headers={'Content-Type':'application/json'})
        try:
            with urllib.request.urlopen(req, timeout=15) as response:
                status, payload = response.status, json.load(response)
        except urllib.error.HTTPError as response:
            status, payload = response.code, json.load(response)
        if status != expected:
            raise AssertionError((path,status,payload))
        if expected == 200:
            assert payload['ok']
            return payload['data']
        assert not payload['ok'] and payload['error']['message']
        return payload

    def setUp(self):
        self.request('/reset', {})

    def test_full_demo_acceptance(self):
        self.assertEqual(len(self.request('/scenarios')),4)
        graph = self.request('/graph')
        self.assertEqual(len(graph['edges']),len(graph['pools'])*2)
        state = self.request('/detect',{'capital':'1000','algorithm':'dfs','maxHops':4})
        best = state['opportunities'][0]
        self.assertGreater(Decimal(best['net']),0)
        self.assertEqual(self.request('/opportunities/'+best['id'])['edges'],best['edges'])
        self.assertGreater(len(self.request('/algorithm/trace')['steps']),0)
        trade = self.request('/simulate-trade',{'id':best['id']})
        self.assertEqual(trade['net'],best['net'])
        self.assertEqual(Decimal(trade['output'])-Decimal(trade['input'])-Decimal(trade['cost']),Decimal(trade['net']))
        self.assertEqual(len(self.request('/trades')),1)
        self.request('/simulate-trade',{'id':best['id']},400)
        reset = self.request('/reset',{})
        self.assertEqual(reset['opportunities'][0]['net'],best['net'])
        self.assertEqual(reset['trades'],[])

    def test_scenarios_edits_and_replay(self):
        for scenario in ['balanced','fee-shock']:
            self.assertEqual(self.request('/scenario/select',{'scenario':scenario})['opportunities'],[])
        self.assertGreater(len(self.request('/scenario/select',{'scenario':'competing'})['opportunities']),1)
        self.request('/reset',{})
        p = next(p for p in self.request('/graph')['pools'] if p['id']=='gamma-jup-usdc')
        p['reserveB']='100000'
        self.assertEqual(self.request('/pools/update',p)['opportunities'],[])
        self.assertEqual(self.request('/replay',{'index':0})['opportunities'],[])
        state=self.request('/replay',{'index':1})
        self.assertGreater(len(state['opportunities']),0)
        self.request('/simulate-trade',{'id':state['opportunities'][0]['id']},400)
        self.assertEqual(self.request('/replay',{'index':3})['opportunities'],[])

    def test_bellman_ford_and_benchmarks(self):
        state=self.request('/detect',{'algorithm':'bellman-ford'})
        self.assertGreater(len(state['opportunities']),0)
        self.assertTrue(any(s['action']=='negative-cycle' for s in self.request('/algorithm/trace')['steps']))
        measurements=self.request('/benchmarks')
        self.assertEqual(len(measurements),8)
        self.assertTrue(all(b['milliseconds']>0 for b in measurements))

    def test_errors_are_structured_and_atomic(self):
        before=self.request('/state')
        for path,body in [('/scenario/select',{'scenario':'../bad'}),('/detect',{'capital':'-1'}),('/detect',{'capital':100}),('/detect',{'maxHops':7}),('/detect',{'maxHops':3.5}),('/replay',{'index':1.5}),('/pools/update',{'id':'missing','reserveA':'1','reserveB':'1','feeBps':30}),('/live/connect',{})]:
            self.request(path,body,400)
        self.assertEqual(before,self.request('/state'))
        self.request('/unknown',expected=404)
        self.request('/scenario/select',{'scenario':'balanced'})
        candidate=self.request('/state')['candidates'][0]
        self.request('/simulate-trade',{'id':candidate['id']},400)
        self.assertEqual(self.request('/trades'),[])

if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]],verbosity=2)
