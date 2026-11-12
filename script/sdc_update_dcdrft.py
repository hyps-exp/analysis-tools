#!/usr/bin/env python3
"""Update SDC XT calibration data.

Run the SDC to update XT calibration macros for the given run.

Prerequisites:
    Generate the required ROOT files beforehand (initial dcdrft generate).
    - run<run>_SdcInTracking.root
    - run<run>_SdcOutTracking.root
    (<run> is the run number zero-padded to 5 digits, e.g. 07162)

Execution:
    1. Run this script with the target run number.
    2. Check the generated PDF files after execution.

Arguments:
    run: run number to be processed (required).

Macros executed (in make_param/):
    update_XT_SDCIn.C("<run>_SdcInTracking", <run>)
    update_XT_SDCOut.C("<run>_SdcOutTracking", <run>)

Examples:
    python3 script/sdc_update_sdcdrft.py 7162
"""
 
import argparse
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Optional, Sequence, Union

MAKE_PARAM_DIR = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' / 'make_param')
 
MacroArg = Union[str, int]
 
 
# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run update_XT_SDCIn.C and update_XT_SDCOut.C.')
    parser.add_argument('run', type=int,
                        help='run number')
    return parser.parse_args(argv)
 
 
# _____________________________________________________________________________
def format_arg(value: MacroArg) -> str:
    if isinstance(value, str):
        return f'"{value}"'
    return str(value)
 
 
# _____________________________________________________________________________
def run_macro(macro: str, args: Sequence[MacroArg], cwd: Path) -> None:
    expr = f'{macro}({", ".join(format_arg(a) for a in args)})'
    cmd = ['root', '-l', '-b', '-q', expr]
    print(f'[{cwd.name}] root -l -b -q \'{expr}\'', flush=True)
    subprocess.run(cmd, cwd=cwd, check=True)
 
 
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
    rid = f'{run:05d}'  # e.g. 07162
 
    start = datetime.now()
    t0 = time.perf_counter()
 
    try:
        run_macro('update_XT_SdcIn.C',
                  [f'{rid}_SdcInTracking', run], MAKE_PARAM_DIR)
        run_macro('update_XT_SdcOut.C',
                  [f'{rid}_SdcOutTracking', run], MAKE_PARAM_DIR)
    except subprocess.CalledProcessError as e:
        print(f'error: command failed (exit code {e.returncode}): '
              f'{" ".join(e.cmd)}', file=sys.stderr)
        return e.returncode
    except FileNotFoundError:
        print("error: 'root' command not found in PATH", file=sys.stderr)
        return 1
 
    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0
 
 
# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
 
