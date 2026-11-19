#!/bin/bash
# Example:
#   ./param_writer.sh 7000 HDPRM RF
#   ./param_writer.sh 7000 HDPRM T0
#   ./param_writer.sh 7000 HDPRM TOF_raw
#   ./param_writer.sh 7000 HDPRM TOF_col
#   ./param_writer.sh 7000 HDPRM RF 7005   # override input_run
#
# See script/param_writer.py for the full argument description.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

exec python3 "${SCRIPT_DIR}/param_writer.py" "$@"
