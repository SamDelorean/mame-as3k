#!/usr/bin/env python3
"""Run deterministic Send+Print stimuli against a private derived Gate 1A ROM."""
import os, subprocess, sys, tempfile
from pathlib import Path
from as2k_test_irless_contract import classify

def main():
    root=Path(__file__).resolve().parents[1]
    private=Path.home()/'Projects/alphasmart/private'
    rom=Path(os.environ.get('AS2K_ROM',private/'roms/as2k/AS2000_v3.1.4_gate1a.bin'))
    binary=Path(os.environ.get('AS2K_DIAG_BIN',root/'as2kdiag')).resolve()
    if not rom.is_file() or not binary.is_file():
        print('PRIVATE_INPUT missing ROM or emulator binary'); return 20
    state=Path(os.environ.get('XDG_STATE_HOME',Path.home()/'.local/state'))/'as2k-emulator-automation/irless-full'
    state.mkdir(parents=True,exist_ok=True); run=Path(tempfile.mkdtemp(prefix='run.',dir=state))
    romdir=run/'roms'; romdir.mkdir(); setdir=romdir/'asma2k'; setdir.mkdir()
    # MAME expects the AS2000 program image under its canonical filename.
    (setdir/'alphasmart__2000__v3.1.4__h4.zpsd211r.plcc44.bin').symlink_to(rom.resolve())
    (setdir/'dictrom__v1.stm_m27c1001-1501.plcc32.bin').symlink_to(private/'roms/as2k/dictrom__v1.stm_m27c1001-1501.plcc32.bin')
    rompath=';'.join(map(str,(romdir,private/'cgrom',private/'roms/ks0066')))
    cmd=[str(binary),'asma2k','-rompath',rompath,'-autoboot_script',str(root/'scripts/as2k_irless_full_probe.lua'),
         '-video','none','-sound','none','-skip_gameinfo','-nothrottle','-log','-seconds_to_run','30']
    try:
        with (run/'console.log').open('w') as out:
            result=subprocess.run(cmd,cwd=run,stdout=out,stderr=subprocess.STDOUT,timeout=40,check=False)
        log=(run/'error.log').read_text(errors='replace')
    except (OSError,subprocess.TimeoutExpired) as exc:
        print(f'BLOCKED infrastructure {exc}'); return 20
    required=('AS2K_IRLESS_FULL START','AS2K_IRLESS_FULL EDITOR_READY','AS2K_IRLESS_FULL PRESS Send',
              'AS2K_IRLESS_FULL RELEASE Send','AS2K_IRLESS_FULL PRESS Print','AS2K_IRLESS_FULL RELEASE Print','AS2K_IRLESS_FULL COMPLETE')
    pos=0
    for marker in required:
        found=log.find(marker,pos)
        if found < 0: print(f'EVIDENCE missing_or_unordered={marker} log={run/"error.log"}'); return 30
        pos=found+len(marker)
    rc,report=classify(log,result.returncode,'full')
    for line in report: print(line)
    print(('IRLESS_FULL_PASS' if rc==0 else 'IRLESS_FULL_FAIL')+f' log={run/"error.log"}')
    return rc

if __name__=='__main__': sys.exit(main())
