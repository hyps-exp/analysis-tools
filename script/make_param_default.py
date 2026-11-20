#!/usr/bin/env python3
"""Create missing initial params selected by run from calib-run-map/*.toml.

Usage: python3 script/make_param_default.py RUN FLAG
FLAG=0 copies defaults; FLAG=1 copies the previous HDPRM for TOF.
Existing files are never overwritten. --dry-run creates nothing and
--no-push disables the original automatic Git publication step.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from calib_run_map import CalibRunMapError, MAP_DIR
from param_names import load_maps, select_param_names

script_dir = Path(__file__).resolve().parent
param_dir = script_dir.parent / 'param'
# Keep the parameter types initialized by the original script.
CREATE_PARAMS = ('DCDRFT', 'DCTDC', 'HDPHC', 'CFTPED', 'CFTPOS',
                 'BGOCALIB', 'CATCHPID', 'HDPRM')


def load_param(run_no, maps):
    """Resolve required filenames from TOML valid_runs and filename_run."""
    registered = {param for _, param, _ in maps}
    missing = [p for p in CREATE_PARAMS if p not in registered]
    if missing:
        raise CalibRunMapError('TOML maps have no definitions for: ' + ', '.join(missing))
    selected = [row for row in maps if row[1] in CREATE_PARAMS]
    params, missing = select_param_names(run_no, selected)
    if missing:
        raise CalibRunMapError(f'run {run_no}: no entry covers ' + ', '.join(missing))
    return params


def previous_hdprm(run_no, maps, file_name):
    """Find the distinct HDPRM used before the first use of this filename.

    Returning to a broad interval after a narrow override must not seed
    the broad file from a later narrow file and create a dependency cycle.
    """
    maps = [row for row in maps if row[1] == 'HDPRM']
    boundaries = set()
    for _, _, entries in maps:
        for e in entries:
            boundaries.add(e.valid_from)
            if e.valid_to is not None:
                boundaries.add(e.valid_to + 1)
    boundaries = sorted(point for point in boundaries if point <= run_no)
    previous = None
    for first in boundaries:
        params, missing = select_param_names(first, maps)
        if missing:
            continue
        if params['HDPRM'] == file_name:
            return previous
        previous = params['HDPRM']
    return None


def copy_missing(source, target):
    """Publish a complete copy without replacing an existing target."""
    if target.is_file():
        return False
    if target.exists() or target.is_symlink():
        raise ValueError(f'parameter target is not a regular file: {target}')
    if not source.is_file():
        raise FileNotFoundError(f'copy source not found: {source}')
    fd, temp = tempfile.mkstemp(prefix=target.name + '.', dir=target.parent)
    os.close(fd)
    try:
        shutil.copy(source, temp)
        try:
            os.link(temp, target)  # FileExistsError protects concurrent creations.
        except FileExistsError:
            return False
    finally:
        Path(temp).unlink()
    print(f'[OK] created {target}')
    return True


def cp_param(dName, file_name, directory=param_dir):
    path = Path(directory) / dName
    return copy_missing(path / f'{dName}_default', path / file_name)


def cp_tof_param(dName, file_name, before_file, directory=param_dir):
    path = Path(directory) / dName
    if (path / file_name).is_file():
        return False
    if before_file is None:
        raise ValueError(f'TOF HDPRM has no previous setting: {file_name}; '
                         'initialize the first setting with FLAG=0')
    return copy_missing(path / before_file, path / file_name)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=int)
    parser.add_argument('flag', type=int, choices=(0, 1))
    parser.add_argument('--map-dir', type=Path, default=MAP_DIR)
    parser.add_argument('--param-dir', type=Path, default=param_dir)
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--no-push', action='store_true')
    args = parser.parse_args(argv)
    try:
        maps = load_maps(args.map_dir)
        params = load_param(args.run, maps)
        copies = []
        for dName in CREATE_PARAMS:
            file_name = params[dName]
            target = args.param_dir / dName / file_name
            if target.is_file():
                continue
            if target.exists() or target.is_symlink():
                raise ValueError(f'parameter target is not a regular file: {target}')
            before_file = None
            if dName == 'HDPRM' and args.flag == 1:
                before_file = previous_hdprm(args.run, maps, file_name)
                if before_file is None:
                    raise ValueError(f'TOF HDPRM has no previous setting: {file_name}; '
                                     'initialize the first setting with FLAG=0')
                source = target.parent / before_file
            else:
                source = target.parent / f'{dName}_default'
            if not source.is_file():
                raise FileNotFoundError(f'copy source not found: {source}')
            copies.append((dName, file_name, before_file, source))
        # Validate every source first, so a missing default cannot leave half a batch.
        created = 0
        for dName, file_name, before_file, source in copies:
            if args.dry_run:
                print(f'[dry-run] {source} -> {args.param_dir / dName / file_name}')
            elif dName == 'HDPRM' and args.flag == 1:
                created += cp_tof_param(dName, file_name, before_file, args.param_dir)
            else:
                created += cp_param(dName, file_name, args.param_dir)
        if not args.dry_run:
            print(f'[OK] created {created} initial param files; existing files kept')
            if created and not args.no_push:
                subprocess.run([sys.executable, str(script_dir / 'gitpush.py'),
                                str(args.param_dir.resolve()), 'Make initial param'],
                               cwd=script_dir, check=True)
    except (CalibRunMapError, ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
