#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

set -euo pipefail

# =================================
# Configuration
# =================================

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../param-maker/CATCH"

# =================================
# Check Makefile
# =================================

echo -e "${CYAN}========== Check Makefile ==========${NC}"

if [[ ! -d "$BUILD_DIR" ]]; then
    echo -e "${RED}[ERROR]${NC} Build directory not found: $BUILD_DIR"
    exit 1
fi

if [[ ! -f "${BUILD_DIR}/Makefile" ]]; then

    if [[ ! -f "${BUILD_DIR}/Makefile.org" ]]; then
        echo -e "${RED}[ERROR]${NC} Makefile.org not found."
        exit 1
    fi

    echo -e "${GREEN}[INFO]${NC} Makefile not found."
    echo -e "${GREEN}[INFO]${NC} Copying Makefile.org to Makefile."

    cp "${BUILD_DIR}/Makefile.org" "${BUILD_DIR}/Makefile"

else
    echo -e "${GREEN}[INFO]${NC} Makefile already exists."
fi

# =================================
# Execute make
# =================================

echo -e "${CYAN}=========== Execute make ===========${NC}"

echo -e "${GREEN}[INFO]${NC} Start make"
if make -C "${BUILD_DIR}" -j; then
    echo -e "${GREEN}[INFO]${NC} make completed successfully"
else
    echo -e "${RED}[ERROR]${NC} make failed (exit code: $?)"
    exit 1
fi

echo -e "${CYAN}========== Build completed ==========${NC}"
