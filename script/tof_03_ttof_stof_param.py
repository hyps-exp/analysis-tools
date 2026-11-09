#!/usr/bin/env python3
"""Create TOF absolute timing (tTofCalc - stof) calibration data.

This script runs the TOF ttof-stof calibration ROOT macro for a given run.
It reads the TOF TDC param written by script/tof_01_tdc_param.py, so run
that script first.

Prerequisites:
    Before running this script, generate the required files.
    - run{run:05d}_HypsTracking.root
    - output/txt/run{run:05d}/TOF_TDC_param_run{run:05d}.txt
      (from script/tof_01_tdc_param.py)

Execution:
    1. Execute script/tof_01_tdc_param.py with the target run number.
    2. Execute this script with the target run number.
    3. Check the generated PDF file after execution.

Arguments:
    run_number: Run number to be processed (required).

Examples: (run07013)
    python3 script/tof_01_tdc_param.py 7013
    python3 script/tof_03_ttof_stof_param.py 7013
"""

import argparse
import sys
import time
from datetime import datetime
from typing import Optional

import subprocess
from pathlib import Path

# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()

    # required
    parser.add_argument('run_number', type=int,
                        help='run number to be processed')

    return parser.parse_args(argv)

# _____________________________________________________________________________
def make_tof_ttof_stof_param(run_number: int) -> None:
    script_dir = Path(__file__).resolve().parent
    base_dir = script_dir.parent
    macro = base_dir / 'param-maker' / 'TOF' / 'make_tof_ttof-stof_param.C'
    run_dir = base_dir / 'root' / f'run{run_number:05d}'
    root_file = run_dir / f'run{run_number:05d}_HypsTracking.root'

    try:
        subprocess.run(
            ['root', '-l', '-b', '-q',
             f'{macro}("{root_file}")'],
             cwd=base_dir,
             check=True
        )
    except subprocess.CalledProcessError as e:
        print(f'error: make_tof_ttof_stof_param failed (exit code: {e.returncode})',
              file=sys.stderr)
        return 1
    except FileNotFoundError as e:
        print(f'error: root or macro not found: {e}',
              file=sys.stderr)
        return 1


# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.run_number <= 0:
        print(f'error: invalid run number ({args.run_number})',
              file=sys.stderr)
        return 1

    run_number = args.run_number

    start = datetime.now()
    t0 = time.perf_counter()

    make_tof_ttof_stof_param(run_number)

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
