#!/usr/bin/env python3

import os
import sys
from typing import Tuple, Optional



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

# Directry = f"{directory}"
# production_run = f"{Production_run}"
runlist = f"../runmanager/runlist/{name}"
production_run = "../mode-param/Production_full.txt"
magnetoff_run = "../mode-param/Magnet_off.txt"
pedestal_run = "../mode-param/Pedestal.txt"
Directry = "/group/had/sks/HYPS/rawdata/"
bin = "./bin/{program}"

def return_confname(run_no:int):
    conf_txt = f"../param/conf/conf.txt"
    nearest_num = -1
    nearest_conf = None
    with open(conf_txt) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            parts = s.split()
            try:
                num = int(parts[0])
            except ValueError:
                continue
            if num <= run_no and num > nearest_num:
                nearest_num = num
                nearest_conf = parts[1]
    return nearest_conf

def return_production_run(start:int, end:int) -> list[int]:
    result = []
    if start is None or end is None:
        return []
    with open(production_run) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            try:
                r = int(s)
            except ValueError:
                continue
            if start <= r <= end:
                result.append(r)
    return result

def return_magnetoff_run(start:int, end:int) -> list[int]:
    result = []
    if start is None or end is None:
        return []
    with open(magnetoff_run) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            try:
                r = int(s)
            except ValueError:
                continue
            if start <= r <= end:
                result.append(r)
    return result

def return_pedestal_run(start:int, end:int) -> list[int]:
    result = []
    if start is None or end is None:
        return []
    with open(pedestal_run) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            try:
                r = int(s)
            except ValueError:
                continue
            if start <= r <= end:
                result.append(r)
    return result

def main():
    if Flag == "production":
        runs = return_production_run(start_run, end_run)
    elif Flag == "magoff":
        runs = return_magnetoff_run(start_run, end_run)
    elif Flag == "ped":
        runs = return_pedestal_run(start_run, end_run)
    
    with open(runlist, 'a') as out:
        for r in runs:
            conf = return_confname(r)
            if conf is None:
                continue
            Number = f"run{r:05d}"
            out.write(f"  {Number}{program}:\n")
            out.write(f"    bin: ./bin/{program}\n")
            out.write(f"    conf:  ../../param/pro/conf/{conf}\n")
            out.write("\n")
        print("Making runlist.yml is Finished")
        
if __name__ == "__main__":
    main()
