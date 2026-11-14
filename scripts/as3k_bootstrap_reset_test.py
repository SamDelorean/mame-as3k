#!/usr/bin/env python3
"""Reuse the same payload/UART acceptance host for normal -> visual bootstrap -> normal."""
from pathlib import Path
import argparse,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--output-directory',required=True);p.add_argument('--fixture',required=True);p.add_argument('--rompath',required=True);p.add_argument('--payload',required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1]
raise SystemExit(subprocess.call([sys.executable,str(root/'scripts/as3k_bootstrap_test.py'),'--system','asma3ksw','--rompath',a.rompath,'--payload',a.payload,'--output-directory',a.output_directory,'--cycle-fixture',a.fixture]))
