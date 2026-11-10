#!/usr/bin/env python3
"""Correct the CFT position.

This script performs position correction for CFT PHI layer.

Prerequisites:
    Before running this script, generate the required ROOT file.
    - CFTTrackingCosmic

Execution:
    1. Run make to build the required programs before executing this script.
    2. Execute this script with the target run number.
    3. Check the generated PDF file after execution.

Arguments:
    run_number: Run number to be processed (required).

Examples: (run03597)
    make
    python3 script/catch_03_01_phi_cft_poscor.py 3597
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
from catch_module import program_count

# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.count is None:
        print(f'error: incorrect arguments \n {args.program_name} <run_number> <count>',
              file=sys.stderr)
        return 1

    run_number = args.run_number
    count = args.count

    check_directory(run_number)

    start = datetime.now()
    t0 = time.perf_counter()

    program0 = "calPhiResidual_16layer"
    program_count(program0, run_number, count)
    program1 = "MergePhiPosCor"
    program_count(program1, run_number, count)

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
