#!/usr/bin/env python3
"""
Write calibration parameters 

Change history:
  2026-11-19  Akao Ryogo  First version
"""
import argparse
import sys
from pathlib import Path
from typing import Optional

sys.path.insert(0, str(Path(__file__).resolve().parent))
from calib_run_map import find_entry, CalibRunMapError

SOURCES = {
    'HDPRM': {
        'RF': {
            'toml': 'RF',
            'files': ['run{run:05d}_RF_HDPRM_TDC.txt'],
        },
        'T0': {
            'toml': 'T0',
            'files': ['run{run:05d}_T0_HDPRM_ADC_TDC.txt'],
        },
        'TOF_raw': {
            'toml': 'TOF',
            'files': ['run{run:05d}_TOF_HDPRM_ADC.txt',
                      'run{run:05d}_TOF_HDPRM_TDC.txt'],
        },
        'TOF_col': {
            'toml': 'TOF',
            'files': ['run{run:05d}_TOF_HDPRM_ttof-stof.txt'],
        },
    },
}

Key = tuple[int, int, int, int, int]


# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()

    # required
    parser.add_argument('run_number', type=int,
                        help='run number of the param file '
                             '(= filename_run)')
    parser.add_argument('param_type',
                        help='parameter type to write '
                             '(implemented: ' + ', '.join(SOURCES) + ')')
    parser.add_argument('detector',
                        help='detector whose calibration to write '
                             '(RF, T0, TOF_raw, TOF_col)')
    # optional
    parser.add_argument('input_run', type=int, nargs='?', default=None,
                        help='run that was analyzed to make the '
                             'calibration (= input_runs); looked up in '
                             'calib-run-map when omitted')

    return parser.parse_args(argv)

# _____________________________________________________________________________
def line_key(line: str) -> Optional[Key]:
    """Return (CId, PlId, SegId, AorT, UorD) of a data line.

    Return None for a comment or blank line, or one with fewer than 5
    whitespace-separated numeric fields.
    """
    fields = line.split('#', 1)[0].split()
    if len(fields) < 5:
        return None
    try:
        return tuple(int(f) for f in fields[:5])
    except ValueError:
        return None

# _____________________________________________________________________________
def source_entry(param_type: str, detector: str) -> dict:
    """Return {'toml': <calib-run-map detector>, 'files': [...]} for
    param_type/detector, or exit with a clear error if that
    combination is not implemented."""
    detectors = SOURCES.get(param_type)
    if detectors is None:
        sys.exit(f'error: {param_type!r} is not implemented yet '
                  f'(implemented: {", ".join(SOURCES)})')
    entry = detectors.get(detector)
    if entry is None:
        sys.exit(f'error: {param_type} of detector {detector!r} is not '
                  f'implemented yet (implemented for {param_type}: '
                  f'{", ".join(detectors)})')
    return entry

# _____________________________________________________________________________
def load_source_lines(base_dir: Path, input_run: int,
                       filenames: list[str], param_type: str,
                       detector: str) -> dict[Key, str]:
    txt_dir = base_dir / 'output' / 'txt' / f'run{input_run:05d}'
    merged: dict[Key, str] = {}
    for name in filenames:
        path = txt_dir / name.format(run=input_run)
        if not path.exists():
            print(f'[WARN] source not found (skipped): {path}',
                  file=sys.stderr)
            continue
        for raw_line in path.read_text().splitlines():
            key = line_key(raw_line)
            if key is None:
                continue
            merged[key] = '\t'.join(raw_line.split('#', 1)[0].split())
        print(f'[INFO] Read {path}')

    if not merged:
        sys.exit(f'error: no source lines found for {param_type} of '
                  f'{detector} (input_run {input_run})')
    return merged

# _____________________________________________________________________________
def check_input_run(entry, detector: str, param_type: str,
                     run_number: int, input_run: int) -> None:
    """Exit with an error unless input_run is in entry.input_runs."""
    if input_run not in entry.input_runs:
        sys.exit(f'error: input_run {input_run} is not in input_runs '
                  f'[{entry.input_first}, {entry.input_last}] of the '
                  f'{param_type} entry of {detector} that covers run '
                  f'{run_number} (calib-run-map/{detector}.toml)')

# _____________________________________________________________________________
def find_param_file(base_dir: Path, param_type: str,
                     run_number: int) -> Path:
    target = (base_dir / 'param' / param_type
              / f'HodoParam_hyps_common_run{run_number:05d}')
    if not target.exists():
        sys.exit(f'error: run{run_number:05d} has no {param_type} param '
                  f'yet: {target} does not exist')
    return target

# _____________________________________________________________________________
def write_param(target: Path, new_lines: dict[Key, str]) -> None:
    remaining = dict(new_lines)
    out_lines = []
    n_replaced = 0
    for raw_line in target.read_text().splitlines(keepends=True):
        key = line_key(raw_line)
        if key is not None and key in remaining:
            eol = '\n' if raw_line.endswith('\n') else ''
            new_line = remaining.pop(key)
            print(f'  {raw_line.rstrip()}  ->  {new_line}')
            out_lines.append(new_line + eol)
            n_replaced += 1
        else:
            out_lines.append(raw_line)

    target.write_text(''.join(out_lines))
    print(f'[INFO] Replaced {n_replaced} line(s) in {target}')
    if remaining:
        keys = ', '.join(str(k) for k in sorted(remaining))
        print(f'[WARN] {len(remaining)} key(s) had no matching line in '
              f'{target} (not written): {keys}', file=sys.stderr)


# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    # Check run number
    if args.run_number <= 0:
        print(f'error: invalid run number ({args.run_number})',
              file=sys.stderr)
        return 1
    if args.input_run is not None and args.input_run <= 0:
        print(f'error: invalid input_run ({args.input_run})',
              file=sys.stderr)
        return 1

    source = source_entry(args.param_type, args.detector)
    toml_detector = source['toml']
    filenames = source['files']

    try:
        entry = find_entry(toml_detector, args.param_type,
                           args.run_number)
    except CalibRunMapError as e:
        sys.exit(f'error: {e}')
    if args.input_run is not None:
        input_run = args.input_run
        check_input_run(entry, toml_detector, args.param_type,
                        args.run_number, input_run)
    else:
        input_run = entry.input_first
        print(f'[INFO] input_run not given; using {input_run} from '
              f'calib-run-map/{toml_detector}.toml '
              f'(run {args.run_number})')

    script_dir = Path(__file__).resolve().parent
    base_dir = script_dir.parent

    new_lines = load_source_lines(base_dir, input_run, filenames,
                                  args.param_type, args.detector)
    target = find_param_file(base_dir, args.param_type, args.run_number)
    write_param(target, new_lines)
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
