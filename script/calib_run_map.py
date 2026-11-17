#!/usr/bin/env python3
"""Read the calibration run maps in calib-run-map/.

A calibration run map (calib-run-map/<detector>.toml) tells, for each
parameter of one detector, which runs are analyzed to make it and for
which runs it is used. Every script that needs this information should
get it through this module, so that the file format can change without
changing the scripts.

Use from another script in script/:
    from calib_run_map import find, load

    entry = find('TOF', 'HDPHC', 7005)   # reads calib-run-map/TOF.toml;
                                         # None if no entry covers 7005
    entry.valid_from, entry.valid_to     # runs that use the parameter
    entry.filename_run                   # run number for the file name
    entry.input_first, entry.input_last  # runs to analyze
    entry.input_runs                     # the same, as a list of runs

    for entry in load('TOF')['HDPRM']:
        ...

Run as a command to check a map and print it:
    python3 script/calib_run_map.py example
    python3 script/calib_run_map.py example HDPHC
    python3 script/calib_run_map.py example HDPHC --run 7005

Requirement:
    tomli (Python 3.10 or older): conda install tomli
"""

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional

try:
    import tomllib
except ModuleNotFoundError:
    try:
        import tomli as tomllib
    except ModuleNotFoundError:
        sys.exit('error: the tomli package is needed to read '
                 'calib-run-map/*.toml (run "conda install tomli")')

# The only place that knows where the maps are.
MAP_DIR = Path(__file__).resolve().parent.parent / 'calib-run-map'

# Parameter names that can be written in a map.
PARAM_NAMES = ('BGOCALIB', 'CATCHPID', 'CFTPED', 'CFTPOS', 'DCDRFT',
               'DCTDC', 'HDPHC', 'HDPRM', 'USER')

ENTRY_KEYS = ('valid_runs', 'filename_run', 'input_runs')


# _____________________________________________________________________________
class CalibRunMapError(ValueError):
    """A calibration run map is missing or is not written correctly."""


# _____________________________________________________________________________
@dataclass(frozen=True)
class Entry:
    """One parameter to make: the runs to analyze and where it is used."""
    valid_from: int
    valid_to: int
    filename_run: int
    input_first: int
    input_last: int

    @property
    def input_runs(self) -> list[int]:
        """Runs to analyze, input_first to input_last (both included)."""
        return list(range(self.input_first, self.input_last + 1))

    def covers(self, run: int) -> bool:
        """Return True if the parameter is used for run."""
        return self.valid_from <= run <= self.valid_to


# _____________________________________________________________________________
def _is_run(value: Any) -> bool:
    return (isinstance(value, int) and not isinstance(value, bool)
            and value >= 0)


# _____________________________________________________________________________
def _read_range(value: Any, key: str, where: str) -> tuple[int, int]:
    if (not isinstance(value, list) or len(value) != 2
            or not all(_is_run(v) for v in value)):
        raise CalibRunMapError(
            f'{where}: {key} must be two run numbers [first, last] '
            f'(got {value!r})')
    if value[0] > value[1]:
        raise CalibRunMapError(
            f'{where}: {key} = {value!r} has first > last')
    return value[0], value[1]


# _____________________________________________________________________________
def _read_entry(item: Any, where: str) -> Entry:
    if not isinstance(item, dict):
        raise CalibRunMapError(
            f'{where}: an entry must be {{ '
            'valid_runs = [A, B], filename_run = N, '
            'input_runs = [X, Y] }'
            f' (got {item!r})')
    missing = [key for key in ENTRY_KEYS if key not in item]
    unknown = [key for key in item if key not in ENTRY_KEYS]
    if missing or unknown:
        raise CalibRunMapError(
            f'{where}: an entry needs exactly the keys '
            f'{", ".join(ENTRY_KEYS)} '
            f'(missing: {missing}, unknown: {unknown})')
    valid_from, valid_to = _read_range(item['valid_runs'],
                                       'valid_runs', where)
    input_first, input_last = _read_range(item['input_runs'],
                                          'input_runs', where)
    filename_run = item['filename_run']
    if not _is_run(filename_run):
        raise CalibRunMapError(
            f'{where}: filename_run must be a run number '
            f'(got {filename_run!r})')
    return Entry(valid_from, valid_to, filename_run,
                 input_first, input_last)


# _____________________________________________________________________________
def load(detector: str,
         map_dir: Optional[Path] = None) -> dict[str, list[Entry]]:
    """Read the map of detector and return {parameter name: entries}.

    The entries of each parameter are in the order of their
    valid_runs. Raise CalibRunMapError if the file is missing or wrong.
    """
    path = Path(map_dir or MAP_DIR) / f'{detector}.toml'
    try:
        with open(path, 'rb') as f:
            data = tomllib.load(f)
    except FileNotFoundError:
        raise CalibRunMapError(f'{path}: no such file') from None
    except tomllib.TOMLDecodeError as e:
        raise CalibRunMapError(f'{path}: {e}') from None

    result = {}
    for param, items in data.items():
        if param not in PARAM_NAMES:
            raise CalibRunMapError(
                f'{path}: unknown parameter name {param!r} '
                f'(known: {", ".join(PARAM_NAMES)})')
        if not isinstance(items, list):
            raise CalibRunMapError(
                f'{path}: {param} must be a list of entries, '
                f'{param} = [ {{...}}, ]')
        entries = [_read_entry(item, f'{path}: {param} entry {i}')
                   for i, item in enumerate(items, 1)]
        for i in range(1, len(entries)):
            before, after = entries[i - 1], entries[i]
            if after.valid_from <= before.valid_to:
                raise CalibRunMapError(
                    f'{path}: {param} entry {i + 1}: valid_runs = '
                    f'[{after.valid_from}, {after.valid_to}] must start '
                    f'after valid_runs of entry {i}, '
                    f'[{before.valid_from}, {before.valid_to}]')
        result[param] = entries
    return result


# _____________________________________________________________________________
def find(detector: str, param: str, run: int,
         map_dir: Optional[Path] = None) -> Optional[Entry]:
    """Return the entry of param used for run, or None if there is none.

    Raise CalibRunMapError if the map of detector has no param.
    """
    entries = load(detector, map_dir)
    if param not in entries:
        raise CalibRunMapError(
            f'{detector}.toml has no {param} '
            f'(it has: {", ".join(entries) or "nothing"})')
    for entry in entries[param]:
        if entry.covers(run):
            return entry
    return None


# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    # required
    parser.add_argument('detector',
                        help='detector name: the file is '
                             'calib-run-map/<detector>.toml')
    # optional
    parser.add_argument('param', nargs='?', default=None,
                        help='parameter name such as HDPRM '
                             '(default: all in the file)')
    parser.add_argument('-r', '--run', type=int, default=None,
                        help='print only the entry used for this run '
                             '(needs param)')
    return parser.parse_args(argv)


# _____________________________________________________________________________
def print_entries(param: str, entries: list[Entry]) -> None:
    """Print the entries of param as a table."""
    print(f'[{param}]')
    print(f'  {"valid_from":>10} {"valid_to":>8} {"filename_run":>12} '
          f'{"input_first":>11} {"input_last":>10}')
    for e in entries:
        print(f'  {e.valid_from:>10} {e.valid_to:>8} {e.filename_run:>12} '
              f'{e.input_first:>11} {e.input_last:>10}')


# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    if args.run is not None and args.param is None:
        print('error: --run needs param', file=sys.stderr)
        return 1

    try:
        if args.run is not None:
            entry = find(args.detector, args.param, args.run)
            if entry is None:
                print(f'error: no {args.param} entry of {args.detector} '
                      f'covers run {args.run}', file=sys.stderr)
                return 1
            print_entries(args.param, [entry])
            return 0

        entries = load(args.detector)
        if args.param is not None and args.param not in entries:
            print(f'error: {args.detector}.toml has no {args.param}',
                  file=sys.stderr)
            return 1
    except CalibRunMapError as e:
        print(f'error: {e}', file=sys.stderr)
        return 1

    for param, items in entries.items():
        if args.param in (None, param):
            print_entries(param, items)
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
