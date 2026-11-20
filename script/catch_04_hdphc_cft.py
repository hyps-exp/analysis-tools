#!/usr/bin/env python3
"""Perform pulse-height correction for CFT.

This script performs pulse-height correction for CFT.

Prerequisites:
    Before running this script, generate the required ROOT file.
    - CFTTracking

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number.
    3. Check the generated PDF file after execution.

Arguments:
    run_number: Run number to be processed (required).

Examples:
    make
    python3 script/catch_04_hdphc_cft.py 3597
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
from catch_module import program_catch
from catch_module import program_cft_layer

from calib_run_map import find_entry, CalibRunMapError

# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.run_number is None:
        print(f'error: incorrect arguments \n {args.program_name} <run_number>',
              file=sys.stderr)
        return 1

    run_number = args.run_number

    try:
        entry = find_entry('CATCH', 'HDPHC', run_number)
    except CalibRunMapError as e:
        sys.exit(f'error: {e}')

    filename_run = entry.filename_run

    check_directory(filename_run)

    start = datetime.now()
    t0 = time.perf_counter()

    program0 = "makePHC"
    program_catch(program0, filename_run)

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
