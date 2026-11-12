#!/usr/bin/env python3
"""Create the histograms for the checks (SdcIn and SdcOut only).
 
Run the show_hist macros for SdcIn and SdcOut for the given run.
Use script/run_show_hist.py to also run the Hyps macro.
 
Arguments:
    run: run number to be processed (required).
 
Macros executed (in check_hist/):
    show_hist_sdcin.C("run<run>_SdcInTracking", <run>)
    show_hist_sdcout.C("run<run>_SdcOutTracking", <run>)
    (<run> in the names is the run number zero-padded to 5 digits,
    e.g. 07162)
 
The script stops at the first macro that fails (like 'set -e').
Requires the 'root' command in PATH.
 
Examples:
    python3 script/run_show_hist_sdc.py 7162
"""
 
import argparse
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Optional, Sequence, Union
 
CHECK_HIST_DIR = (Path(__file__).resolve().parent.parent / 'param-maker' / 'SDC' / 'check_hist')
 
MacroArg = Union[str, int]
 
 
# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run the SdcIn and SdcOut show_hist macros for one run.')
    # required
    parser.add_argument('run', type=int,
                        help='run number (e.g. 7162)')
    return parser.parse_args(argv)
 
 
# _____________________________________________________________________________
def format_arg(value: MacroArg) -> str:
    """Format a Python value as a ROOT macro argument."""
    if isinstance(value, str):
        return f'"{value}"'
    return str(value)
 
 
# _____________________________________________________________________________
def run_macro(macro: str, args: Sequence[MacroArg], cwd: Path) -> None:
    """Run `root -l -b -q 'macro(args...)'` in cwd.
 
    Raises subprocess.CalledProcessError if root fails.
    """
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
    if not CHECK_HIST_DIR.is_dir():
        print(f'error: directory not found: {CHECK_HIST_DIR}',
              file=sys.stderr)
        return 1
 
    run = args.run
    rid = f'{run:05d}'  # e.g. 07162
 
    start = datetime.now()
    t0 = time.perf_counter()
 
    try:
        run_macro('show_hist_sdcin.C',
                  [f'run{rid}_SdcInTracking', run], CHECK_HIST_DIR)
        run_macro('show_hist_sdcout.C',
                  [f'run{rid}_SdcOutTracking', run], CHECK_HIST_DIR)
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
