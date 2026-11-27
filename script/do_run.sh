#!/bin/bash

if [ "$#" -ne 5 ]; then
    echo "$0 <program name> <start run number> <stop run number> <root file directory> <production or others>"
    exit 1
fi
program="$1"
start="$2"
end="$3"
root_dir="$4"
run_type="$5"
file_name="runlist_${program}_${run_type}_${start}_${end}_$$.yml"

if [ "$run_type" != "production" ] && [ "$run_type" != "others" ]; then
    echo "run type must be production or others"
    exit 1
fi
script_dir=$(cd -- "$(dirname -- "$0")" && pwd) || exit 1
runmanager_dir="$script_dir/../../hyps-analyzer/pro/runmanager"
python_bin="${PYTHON_EXECUTABLE:-python3}"

cp "$script_dir/../mode-param/runlist_example.yml" "$runmanager_dir/runlist/$file_name" || exit 1
"$python_bin" "$script_dir/make_runlist.py" "$file_name" "$program" "$start" "$end" "$run_type" "$root_dir" || exit 1

cd "$runmanager_dir" || exit 1
"$python_bin" ./run.py "runlist/$file_name" || exit 1

sleep 5
