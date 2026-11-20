#!/usr/bin/env python3
"""Read the calibration run maps in calib-run-map/.

A calibration run map (calib-run-map/<detector>.toml) tells, for each
parameter of one detector, which runs are analyzed to make it and for
which runs it is used. Every script that needs this information should
get it through this module, so that the file format can change without
changing the scripts.

Use from another script in script/:
    from calib_run_map import CalibRunMapError, find_entry, load_map

    try:
        entry = find_entry('TOF', 'HDPHC', 7005)   # reads TOF.toml
    except CalibRunMapError as e:       # also if no entry covers 7005
        sys.exit(f'error: {e}')

    entry.valid_from, entry.valid_to     # runs that use the parameter;
                                         # valid_to is None if the end
                                         # is not decided (-1 in the map)
    entry.is_open_ended                  # True if valid_to is None
    entry.filename_run                   # run number for the file name
    entry.input_first, entry.input_last  # runs to analyze
    entry.input_runs                     # the same, as a list of runs
    entry.pedestal_run                   # pedestal run; None if the
                                         # entry has no pedestal_run

    for entry in load_map('TOF')['HDPRM']:
        ...

Run as a command to check a map and print it:
    ./script/calib_run_map.py calib-run-map/TOF.toml
    ./script/calib_run_map.py calib-run-map/TOF.toml HDPHC
    ./script/calib_run_map.py calib-run-map/TOF.toml HDPHC -r 7005

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
PARAM_NAMES = ('BGOCALIB', 'BGOTEMP', 'CATCHPID', 'CFTPED', 'CFTPOS',
               'DCDRFT', 'DCTDC', 'HDPHC', 'HDPRM', 'USER')

# Keys of an entry: every entry has REQUIRED_KEYS and can have
# OPTIONAL_KEYS.
REQUIRED_KEYS = ('valid_runs', 'filename_run', 'input_runs')
OPTIONAL_KEYS = ('pedestal_run',)

# Written as the end of valid_runs when it is not decided yet. It is
# the only way to write it: a large number such as 99999 is an error.
OPEN_ENDED = -1

# Run numbers are 0 to MAX_RUN (run%05d). MAX_RUN itself was once used
# to mean that, so it is not accepted as the end of valid_runs.
MAX_RUN = 99999


# _____________________________________________________________________________
class CalibRunMapError(ValueError):
    """A calibration run map is missing or is not written correctly."""


# _____________________________________________________________________________
class EntryNotFoundError(CalibRunMapError):
    """The map is read correctly, but no entry covers the run.

    Catch this one, not CalibRunMapError, in a script that can go on
    without an entry.
    """


# _____________________________________________________________________________
@dataclass(frozen=True)
class Entry:
    """One parameter to make: the runs to analyze and where it is used."""
    valid_from: int
    valid_to: Optional[int]
    filename_run: int
    input_first: int
    input_last: int
    pedestal_run: Optional[int] = None

    @property
    def input_runs(self) -> list[int]:
        """Runs to analyze, input_first to input_last (both included)."""
        return list(range(self.input_first, self.input_last + 1))

    @property
    def is_open_ended(self) -> bool:
        """True if the end of the valid runs is not decided yet."""
        return self.valid_to is None

    def covers(self, run: int) -> bool:
        """Return True if the parameter is used for run."""
        if run < self.valid_from:
            return False
        return self.valid_to is None or run <= self.valid_to


# _____________________________________________________________________________
def _is_run(value: Any) -> bool:
    return (isinstance(value, int) and not isinstance(value, bool)
            and 0 <= value <= MAX_RUN)


# _____________________________________________________________________________
def _read_range(value: Any, key: str, where: str) -> tuple[int, int]:
    if (not isinstance(value, list) or len(value) != 2
            or not all(_is_run(v) for v in value)):
        raise CalibRunMapError(
            f'{where}: {key} must be two run numbers [first, last], '
            f'each 0 to {MAX_RUN} (got {value!r})')
    if value[0] > value[1]:
        raise CalibRunMapError(
            f'{where}: {key} = {value!r} has first > last')
    return value[0], value[1]


# _____________________________________________________________________________
def _read_valid_runs(value: Any, where: str) -> tuple[int, Optional[int]]:
    """Read valid_runs; the end is None if it is written as OPEN_ENDED."""
    if (isinstance(value, list) and len(value) == 2
            and _is_run(value[0]) and value[1] == OPEN_ENDED
            and not isinstance(value[1], bool)):
        return value[0], None
    hint = (f'write {OPEN_ENDED} as last if the end is not decided yet '
            '(no other value means that)')
    try:
        first, last = _read_range(value, 'valid_runs', where)
    except CalibRunMapError as e:
        raise CalibRunMapError(f'{e}; {hint}') from None
    if last == MAX_RUN:
        raise CalibRunMapError(
            f'{where}: valid_runs = {value!r} ends with {MAX_RUN}; '
            f'{hint}')
    return first, last


# _____________________________________________________________________________
def _valid_text(entry: Entry) -> str:
    """Return valid_runs of entry as it is written in the map."""
    last = OPEN_ENDED if entry.is_open_ended else entry.valid_to
    return f'[{entry.valid_from}, {last}]'


# _____________________________________________________________________________
def _read_entry(item: Any, where: str) -> Entry:
    if not isinstance(item, dict):
        raise CalibRunMapError(
            f'{where}: an entry must be {{ '
            'valid_runs = [A, B], filename_run = N, '
            'input_runs = [X, Y] }'
            f' (got {item!r})')
    missing = [key for key in REQUIRED_KEYS if key not in item]
    unknown = [key for key in item
               if key not in REQUIRED_KEYS + OPTIONAL_KEYS]
    if missing or unknown:
        raise CalibRunMapError(
            f'{where}: an entry needs the keys '
            f'{", ".join(REQUIRED_KEYS)} and can have '
            f'{", ".join(OPTIONAL_KEYS)} '
            f'(missing: {missing}, unknown: {unknown})')
    valid_from, valid_to = _read_valid_runs(item['valid_runs'], where)
    input_first, input_last = _read_range(item['input_runs'],
                                          'input_runs', where)
    filename_run = item['filename_run']
    if not _is_run(filename_run):
        raise CalibRunMapError(
            f'{where}: filename_run must be a run number, '
            f'0 to {MAX_RUN} '
            f'(got {filename_run!r})')
    pedestal_run = item.get('pedestal_run')
    if pedestal_run is not None and not _is_run(pedestal_run):
        raise CalibRunMapError(
            f'{where}: pedestal_run must be a run number, '
            f'0 to {MAX_RUN} '
            f'(got {pedestal_run!r})')
    return Entry(valid_from, valid_to, filename_run,
                 input_first, input_last, pedestal_run)


# _____________________________________________________________________________
def _read_same_as(value: dict, where: str) -> str:
    """Return the parameter name written as { same_as = "<name>" }."""
    if list(value) != ['same_as'] or not isinstance(value['same_as'], str):
        raise CalibRunMapError(
            f'{where} must be a list of entries or '
            '{ same_as = "<parameter name>" } with no other key '
            f'(got {value!r})')
    return value['same_as']


# _____________________________________________________________________________
def _load(detector: str, map_dir: Optional[Path] = None
          ) -> tuple[dict[str, list[Entry]], dict[str, str]]:
    """Read the map of detector.

    Return {parameter name: entries} and {parameter name: the
    parameter it is the same as}; both are in the order of the file.
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
    same_as = {}
    for param, items in data.items():
        if param not in PARAM_NAMES:
            raise CalibRunMapError(
                f'{path}: unknown parameter name {param!r} '
                f'(known: {", ".join(PARAM_NAMES)})')
        if isinstance(items, dict):
            same_as[param] = _read_same_as(items, f'{path}: {param}')
            continue
        if not isinstance(items, list):
            raise CalibRunMapError(
                f'{path}: {param} must be a list of entries, '
                f'{param} = [ {{...}}, ], or '
                '{ same_as = "<parameter name>" }')
        entries = [_read_entry(item, f'{path}: {param} entry {i}')
                   for i, item in enumerate(items, 1)]
        for i in range(1, len(entries)):
            before, after = entries[i - 1], entries[i]
            if before.is_open_ended:
                raise CalibRunMapError(
                    f'{path}: {param} entry {i}: valid_runs = '
                    f'{_valid_text(before)} has no end yet, so it '
                    f'must be the last entry of {param}')
            if after.valid_from <= before.valid_to:
                raise CalibRunMapError(
                    f'{path}: {param} entry {i + 1}: valid_runs = '
                    f'{_valid_text(after)} must start after valid_runs '
                    f'of entry {i}, {_valid_text(before)}')
        result[param] = entries

    for param, target in same_as.items():
        if target in same_as:
            raise CalibRunMapError(
                f'{path}: {param} is same_as {target}, but {target} '
                'is also written with same_as; point to a parameter '
                'that has a list of entries')
        if target not in result:
            raise CalibRunMapError(
                f'{path}: {param} is same_as {target!r}, but this file '
                f'has no {target} '
                f'(it has: {", ".join(result) or "nothing"})')
        result[param] = result[target]
    return {param: result[param] for param in data}, same_as


# _____________________________________________________________________________
def load_map(detector: str,
             map_dir: Optional[Path] = None) -> dict[str, list[Entry]]:
    """Read the map of detector and return {parameter name: entries}.

    The entries of each parameter are in the order of their
    valid_runs. A parameter written as { same_as = "<name>" } has the
    entries of that parameter. Raise CalibRunMapError if the file is
    missing or wrong.
    """
    return _load(detector, map_dir)[0]


# _____________________________________________________________________________
def find_entry(detector: str, param: str, run: int,
               map_dir: Optional[Path] = None) -> Entry:
    """Return the entry of param whose valid_runs include run.

    Raise EntryNotFoundError if no entry covers run, and
    CalibRunMapError if the map is missing or wrong or has no param.
    """
    entries = load_map(detector, map_dir)
    if param not in entries:
        raise CalibRunMapError(
            f'{detector}.toml has no {param} '
            f'(it has: {", ".join(entries) or "nothing"})')
    for entry in entries[param]:
        if entry.covers(run):
            return entry
    raise EntryNotFoundError(
        f'{detector}.toml has no {param} entry that covers run {run}')


# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    # required
    parser.add_argument('map_file',
                        help='path of the map to check, such as '
                             'calib-run-map/TOF.toml')
    # optional
    parser.add_argument('param', nargs='?', default=None,
                        help='parameter name such as HDPRM '
                             '(default: all in the file)')
    parser.add_argument('-r', '--run', type=int, default=None,
                        help='print only the entry used for this run '
                             '(needs param)')
    return parser.parse_args(argv)


# _____________________________________________________________________________
def print_entries(param: str, entries: list[Entry],
                  same_as: Optional[str] = None) -> None:
    """Print the entries of param as a table."""
    print(f'[{param}]' + (f' (same as {same_as})' if same_as else ''))
    print(f'  {"valid_from":>10} {"valid_to":>8} {"filename_run":>12} '
          f'{"input_first":>11} {"input_last":>10} '
          f'{"pedestal_run":>12}')
    for e in entries:
        valid_to = OPEN_ENDED if e.is_open_ended else e.valid_to
        pedestal = '-' if e.pedestal_run is None else e.pedestal_run
        print(f'  {e.valid_from:>10} {valid_to:>8} {e.filename_run:>12} '
              f'{e.input_first:>11} {e.input_last:>10} '
              f'{pedestal:>12}')


# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    if args.run is not None and args.param is None:
        print('error: --run needs param', file=sys.stderr)
        return 1

    map_file = Path(args.map_file)
    if map_file.suffix != '.toml':
        print(f'error: {args.map_file} is not the path of a map; give '
              'a path such as calib-run-map/TOF.toml', file=sys.stderr)
        return 1
    detector, map_dir = map_file.stem, map_file.parent

    try:
        if args.run is not None:
            entry = find_entry(detector, args.param, args.run, map_dir)
            print_entries(args.param, [entry])
            return 0

        entries, same_as = _load(detector, map_dir)
        if args.param is not None and args.param not in entries:
            print(f'error: {map_file} has no {args.param}',
                  file=sys.stderr)
            return 1
    except CalibRunMapError as e:
        print(f'error: {e}', file=sys.stderr)
        return 1

    for param, items in entries.items():
        if args.param in (None, param):
            print_entries(param, items, same_as.get(param))
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
