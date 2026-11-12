#!/usr/bin/env python3
"""Create DCTDC T0 calibration data.
 
Run the make_dctdc_t0 macro for the given run.
 
Arguments:
    run: run number to be processed (required).
 
Macro executed (in make_param/):
    make_dctdc_t0.C("<run>_In", "<run>_Out", <run>, 0, "<name>")
    (<run> in the names is the run number zero-padded to 5 digits,
    e.g. 07162. <name> is CALIB_NAME_BEFORE for runs below RUN_THRESHOLD,
    and CALIB_NAME_FROM for runs at or above it.)
 
Examples:
    python3 script/run_make_dctdc_t0.py 7162
"""
 
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional
 
MAKE_PARAM_DIR = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' / 'make_param')
 
RUN_THRESHOLD = 3135
CALIB_NAME_BEFORE = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' / 'DCTdcCalib_hyps_default_ver1')
CALIB_NAME_FROM = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' /'DCTdcCalib_hyps_default_ver2')
 
 
# _____________________________________________________________________________
def select_calib_name(run: int) -> str:
    if run < RUN_THRESHOLD:
        return CALIB_NAME_BEFORE
    return CALIB_NAME_FROM
 
 
# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run make_dctdc_t0.C for one run.')
    # required
    parser.add_argument('run', type=int,
                        help='run number')
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
 
    run = args.run
    rid = f'{run:05d}'
    
    output_dir = ( Path(__file__).resolve().parent.parent / 'output' / f'run{rid}')
    output_dir.mkdir(parents=True, exist_ok=True)
    
    expr = (f'make_dctdc_t0.C("{rid}_In", "{rid}_Out", {run}, 0, '
            f'"{select_calib_name(run)}")')
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
