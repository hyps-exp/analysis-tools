#!/usr/bin/env python3

import os
from pathlib import Path
import sys
from typing import Tuple, Optional
import subprocess

mode_txt = "../mode-param/param_mode.txt"
conf_txt = "../param/conf/conf.txt"

def get_arg(i, default=None, cast=str):
    try:
        return cast(sys.argv[i])
    except (IndexError, ValueError):
        return default

run = get_arg(1, default=None, cast=int)

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

def return_confname(run_no:int) ->Tuple[Optional[int], Optional[str]]:
    nearest_num = -1
    nearest_conf = None
    with open(mode_txt) as f:
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
                nearest_conf = f"analyzer_hyps_common_run{num:05d}.conf"
    return nearest_num, nearest_conf

def confirm_conf(file_name):
    file_path = Path("../param/conf/") / file_name
    if Path(file_path).is_file():
        print(f"指定のconf fileは存在します: {file_name}")
        sys.exit(0)
        
def main():
    run_no, conf_name = return_confname(run)
    confirm_conf(conf_name)
    with open(mode_txt, "r") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            cols = s.split()
            run_num = int(cols[0])
            if run_num != run_no:
                continue
            cmap, dcdrft, dctdc, hdprm, hdphc, user, cftped, cftpos, bgocalib, catchpid = map(int, cols[1:11])
            break

    with open(conf_txt, "a") as out:
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

        missing_msgs = []
        if cmap_param   is None: missing_msgs.append("CMAP")
        if dcdrft_param is None: missing_msgs.append("DCDRFT")
        if dctdc_param  is None: missing_msgs.append("DCTDC")
        if hdprm_param  is None: missing_msgs.append("HDPRM")
        if hdphc_param  is None: missing_msgs.append("HDPHC")
        if user_param   is None: missing_msgs.append("USER")
        if cftped_param   is None: missing_msgs.append("CFTPED")
        if cftpos_param   is None: missing_msgs.append("CFTPOS")
        if bgocalib_param   is None: missing_msgs.append("BGOCALIB")            
        if catchpid_param   is None: missing_msgs.append("CATCHPID")
        
        conf_out = f"../param/conf/{conf_name}"
        out.write(f"{run_no} {conf_name}\n")
        
        with open(conf_out, 'w') as conf:
            conf.write(f"#\n")
            conf.write(f"# analyzer_hyps_common_{run_no:05d}.conf\n")
            conf.write(f"#\n\n")
            conf.write(f"UNPACK:         ../UNPACK/unpacker_online.xml\n")
            conf.write(f"DIGIT:          ../DIGIT/digit_data_hyps_20250419.xml\n")
            conf.write(f"CMAP:           ../CMAP/{cmap_param}\n")
            conf.write(f"BH2FLT:         ../BH2FLT/BH2Filter_20180603\n")
            conf.write(f"TAGPLMTH:       ../TAGPLMTH/TAGPLMatchParam_hyps_0\n")
            conf.write(f"DCGEO:          ../DCGEO/DCGeomParam_hyps_common\n")
            conf.write(f"DCDRFT:         ../DCDRFT/{dcdrft_param}\n")
            conf.write(f"DCTDC:          ../DCTDC/{dctdc_param}\n")
            conf.write(f"HDPRM:          ../HDPRM/{hdprm_param}\n")
            conf.write(f"HDPHC:          ../HDPHC/{hdphc_param}\n")
            conf.write(f"FLDMAP:         ../fieldmap/lepsSD53_std_v12_K18style_extend.dat\n")
            conf.write(f"FLDNMR:         1.\n")
            conf.write(f"FLDCALC:        1.\n")
            conf.write(f"PK18:           1.8\n")
            conf.write(f"K18TM:          ../K18TM/K18MatrixParamD2U_0\n")
            conf.write(f"MATRIX2D1:      ../MATRIX/mtx2d1_e42_GEANT4_Normal.txt\n")
            conf.write(f"MATRIX2D2:      ../MATRIX/mtx2d1_e42_GEANT4_tight.txt\n")
            conf.write(f"MATRIX3D:       ../MATRIX/mtx3d_e42_GEANT4.txt\n")
            conf.write(f"USER:           ../USER/{user_param}\n")
            conf.write(f"CFTPED:         ../CFTPED/{cftped_param}\n")
            conf.write(f"CFTPOS:         ../CFTPOS/{cftpos_param}\n")
            conf.write(f"BGOTEMP:        ../BGOTEMP/run01514.txt\n")
            conf.write(f"BGOCALIB:       ../BGOCALIB/{bgocalib_param}\n")
            conf.write(f"# for normal usage without event selection\n")
            conf.write(f"EVSELECT:       ../EVSELECT/event_dummy.txt\n")
            conf.write(f"CATCHPID:       ../CATCHPID/{catchpid_param}\n")

    if missing_msgs:
        print(f"[OK] wrote {conf_name}  (missing: {', '.join(missing_msgs)})")
    else:
        print(f"[OK] wrote {conf_name}\n")
        subprocess.run([sys.executable, "gitpush.py", "../param/conf", "Make new conf file", ], check=True, )

if __name__ == "__main__":
    main()

