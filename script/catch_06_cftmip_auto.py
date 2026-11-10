#!/usr/bin/env python3
"""Automatically determine the MIP position for CFT.

This script automatically determines the MIP position for CFT.

Prerequisites:
    Before running this script, generate the required ROOT file.
    - CATCH

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number.
    3. Check the generated PDF file.
    4. Run the corresponding manual script to complete the calibration.

Arguments:
    run_number: Run number to be processed (required).

Examples:
    make
    python3 script/catch_06_cftmip_auto.py 3597
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
from catch_module import hadd_root
from catch_module import program_catch

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

    usr = "MIPCFT"
    output_name = f"cft_proton_mip_{run_number:05}.root"
    hadd_root(usr, run_number, output_name)

    program0 = "cft_lowmip_load"
    program_catch(program0, run_number)

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
