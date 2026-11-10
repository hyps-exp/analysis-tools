#!/usr/bin/env python3
"""Create CFT pedestal correction data.

This script performs pedestal correction for CFT.

Prerequisites:
    Before running this script, generate the required ROOT file.
    - CFT

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number.
    3. Check the generated PDF file after execution.

Arguments:
    run_number: Run number to be processed (required).

Examples: (run03597)
    make
    python3 script/catch_02_cft_pedcor.py 3597
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

    program0 = 'makePedestalCorrection'
    program1 = 'makePedestalCorrection2'
    for layer in range(0, 8):
        program_cft_layer(program0, run_number, layer)
    for layer in range(0, 8):
        program_cft_layer(program1, run_number, layer)

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
