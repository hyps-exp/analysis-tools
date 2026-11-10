#!/usr/bin/env python3
"""Create CATCH HDPRM TDC calibration data.

This script creates the HDPRM TDC calibration output for CATCH.

Prerequisites:
    Before running this script, generate the required ROOT files.
    - CFT, BGO

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number and count.
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

from catch_module import parse_args
from catch_module import check_directory
from catch_module import program_catch
from catch_module import program_cft_layer

# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.run_number is None:
        print(f'error: incorrect arguments \n {args.program_name} <run_number>',
              file=sys.stderr)
        return 1

    run_number = args.run_number

    check_directory(run_number)

    start = datetime.now()
    t0 = time.perf_counter()

    program0 = "cft_hdprm_tdc"
    for layer in range(0, 8):
        program_cft_layer(program0, run_number, layer)

    program1 = "bgo_hdprm_tdc"
    program_catch(program1, run_number)
    program2 = "piid_hdprm_tdc"
    program_catch(program2, run_number)

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
