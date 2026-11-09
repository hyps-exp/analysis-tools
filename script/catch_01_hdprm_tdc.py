#!/usr/bin/env python3
"""Create CATCH HDPRM TDC calibration data.

This script creates the HDPRM TDC calibration output for CATCH.

Prerequisites:
    Before running this script, generate the required ROOT files.
    - CFT, BGO

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number.
    3. Check the generated PDF file after execution.

Arguments:
    run_number: Run number to be processed (required).

Examples: (run03597)
    make
    python3 script/catch_01_hdprm_tdc.py 3597
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
def hdprm_tdc_others(run_number: int, detector: str) -> None:
    script_dir = Path(__file__).resolve().parent
    work_dir = script_dir.parent / 'param-maker' / 'CATCH'

    program = f'{detector}_hdprm_tdc'

    try:
        subprocess.run(
            [f'./bin/{program}', str(run_number)],
             cwd=work_dir,
             check=True
        )
    except subprocess.CalledProcessError as e:
        print(f'error: {program} faild (exit code: {e.returncode})',
              file=sys.stderr)
        return 1
    except FileNotFoundError as e:
        print(f'error: executable or working directory no found: {e}',
              file=sys.stderr)
        return 1


# _____________________________________________________________________________
def hdprm_tdc_cft(run_number: int, layer: int) -> None:
    script_dir = Path(__file__).resolve().parent
    work_dir = script_dir.parent / 'param-maker' / 'CATCH'

    try:
        subprocess.run(
            [f'./bin/cft_hdprm_tdc', str(run_number), str(layer)],
             cwd=work_dir,
             check=True
        )
    except subprocess.CalledProcessError as e:
        print(f'error: cft_hdprm_tdc faild (exit code: {e.returncode})',
              file=sys.stderr)
        return 1
    except FileNotFoundError as e:
        print(f'error: executable or working directory no found: {e}',
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

    for layer in range(0, 8):
      hdprm_tdc_cft(run_number, layer)

    hdprm_tdc_others(run_number, "bgo")
    hdprm_tdc_others(run_number, "piid")
    # rewrite HDPRM
    # end

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
