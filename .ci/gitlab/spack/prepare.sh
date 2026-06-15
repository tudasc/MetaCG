#!/usr/bin/env bash

# Concretize Spack environment at CWD and check whether everything is installed

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ "$FORCE_RECONCRETIZE" = "1" ]; then
    rm spack.lock
fi

spack concretize || exit 1

spack python ${SCRIPT_DIR}/check_spack_env.py || exit 1

spack env view regenerate || exit 1

# symlink flang to flang-new if flang is not in the PATH
if ! command -v flang &>/dev/null; then
    flang_new=$(command -v flang-new) || { echo "flang-new not found in PATH"; exit 1; }
    ln -s "$flang_new" "$(dirname "$flang_new")/flang"
    echo "Created symlink: $(dirname "$flang_new")/flang -> $flang_new"
fi

exit 0
