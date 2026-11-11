#!/usr/bin/env python3

import os
import sys
import subprocess
import shutil
from pathlib import Path
from typing import Tuple, Optional
from make_conf import return_confname

mode_txt = "../mode-param/param_mode.txt"
conf_txt = "../param/conf/conf.txt"

def get_arg(i, default=None, cast=str):
    try:
        return cast(sys.argv[i])
    except (IndexError, ValueError):
        return default

run = get_arg(1, default=None, cast=int)
flag = get_arg(2, default=None, cast=int)

def load_param(param_name:str, param_no:int) -> str:
    param_file = f"../mode-param/{param_name}_mode.txt"
    with open(param_file) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            parts = s.split()
            try:
                num = int(parts[0])
            except ValueError:
                continue
            if num == param_no:
                return " ".join(parts[1:])
    return None 

def cp_param(dName, file_name):
    path = Path("../param/")/ dName
    file_path = f"{path}/{file_name}"
    if Path(file_path).is_file():
        return
    shutil.copy(f"{path}/{dName}_default", f"{path}/{file_name}")

def cp_tof_param(dName, file_name, before_file):
    path = Path("../param/")/ dName
    file_path = f"{path}/{file_name}"
    if Path(file_path).is_file():
        return
    if before_file is None:
        sys.exit(f"エラー: TOF用のコピー元となる前の設定がありません: {dName}/{file_name}")
    shutil.copy(f"{path}/{before_file}", f"{path}/{file_name}")
        
def main():
    run_no, conf_name = return_confname(run)
    pre_hdprm = None
    with open(mode_txt, "r") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            cols = s.split()
            run_num = int(cols[0])
            if run_num != run_no:
                if (flag==1):
                    pre_hdprm = int(cols[4])
                continue
            cmap, dcdrft, dctdc, hdprm, hdphc, user, cftped, cftpos, bgocalib, catchpid = map(int, cols[1:11])
            break
    
    prehdprm_param  = load_param("HDPRM",  pre_hdprm)
    cmap_param   = load_param("CMAP",   cmap)
    dcdrft_param = load_param("DCDRFT", dcdrft)
    dctdc_param  = load_param("DCTDC",  dctdc)
    hdprm_param  = load_param("HDPRM",  hdprm)
    hdphc_param  = load_param("HDPHC",  hdphc)
    user_param   = load_param("USER",   user)
    cftped_param = load_param("CFTPED",   cftped)
    cftpos_param = load_param("CFTPOS",   cftpos)
    bgocalib_param   = load_param("BGOCALIB",   bgocalib)
    catchpid_param   = load_param("CATCHPID",   catchpid)

    cp_param("DCDRFT", dcdrft_param)
    cp_param("DCTDC", dctdc_param)
    cp_param("HDPHC", hdphc_param)
    cp_param("CFTPED", cftped_param)
    cp_param("CFTPOS", cftpos_param)
    cp_param("BGOCALIB", bgocalib_param)
    cp_param("CATCHPID", catchpid_param)   

    if(flag==1):
        cp_tof_param("HDPRM", hdprm_param, prehdprm_param)
    else:
        cp_param("HDPRM", hdprm_param)
    
    subprocess.run([sys.executable, "gitpush.py", "../param", "Make initial param", ], check=True, )

if __name__ == "__main__":
    main()

