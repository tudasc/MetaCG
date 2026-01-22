#!/usr/bin/env bash

# Concretize Spack environment at CWD and check whether everything is installed

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ "$FORCE_RECONCRETIZE" = "1" ]; then
    rm spack.lock
fi

spack concretize || exit 1

spack python ${SCRIPT_DIR}/check_spack_env.py || exit 1

spack env view regenerate || exit 1

exit 0
