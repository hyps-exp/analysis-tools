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
    if argv is None:
        argv = sys.argv[1:]

    parser = argparse.ArgumentParser(prog=Path(sys.argv[0]).name)

    # required
    parser.add_argument('run_number', type=int,
                        help='run number to be processed')

    parser.add_argument('count', type=int, nargs='?',
                        default=None,
                        help='number of times the program was run (need script:catch_03_01..., catch_05...)')

    args = parser.parse_args(argv)
    args.program_name = parser.prog

    # return parser.parse_args(argv)
    return args

# _____________________________________________________________________________
def check_directory(run_number: int) -> None:
    script_dir = Path(__file__).resolve().parent
    output_pdf_dir = script_dir.parent / 'output' / 'pdf' / f'run{run_number:05}'
    output_txt_dir = script_dir.parent / 'output' / 'txt' / f'run{run_number:05}'

    output_pdf_dir.mkdir(parents=True, exist_ok=True)
    output_txt_dir.mkdir(parents=True, exist_ok=True)

# _____________________________________________________________________________
def hadd_root(usr: str, run_number: int, output_name: str) -> None:
    script_dir = Path(__file__).resolve().parent
    root_dir = script_dir.parent / 'rootfile' / f'run{run_number:05}'

    root_files = sorted(
        file for file in root_dir.glob('*.root')
        if usr in file.name
    )

    if not root_files:
        raise FileNotFoundError(
            f'No ROOT files containing "{usr}" were found.'
        )

    # Execute hadd
    output_file = root_dir / output_name

    subprocess.run(
        ['hadd', '-f', str(output_file)]
        + [str(file) for file in root_files],
        check=True
    )


# _____________________________________________________________________________
def program_catch(program: str, run_number: int) -> None:
    script_dir = Path(__file__).resolve().parent
    work_dir = script_dir.parent / 'param-maker' / 'CATCH'

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
def program_count(program: str, run_number: int, count:int) -> None:
    script_dir = Path(__file__).resolve().parent
    work_dir = script_dir.parent / 'param-maker' / 'CATCH'

    try:
        subprocess.run(
            [f'./bin/{program}', str(run_number), str(count)],
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
def program_cft_layer(program: str, run_number: int, layer: int) -> None:
    script_dir = Path(__file__).resolve().parent
    work_dir = script_dir.parent / 'param-maker' / 'CATCH'

    try:
        subprocess.run(
            [f'./bin/{program}', str(run_number), str(layer)],
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
