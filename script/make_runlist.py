#!/usr/bin/env python3

import os
import sys
from pathlib import Path
from typing import Tuple, Optional

from calib_run_map import return_production_run



def get_arg(i, default=None, cast=str):
    try:
        return cast(sys.argv[i])
    except (IndexError, ValueError):
        return default

name            = get_arg(1)
program         = get_arg(2)
start_run       = get_arg(3, default=None, cast=int)
end_run         = get_arg(4, default=None, cast=int)
Flag            = get_arg(5)
dir_root        = get_arg(6)

SCRIPT_DIR = Path(__file__).resolve().parent
RUNMANAGER_DIR = SCRIPT_DIR.parent.parent / "hyps-analyzer/pro/runmanager"
ROOT_DIRECTORY = Path("/group/had/sks/HYPS/param-make/rootfile")

def return_confname(run_no: int, conf_txt=None):
    """Read conf.txt and return the conf name covering run_no, or None.

    Read the new bounded index, or the old two-column index for backward
    compatibility. A bounded index never falls back across a run gap.
    """
    if isinstance(run_no, bool) or not isinstance(run_no, int) or run_no < 0:
        raise ValueError('run must be a nonnegative integer')
    path = Path(conf_txt) if conf_txt is not None else Path(__file__).resolve().parent.parent / 'param/conf/conf.txt'
    records = []
    widths = set()
    for number, line in enumerate(path.read_text().splitlines(), 1):
        fields = line.split('#', 1)[0].split()
        if not fields:
            continue
        if len(fields) not in (2, 3):
            raise ValueError(f'{path}:{number}: expected 2 or 3 columns')
        try:
            first = int(fields[0])
            last = int(fields[1]) if len(fields) == 3 else None
            if last == -1:
                last = None
        except ValueError:
            raise ValueError(f'{path}:{number}: invalid run number') from None
        if first < 0 or (last is not None and last < first):
            raise ValueError(f'{path}:{number}: invalid run interval')
        widths.add(len(fields))
        records.append((first, last, fields[-1]))
    if len(widths) > 1:
        raise ValueError(f'{path}: cannot mix bounded and legacy index rows')
    records.sort(key=lambda row: row[0])
    for before, after in zip(records, records[1:]):
        if (after[0] == before[0] or
                (widths == {3} and (before[1] is None or after[0] <= before[1]))):
            raise ValueError(f'{path}: overlapping or duplicate run intervals')
    for first, last, name in reversed(records):
        if first <= run_no and (last is None or run_no <= last):
            return name
    return None


def main():
    if (len(sys.argv) != 7 or not name or not program or not dir_root or
            start_run is None or end_run is None or
            start_run < 0 or end_run < start_run):
        raise ValueError("Expected: <runlist name> <program> <first run> <last run> <production or others> <output directory>")
    if Flag == "production":
        runs = return_production_run(start_run, end_run)
    elif Flag == "others":
        runs = range(start_run, end_run + 1)
    else:
        raise ValueError("run type must be production or others")
    if not runs:
        raise ValueError(f"No {Flag} runs in {start_run}..{end_run}")

    # Keep numeric directory arguments compatible with the previous script.
    output_name = f"run{int(dir_root):05d}" if dir_root.isdigit() else dir_root
    root_directory = ROOT_DIRECTORY / output_name
    records = []
    for r in runs:
        conf = return_confname(r)
        if conf is None:
            raise ValueError(f"No conf.txt entry covers run {r}")
        records.append((r, conf))

    root_directory.mkdir(parents=True, exist_ok=True)
    runlist = RUNMANAGER_DIR / "runlist" / name
    with open(runlist, 'a') as out:
        for r, conf in records:
            out.write(f"  {r}:\n")
            out.write(f"    bin: ./bin/{program}\n")
            out.write(f"    conf:  ../../param/pro/conf/{conf}\n")
            out.write(f"    root:  {root_directory}\n")
            out.write("\n")
    print("Making runlist.yml is Finished")
        
if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as error:
        sys.exit(f"Error: {error}")
