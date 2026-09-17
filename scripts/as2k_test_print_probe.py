#!/usr/bin/env python3
"""Stock v3.1.4 normal Print routing probe; no patched-firmware claim."""
import hashlib, os, subprocess, sys, tempfile, unittest
from pathlib import Path
MARKERS = ('AS2K_PRINT_PROBE FIELD port=:COL.9 mask=10 name=Print',
           'AS2K_PRINT_PROBE READY', 'AS2K_PRINT_PROBE PRESS after_typing_idle')
END = ('AS2K_PRINT_PROBE RELEASE', 'AS2K_PRINT_PROBE COMPLETE', 'AS2K_GATE1A PROBE_STOP instructions=')
def classify(log, rc):
    if rc: return 20, f'EMULATOR_RUNTIME rc={rc}'
    pos=0
    for marker in MARKERS:
        found=log.find(marker,pos)
        if found < 0: return 30, f'EVIDENCE missing_or_unordered={marker}'
        pos=found+len(marker)
    routes=[(log.find(x,pos),x) for x in ('AS2K_GATE1A PRINT_DETACH_962D PC=962D','AS2K_GATE1A PRINT_DETACH_9804 PC=9804','AS2K_GATE1A PRINT_FALLBACK PC=ABC9','AS2K_GATE1A FAIL_IR_PRINT_D437 PC=D437')]
    observed=[(i,x) for i,x in routes if i >= 0]
    for marker in END:
        found=log.find(marker,pos)
        if found < 0: return 30, f'EVIDENCE missing_or_unordered={marker}'
        pos=found+len(marker)
    if not observed: return 10, 'stock_print_route_unobserved'
    observed.sort()
    names=[x for _,x in observed]
    return 0, 'VALIDATED stock_Print_matrix route=' + ' -> '.join(x.split(' PC=')[0].split()[-1] for x in names)
def runtime():
    root=Path(__file__).resolve().parents[1]; private=Path.home()/'Projects/alphasmart/private'
    rom=Path(os.environ.get('AS2K_ROM',private/'roms/as2k/AS2000_v3.1.4.bin')); binary=Path(os.environ.get('AS2K_DIAG_BIN',root/'as2kdiag')).resolve()
    if hashlib.sha1(rom.read_bytes()).hexdigest()!='e0b777dc68c671c31ba808e214fb9d2573b9a853': print('PRIVATE_INPUT wrong_stock_rom_sha1'); return 20
    state=Path(os.environ.get('XDG_STATE_HOME',Path.home()/'.local/state'))/'as2k-emulator-automation/print-probe'; state.mkdir(parents=True,exist_ok=True); run=Path(tempfile.mkdtemp(prefix='run.',dir=state))
    rompath=';'.join(map(str,(rom.resolve().parent,private/'cgrom',private/'roms/ks0066')))
    cmd=[str(binary),'asma2k','-rompath',rompath,'-autoboot_script',str(root/'scripts/as2k_print_probe.lua'),'-video','none','-sound','none','-skip_gameinfo','-nothrottle','-log','-seconds_to_run','20']
    with (run/'console.log').open('w') as out: result=subprocess.run(cmd,cwd=run,stdout=out,stderr=subprocess.STDOUT,timeout=30,check=False)
    rc,detail=classify((run/'error.log').read_text(errors='replace'),result.returncode); print(f'{detail} log={run/"error.log"}'); return rc
class EvidenceTests(unittest.TestCase):
    def fixture(self,route='AS2K_GATE1A FAIL_IR_PRINT_D437 PC=D437'): return '\n'.join(MARKERS+(route,)+END)
    def test_stock_route_observed(self): self.assertEqual(classify(self.fixture(),0)[0],0)
    def test_fallback_route_observed(self): self.assertEqual(classify(self.fixture('AS2K_GATE1A PRINT_DETACH_962D PC=962D\nAS2K_GATE1A PRINT_FALLBACK PC=ABC9'),0)[0],0)
    def test_missing_route_rejected(self): self.assertEqual(classify('\n'.join(MARKERS+END),0)[0],10)
    def test_route_before_press_not_evidence(self): self.assertEqual(classify('AS2K_GATE1A FAIL_IR_PRINT_D437 PC=D437\n'+'\n'.join(MARKERS+END),0)[0],10)
    def test_runtime_failure(self): self.assertEqual(classify(self.fixture(),1)[0],20)
if __name__=='__main__':
    if sys.argv[1:]==['--runtime']: sys.exit(runtime())
    unittest.main()
