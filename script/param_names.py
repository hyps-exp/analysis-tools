#!/usr/bin/env python3
"""Generate parameter filenames from calib-run-map/*.toml.

Run from any directory:
    python3 script/param_names.py                # all maps except example.toml
    python3 script/param_names.py TOF            # every name in TOF.toml
    python3 script/param_names.py example --run 7005

Names use each entry's filename_run, not its valid_from or input_first.
--run selects entries whose valid_runs contains the analysis run; it does
not replace filename_run. Without --run no analysis run is required.

Use from another script in script/:
    from param_names import param_filename, load_param_names, find_param_names
    param_filename('DCTDC', 7162)     # DCTdcCalib_hyps_common_run07162
    load_param_names('TOF')          # {parameter: [unique filenames, ...]}
    find_param_names('TOF', 7005)    # {parameter: filename}

Fixed names are included in both mappings.
TOML validation and overlap selection here leave calib_run_map.py unchanged. Missing parameter types
are not invented. Detectors remain separate; this module does not combine
several detectors' calibration values into one parameter file. It creates
no parameter, conf, or TXT files; the command prints its table to stdout.
"""

import argparse
import sys
from pathlib import Path
from typing import Optional

from calib_run_map import (CalibRunMapError, Entry, MAP_DIR, _read_entry,
                           _read_same_as, _read_valid_runs, _is_run, tomllib)

# Keep spelling and capitalization exactly as specified for shared params.
# In particular, TAGPLMTH intentionally uses "commn", without the second "o".
FIXED_NAMES = {
    'TAGPLMTH': 'TAGPLMatchParam_hyps_commn',
    'DCGEO': 'DCGeomParam_hyps_common',
    'BGOTEMP': 'run01514.txt',
}
RUN_NAME_FORMATS = {
    'CMAP': 'channel_map_hyps_common_run{run:05d}.xml',
    'DCDRFT': 'DCDriftParam_hyps_common_run{run:05d}',
    'DCTDC': 'DCTdcCalib_hyps_common_run{run:05d}',
    'HDPRM': 'HodoParam_hyps_common_run{run:05d}',
    'HDPHC': 'HodoPHCParam_hyps_common_run{run:05d}',
    'USER': 'UserParam_hyps_common_run{run:05d}',
    'CFTPED': 'CFTPedParam_hyps_common_run{run:05d}',
    'CFTPOS': 'CftPosParam_hyps_common_run{run:05d}',
    'BGOCALIB': 'BGOEnergyCalib_hyps_common_run{run:05d}',
    'CATCHPID': 'CATCHPid_hyps_common_run{run:05d}',
}
PARAM_ORDER = ('CMAP', 'TAGPLMTH', 'DCGEO', 'DCDRFT', 'DCTDC', 'HDPRM',
               'HDPHC', 'USER', 'CFTPED', 'CFTPOS', 'BGOTEMP', 'BGOCALIB',
               'CATCHPID')


def _check_run(run: int) -> None:
    if isinstance(run, bool) or not isinstance(run, int) or run < 0:
        raise ValueError(f'run must be a nonnegative integer (got {run!r})')


def param_filename(param: str, filename_run: Optional[int] = None) -> str:
    """Return one filename. Fixed names do not require a run number."""
    if param in FIXED_NAMES:
        return FIXED_NAMES[param]
    if param not in RUN_NAME_FORMATS:
        raise ValueError(f'unknown parameter {param!r}; expected '
                         f'{", ".join(PARAM_ORDER)}')
    _check_run(filename_run)
    return RUN_NAME_FORMATS[param].format(run=filename_run)


def _read_naming_entry(item, where, param):
    """CMAP and USER can name files without calibration input_runs."""
    if (param not in ('CMAP', 'USER') or not isinstance(item, dict)
            or 'input_runs' in item):
        return _read_entry(item, where, param)
    required = ('valid_runs', 'filename_run')
    missing = [key for key in required if key not in item]
    unknown = [key for key in item if key not in required]
    if missing or unknown:
        raise CalibRunMapError(
            f'{where}: an entry needs the keys {", ".join(required)} '
            f'(missing: {missing}, unknown: {unknown})')
    first, last = _read_valid_runs(item['valid_runs'], where)
    if not _is_run(item['filename_run']):
        raise CalibRunMapError(f'{where}: invalid filename_run {item["filename_run"]!r}')
    return Entry(first, last, item['filename_run'])


def load_map(detector: str, map_dir: Optional[Path] = None):
    """Read naming maps without modifying calib_run_map.py.

    Reuse its per-entry schema validation. Naming-specific parameter types
    and overlap resolution stay here; the original reader is unchanged.
    """
    path = Path(map_dir if map_dir is not None else MAP_DIR) / f'{detector}.toml'
    try:
        with open(path, 'rb') as source:
            data = tomllib.load(source)
    except FileNotFoundError:
        raise CalibRunMapError(f'{path}: no such file') from None
    except tomllib.TOMLDecodeError as error:
        raise CalibRunMapError(f'{path}: {error}') from None
    result, same_as = {}, {}
    for param, items in data.items():
        if param not in RUN_NAME_FORMATS and param != 'FILLING':
            raise CalibRunMapError(f'{path}: unknown parameter name {param!r}')
        if isinstance(items, dict):
            same_as[param] = _read_same_as(items, f'{path}: {param}')
            continue
        if not isinstance(items, list):
            raise CalibRunMapError(f'{path}: {param} must be a list of entries')
        entries = [_read_naming_entry(item, f'{path}: {param} entry {i}', param)
                   for i, item in enumerate(items, 1)]
        result[param] = sorted(entries, key=lambda e: (
            e.valid_from, float('inf') if e.valid_to is None else e.valid_to))
    for param, target in same_as.items():
        if target in same_as:
            raise CalibRunMapError(f'{path}: {param} same_as must point directly to a list')
        if target not in result:
            raise CalibRunMapError(f'{path}: {param} same_as target {target!r} has no entries')
        result[param] = result[target]
    # FILLING is metadata; USER filenames come from explicit USER entries.
    result.pop('FILLING', None)
    return result


def load_maps(map_dir: Optional[Path] = None):
    """Read all detector maps as (detector, parameter, entries) tuples."""
    folder = Path(map_dir if map_dir is not None else MAP_DIR)
    result = []
    for path in sorted(folder.glob('*.toml')):
        if path.name == 'example.toml':
            continue
        for param, entries in load_map(path.stem, folder).items():
            if not entries:
                raise CalibRunMapError(f'{path}: {param} has no entries')
            result.append((path.stem, param, entries))
    if not result:
        raise CalibRunMapError(f'{folder}: no detector TOML entries found')
    return result


def entry_priority(entry):
    """Compare original valid_runs without replacing an unknown end."""
    if entry.valid_to is None:
        return (1, -entry.valid_from)
    return (0, entry.valid_to - entry.valid_from + 1)


def select_entry(entries: list[Entry], run: int,
                 where: str = 'map') -> Optional[Entry]:
    """Select the narrowest original valid_runs containing run.

    If minimum-width entries agree on filename_run, use the earliest
    valid_from/valid_to for a stable representative of that filename.
    """
    _check_run(run)
    candidates = [entry for entry in entries if entry.covers(run)]
    if not candidates:
        return None
    priority = min(entry_priority(e) for e in candidates)
    best = [e for e in candidates if entry_priority(e) == priority]
    if len({e.filename_run for e in best}) != 1:
        details = ', '.join(
            f'[{e.valid_from}, {e.valid_to}] -> {e.filename_run}' for e in best)
        raise CalibRunMapError(
            f'{where}: run {run}: equal-width conflicting entries: {details}')
    return min(best, key=lambda e: (e.valid_from, float('inf') if e.valid_to is None else e.valid_to))


def select_param_names(run_no, maps):
    """Use the narrowest valid_runs covering run_no for each parameter.

    Candidates within and across detector maps are compared by their original
    inclusive interval width. Equal-width candidates must name the same
    file; otherwise the choice is ambiguous and generation stops.
    """
    _check_run(run_no)
    candidates = {}
    for detector, param, entries in maps:
        candidates.setdefault(param, []).extend(
            (detector, e) for e in entries if e.covers(run_no))
    missing = [param for param, entries in candidates.items() if not entries]
    if missing:
        return None, missing
    params = dict(FIXED_NAMES)
    for param, entries in candidates.items():
        priority = min(entry_priority(e) for _, e in entries)
        best = [(detector, e) for detector, e in entries
                if entry_priority(e) == priority]
        names = {param_filename(param, e.filename_run) for _, e in best}
        if len(names) != 1:
            details = ', '.join(
                f'{detector} [{e.valid_from}, {e.valid_to}]='
                f'{param_filename(param, e.filename_run)}' for detector, e in best)
            raise CalibRunMapError(
                f'run {run_no}: equal-width conflicting {param} entries: {details}')
        params[param] = names.pop()
    return params, []


def load_param_names(detector: str,
                     map_dir: Optional[Path] = None) -> dict[str, list[str]]:
    """Return every filename in a map, removing duplicates per parameter.

    Every list follows valid_runs order. Fixed names are included even
    though they do not need entries in TOML. No parameter files are read.
    """
    entries = load_map(detector, map_dir)
    result = {param: [name] for param, name in FIXED_NAMES.items()}
    for param, items in entries.items():
        result[param] = list(dict.fromkeys(
            param_filename(param, entry.filename_run) for entry in items))
    return {param: result[param] for param in PARAM_ORDER if param in result}


def find_param_names(detector: str, run: int,
                     map_dir: Optional[Path] = None) -> dict[str, str]:
    """Resolve all mapped parameter types for an analysis run.

    Raise CalibRunMapError if any type present in this map has no entry
    covering run. The narrowest covering valid_runs wins. An uncovered
    run never inherits the previous entry.
    """
    _check_run(run)
    entries = load_map(detector, map_dir)
    result = dict(FIXED_NAMES)
    missing = []
    for param, items in entries.items():
        entry = select_entry(items, run, f'{detector}.toml {param}')
        if entry is None:
            missing.append(param)
        else:
            result[param] = param_filename(param, entry.filename_run)
    if missing:
        raise CalibRunMapError(f'{detector}.toml: no entry covers run {run} '
                               f'for {", ".join(missing)}')
    return {param: result[param] for param in PARAM_ORDER if param in result}


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('detector', nargs='?',
                        help='map name, e.g. TOF or example '
                             '(default: all maps except example.toml)')
    parser.add_argument('-r', '--run', type=int,
                        help='analysis run to select by valid_runs '
                             '(default: list all filename_run values)')
    parser.add_argument('--map-dir', type=Path,
                        help='directory containing TOML maps '
                             '(default: calib-run-map next to script/)')
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    map_dir = args.map_dir if args.map_dir is not None else MAP_DIR
    try:
        if args.run is not None:
            _check_run(args.run)
        if args.detector is not None:
            detectors = [args.detector]
        else:
            detectors = [path.stem for path in sorted(map_dir.glob('*.toml'))
                         if path.name != 'example.toml']
            if not detectors:
                raise CalibRunMapError(f'{map_dir}: no detector TOML maps found')
        # Validate every selected map before printing anything, so malformed
        # input cannot leave a plausible-looking partial table on stdout.
        rows = []
        for detector in detectors:
            if args.run is None:
                names = load_param_names(detector, map_dir)
            else:
                names = {param: [name] for param, name in
                         find_param_names(detector, args.run, map_dir).items()}
            for param, filenames in names.items():
                for name in filenames:
                    rows.append((detector, param, name))
    except (CalibRunMapError, ValueError, OSError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    print('# detector parameter filename')
    for detector, param, name in rows:
        print(f'{detector}\t{param}\t{name}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
