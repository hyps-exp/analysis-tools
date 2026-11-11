#!/bin/bash


if [ $# -lt 2 ]; then
    echo "$0 <run number> <TOF or other>" 
    exit 1
fi

run="$1"
flag="$2"

if [ "$flag" != "TOF" ] && [ "$flag" != "other" ]; then
    echo "$0 <run number> <TOF or other>" 
    exit 1
fi

flag_num=100
if [ "$flag" = "TOF" ]; then
    flag_num=1
else
    flag_num=0
fi

cd -- "$(dirname -- "$0")" || exit 1

git -C ../param pull --ff-only || exit 1
python3 make_conf.py "$run" || exit 1
python3 make_param_default.py "$run" "$flag_num"


