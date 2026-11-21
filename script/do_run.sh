#!/bin/bash

if [ $# -ne 4 ]; then
    echo "$0 <program name> <strat run number> <stop run number> <root file directory> <production or others>" 
    exit 1
fi
program="$1"
start=$2
end=$3
root_dir="$4"
run_type="$5"
file_name="runlist_"${program}"_"${run_type}".yml"

if [ "$run_type" != "production" ] && [ "$run_type" != "others" ]; then
    echo "$0 <program name> <strat run number> <stop run number> <production or magoff or ped>"
    exit 1
fi
cd -- "$(dirname -- "$0")" || exit 1

cp ../mode-param/runlist_example.yml ../runmanager/runlist/"$file_name" || exit 1

if [ $# -eq 4 ]; then
    root_name="${program}"
    python3 make_runlist.py "$file_name" "$program" "$start" "$end" "$run_type" "$root_dir" || exit 1
fi

cd ../runmanager || exit 1

./run.py runlist/"$file_name"  || exit 1

sleep 5
