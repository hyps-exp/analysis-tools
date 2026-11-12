#!/usr/bin/env python3
"""Read the old DCTDC T0 file and write the new one.
 
Run the read_write_T0 macro for the given run.
 
Arguments:
    run: run number to be processed (required).
 
Macro executed (in make_param/):
    read_write_T0.C("DCTDC.old", "DCTDC.new", <run>)
 
Examples:
    python3 script/run_read_write_T0.py 7162
"""
 
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional
 
MAKE_PARAM_DIR = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' / 'make_param')

CALIB_NAME_BEFORE = 'DCTdcCalib_hyps_common_run{run:05d}'
CALIB_NAME_AFTER = 'DCTdcCalib_hyps_update'
 
# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run read_write_T0.C for one run.')
    # required
    parser.add_argument('run', type=int,
                        help='run number (e.g. 7162)')
    return parser.parse_args(argv)
 
 
# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    if args.run < 0:
        print(f'error: run must be 0 or more (got {args.run})',
              file=sys.stderr)
        return 1
    if not MAKE_PARAM_DIR.is_dir():
        print(f'error: directory not found: {MAKE_PARAM_DIR}',
              file=sys.stderr)
        return 1
 
    before = CALIB_NAME_BEFORE.format(run=args.run)
    expr = f'read_write_T0.C("{before}", "{CALIB_NAME_AFTER}", {args.run})'
    cmd = ['root', '-l', '-b', '-q', expr]
    
    print(f'[{MAKE_PARAM_DIR.name}] root -l -b -q \'{expr}\'', flush=True)
    try:
        subprocess.run(cmd, cwd=MAKE_PARAM_DIR, check=True)
    except subprocess.CalledProcessError as e:
        print(f'error: command failed (exit code {e.returncode})',
              file=sys.stderr)
        return e.returncode
    except FileNotFoundError:
        print("error: 'root' command not found in PATH", file=sys.stderr)
        return 1
    return 0
 
 
# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
 
