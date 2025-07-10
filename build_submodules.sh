#!/usr/bin/env bash
#"""
# File: build_submodules.sh
# License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/MetaCG/LICENSE.txt
# Description: Helper script to build the git submodules useed in MetaCG.
#"""
set -euo pipefail

function log()
{
  echo "[MetaCG] $1"
}

function err_exit()
{
  echo "[MetaCG] Error: $1" >&2
  exit 1
}

# python version
set +e
python_bin=""
for cmd in python3.8 python3.9 python3.10 python3; do
  if command -v "$cmd" >/dev/null 2>&1; then
    python_bin="$cmd"
    break
  fi
done
set -e

scriptdir="$(cd "$(dirname "$0")" && pwd -P)"
extsourcedir="$scriptdir/extern/src"
extinstalldir="$scriptdir/extern/install"
venv_dir="$scriptdir/.venv"

mkdir -p "$extsourcedir" "$extinstalldir"

# TODO Make this actually working better!
# Allow configure options (here for Score-P, bc I want to build it w/o MPI)
parallel_jobs="${1:-$(nproc)}"
add_flags="${2:-}"

log "Setting up python venv"
if [ -f "$venv_dir/bin/activate" ]; then
  log "python venv already set up. Skipping."
  source "$venv_dir/bin/activate" || err_exit "Activating python venv failed"
else
  "$python_bin" -m venv "$venv_dir" || err_exit "Creating python venv failed"
  source "$venv_dir/bin/activate" || err_exit "Activating python venv failed"
fi
pip install --upgrade pip
pip install pytest pytest-cmake || err_exit "Installing pymetacg test dependencies failed."

log "Building cubelib"
if [ -f "$extinstalldir/cubelib/bin/cubelib-config" ]; then
  log "cubelib is already built. Skipping."
else
  cubelib_dir="$extsourcedir/cubelib"
  cubelib_tarball="$cubelib_dir/cubelib-4.5.tar.gz"
  cubelib_build="$cubelib_dir/cubelib-4.5/build"
  mkdir -p "$cubelib_dir"

  if [ ! -f "$cubelib_tarball" ]; then
    log "Downloading cubelib"
    wget -O "$cubelib_tarball" "http://apps.fz-juelich.de/scalasca/releases/cube/4.5/dist/cubelib-4.5.tar.gz" || err_exit "cubelib download failed"
  fi

  tar -xzf "$cubelib_tarball" -C "$cubelib_dir"
  rm -rf "$cubelib_build"
  mkdir -p "$cubelib_build"

  (
    cd "$cubelib_build"
    ../configure --prefix="$extinstalldir/cubelib" || err_exit "Configuring cubelib failed"
    make -j "$parallel_jobs" || err_exit "Building cubelib failed"
    make install || err_exit "Installing cubelib failed"
  )

  log "cubelib built and installed successfully"
fi
