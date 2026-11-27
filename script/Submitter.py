#!/usr/bin/env python3
"""Launch one do_run.sh per non-None list in separate tmux sessions.

    python3 Submitter.py 7035 DC_make
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from uuid import uuid4

from calib_run_map import find_entry, param_filename, return_production_run
from make_runlist import return_confname

TARGET_SCRIPT = Path(__file__).resolve().with_name("do_run.sh")

def build_params(number: int, key: str):
    if key == "DC_make":
        entry = find_entry("SDC", "DCTDC", number)        
        dir_run = param_filename("DCTDC", entry.filename_run)
        first_run = entry.input_first
        last_run = entry.input_last        
        return dir_run, ["SdcInTracking", first_run, last_run, dir_run, "others"], ["SdcOutTracking", first_run, last_run, dir_run, "others"], None
    elif key == "DC_check":
        entry = find_entry("SDC", "DCTDC", number)        
        dir_run = param_filename("DCTDC", entry.filename_run)
        first_run = entry.input_first
        last_run = entry.input_last        
        return dir_run, ["SdcInTracking", first_run, last_run, dir_run, "others"], ["SdcOutTracking", first_run, last_run, dir_run, "others"], ["HypsTracking", first_run, last_run, dir_run, "production"]
    elif key == "TOF_ped": # example
        entry = find_entry("TOF", "HDPRM", number)        
        dir_run = param_filename("HDPRM", entry.filename_run)
        first_run = entry.input_first
        last_run = entry.input_last
        pedestal_run = entry.pedestal_run        
        return dir_run, ["Hodoscope", first_ru, last_run, dir_run, "others"]
    else:
        raise ValueError(f"Unsupported key: {key}")


def prepare_jobs(*lists):
    jobs = []
    for data in lists:
        if data is None:
            continue
        if not isinstance(data, (list, tuple)) or len(data) != 5:
            raise ValueError("Each job must contain [program, first_run, last_run, output_name, run_type]")
        program, first, last, output, run_type = data
        if not isinstance(program, str) or not program or any(c.isspace() for c in program):
            raise ValueError(f"Invalid program: {program!r}")
        if (type(first) is not int or type(last) is not int or
                first < 0 or last < first):
            raise ValueError(f"Invalid input run range: {first!r}..{last!r}")
        if not isinstance(output, str) or not output:
            raise ValueError("An output directory name is required")
        if run_type not in ("production", "others"):
            raise ValueError(f"Invalid run type: {run_type!r}")
        jobs.append(list(data))
    if not jobs:
        raise ValueError("No jobs to launch")
    return jobs


def check_inputs(jobs):
    checked = set()
    for program, first, last, output, run_type in jobs:
        runs = (return_production_run(first, last) if run_type == "production"
                else range(first, last + 1))
        if not runs:
            raise ValueError(f"No {run_type} runs in {first}..{last}")
        for run in runs:
            if run not in checked:
                if return_confname(run) is None:
                    raise ValueError(f"No conf.txt entry covers run {run}")
                checked.add(run)


def launch_tmux(run, *lists, dry_run=False):
    jobs = prepare_jobs(*lists)
    worker = TARGET_SCRIPT
    if not worker.is_file():
        raise FileNotFoundError(worker)
    if not dry_run:
        check_inputs(jobs)
        if shutil.which("tmux") is None:
            raise ValueError("tmux was not found")
    batch = uuid4().hex[:8]
    worker_path = os.pathsep.join(dict.fromkeys(os.get_exec_path()))
    sessions = []
    for i, data in enumerate(jobs, start=1):
        session = f"job_{run}_{batch}_{i}"
        command = shlex.join([
            "env", f"PATH={worker_path}",
            f"PYTHON_EXECUTABLE={sys.executable}", "bash", str(worker),
            *(str(value) for value in data),
        ])
        if dry_run:
            print(shlex.join(["bash", str(worker), *(str(value) for value in data)]))
            continue
        subprocess.run([
            "tmux", "new-session", "-d", "-s", session,
            "-c", str(worker.parent), command,
        ], check=True)
        sessions.append(session)
        print("起動:", session, flush=True)
    return sessions


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=int)
    parser.add_argument("key")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without starting jobs")
    args = parser.parse_args()
    try:
        output_name, *lists = build_params(args.run, args.key)
        launch_tmux(args.run, *lists, dry_run=args.dry_run)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
