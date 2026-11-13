#!/usr/bin/env python3
"""Perform CFT energy calibration for a specified run.

This script performs CFT energy calibration.

Prerequisites:
    1. Run GammaCATCHCFT usr to create the required ROOT file.
    2. Run make before executing this script.

Arguments:
    run_number: Run number used for BGO energy calibration.

After execution:
    Check the generated PDF file to confirm the calibration results.

Examples: run07035
    python3 script/catch_08_cft_enecalib.py 7035
"""

import argparse
import sys
import time
from datetime import datetime
from typing import Optional

import subprocess
from pathlib import Path

from catch_module import parse_args
from catch_module import check_directory
from catch_module import program_calib
from catch_module import setup_simulation
from catch_module import program_simulation

# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.run_number is None:
        print(f'error: incorrect arguments \n {args.program_name} <run_number>',
              file=sys.stderr)
        return 1

    run_number = args.run_number
    count = args.count

    check_directory(run_number)

    start = datetime.now()
    t0 = time.perf_counter()

    usr = 'GammaCATCHCFT'
    output_name = f'GammaCATCHCFT_{start_num:05}_{end_num:05}.root'
    hadd_root(usr, run_number, output_name)

    program0 = 'makeCalibtxt'
    entries = program_calib(program0, run_number, start_number, end_number, 1)

    setup_simulation(run_number, entries, 1)
    program_simulation(run_number, 0)
    program_simulation(run_number, 1)

    program1 = 'makeCalibroot'
    dummy = program_calib(program1, run_number, run_number+100000, start_number, end_number)

    program2 = 'cft_calib'
    dummy = program_calib(program2, run_number, run_number+100000, start_number, end_number)


    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
