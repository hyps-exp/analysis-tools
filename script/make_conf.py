#!/usr/bin/env python3
"""Generate all conf files from TOML. No run argument is accepted."""

import argparse
import os
from pathlib import Path
import sys
import tempfile

from calib_run_map import CalibRunMapError, MAP_DIR
from param_names import PARAM_ORDER, RUN_NAME_FORMATS, load_map as load, select_param_names

base_dir = Path(__file__).resolve().parent.parent
conf_dir = base_dir / 'param' / 'conf'


def load_param(run_no, maps):
    """Resolve this run with the common filename/interval selection rules."""
    return select_param_names(run_no, maps)


def build_conf_plans(map_dir=MAP_DIR):
    """Return (conf_list, gaps); each conf row is (first, last, name, params)."""
    maps, boundaries = [], set()
    open_ended = False
    for path in sorted(Path(map_dir).glob('*.toml')):
        if path.name == 'example.toml':
            continue
        for param, entries in load(path.stem, Path(map_dir)).items():
            if not entries:
                raise CalibRunMapError(f'{path.name}: {param} has no entries')
            maps.append((path.stem, param, entries))
            for entry in entries:
                boundaries.add(entry.valid_from)
                if entry.valid_to is None:
                    open_ended = True
                else:
                    boundaries.add(entry.valid_to + 1)
    if not maps:
        raise CalibRunMapError(f'{map_dir}: no detector TOML entries found')
    registered = {param for _, param, _ in maps}
    missing = [p for p in RUN_NAME_FORMATS if p not in registered]
    if missing:
        raise CalibRunMapError('TOML maps have no definitions for: ' + ', '.join(missing))
    conf_list, gaps, conf_names = [], [], {}
    boundaries = sorted(boundaries)
    stops = boundaries[1:] + ([None] if open_ended else [])
    for first, stop in zip(boundaries, stops):
        last = None if stop is None else stop - 1
        params, missing = load_param(first, maps)
        if missing:
            if gaps and gaps[-1][1] + 1 == first and gaps[-1][2] == missing:
                gaps[-1] = (gaps[-1][0], last, missing)
            else:
                gaps.append((first, last, missing))
            continue
        key = tuple(params[p] for p in PARAM_ORDER)
        if key not in conf_names:
            conf_names[key] = f'analyzer_hyps_common_run{first:05d}.conf'
        name = conf_names[key]
        if conf_list and conf_list[-1][1] + 1 == first and conf_list[-1][2] == name:
            conf_list[-1] = (conf_list[-1][0], last, name, params)
        else:
            conf_list.append((first, last, name, params))
    if not conf_list:
        raise CalibRunMapError('no run interval is covered by all required parameters')
    return conf_list, gaps


def write_conf(conf, params):
    """Write the same conf items as the original script, with TOML filenames."""
    conf.write('# Generated from calib-run-map/*.toml by make_conf.py.\n\n')
    conf.write('UNPACK:         ../UNPACK/unpacker_online.xml\n')
    conf.write('DIGIT:          ../DIGIT/digit_data_hyps_20250419.xml\n')
    conf.write(f"CMAP:           ../CMAP/{params['CMAP']}\n")
    conf.write('BH2FLT:         ../BH2FLT/BH2Filter_20180603\n')
    conf.write(f"TAGPLMTH:       ../TAGPLMTH/{params['TAGPLMTH']}\n")
    conf.write(f"DCGEO:          ../DCGEO/{params['DCGEO']}\n")
    conf.write(f"DCDRFT:         ../DCDRFT/{params['DCDRFT']}\n")
    conf.write(f"DCTDC:          ../DCTDC/{params['DCTDC']}\n")
    conf.write(f"HDPRM:          ../HDPRM/{params['HDPRM']}\n")
    conf.write(f"HDPHC:          ../HDPHC/{params['HDPHC']}\n")
    conf.write('FLDMAP:         ../fieldmap/lepsSD53_std_v12_K18style_extend.dat\n')
    conf.write('FLDNMR:         1.\n')
    conf.write('FLDCALC:        1.\n')
    conf.write('PK18:           1.8\n')
    conf.write('K18TM:          ../K18TM/K18MatrixParamD2U_0\n')
    conf.write('MATRIX2D1:      ../MATRIX/mtx2d1_e42_GEANT4_Normal.txt\n')
    conf.write('MATRIX2D2:      ../MATRIX/mtx2d1_e42_GEANT4_tight.txt\n')
    conf.write('MATRIX3D:       ../MATRIX/mtx3d_e42_GEANT4.txt\n')
    conf.write(f"USER:           ../USER/{params['USER']}\n")
    conf.write(f"CFTPED:         ../CFTPED/{params['CFTPED']}\n")
    conf.write(f"CFTPOS:         ../CFTPOS/{params['CFTPOS']}\n")
    conf.write(f"BGOTEMP:        ../BGOTEMP/{params['BGOTEMP']}\n")
    conf.write(f"BGOCALIB:       ../BGOCALIB/{params['BGOCALIB']}\n")
    conf.write('EVSELECT:       ../EVSELECT/event_dummy.txt\n')
    conf.write(f"CATCHPID:       ../CATCHPID/{params['CATCHPID']}\n")


def write_conf_list(out, conf_list):
    out.write('# first_run last_run conf_file\n')
    for first, last, conf_name, _ in conf_list:
        out.write(f'{first} {-1 if last is None else last} {conf_name}\n')


def write_outputs(conf_list, output_dir):
    """Stage complete files, then replace them; publish conf.txt last."""
    confs = {}
    for _, _, name, params in conf_list:
        missing = [p for p in PARAM_ORDER if p not in params]
        if missing:
            raise CalibRunMapError('incomplete conf: ' + ', '.join(missing))
        if name in confs and confs[name] != params:
            raise ValueError(f'conf name collision: {name}')
        confs[name] = params
    if not confs:
        raise ValueError('no conf files to write')
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.make_conf.', dir=output_dir) as tmp:
        stage, previous = Path(tmp), {}
        files = list(confs) + ['conf.txt']
        for name in files:
            target = output_dir / name
            if Path(name).name != name or target.is_symlink() or (target.exists() and not target.is_file()):
                raise ValueError(f'refusing to replace non-regular output: {target}')
            previous[name] = (target.read_bytes(), target.stat().st_mode & 0o777) if target.exists() else None
            with open(stage / name, 'w', encoding='utf-8') as out:
                if name == 'conf.txt':
                    write_conf_list(out, conf_list)
                else:
                    write_conf(out, confs[name])
            os.chmod(stage / name, previous[name][1] if previous[name] else 0o644)
        published = []
        try:
            for name in files:
                os.replace(stage / name, output_dir / name)
                published.append(name)
        except OSError:
            for name in reversed(published):
                if previous[name] is None:
                    (output_dir / name).unlink()
                else:
                    restore = stage / ('restore_' + name)
                    restore.write_bytes(previous[name][0])
                    os.chmod(restore, previous[name][1])
                    os.replace(restore, output_dir / name)
            raise
    return len(confs)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map-dir', type=Path, default=MAP_DIR)
    parser.add_argument('--output-dir', type=Path, default=conf_dir)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args(argv)
    try:
        conf_list, gaps = build_conf_plans(args.map_dir)
        for first, last, missing in gaps:
            print(f'warning: no conf for runs {first}-{-1 if last is None else last}; missing {", ".join(missing)}', file=sys.stderr)
        if args.dry_run:
            write_conf_list(sys.stdout, conf_list)
        else:
            count = write_outputs(conf_list, args.output_dir)
            print(f'[OK] regenerated {count} confs and conf.txt ({len(conf_list)} intervals)')
    except (CalibRunMapError, ValueError, OSError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
