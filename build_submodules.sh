#! /usr/bin/env bash
#"""
# File: build_submodules.sh
# License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/MetaCG/LICENSE.txt
# Description: Helper script to build the git submodules useed in MetaCG.
#"""

scriptdir="$(
  cd "$(dirname "$0")"
  pwd -P
)"
mkdir -p $scriptdir/extern/src
mkdir -p $scriptdir/extern/install
extsourcedir=$scriptdir/extern/src
extinstalldir=$scriptdir/extern/install

# TODO Make this actually working better!
# Allow configure options (here for Score-P, bc I want to build it w/o MPI)
parallel_jobs="$1"
add_flags="$2"

# Extra-P (https://www.scalasca.org/software/extra-p/download.html)
echo "[MetaCG] Building Extra-P (for PIRA II modeling)"
echo "[MetaCG] Getting prerequisites ..."
pip3 install PyQt5 >/dev/null 2>&1
pip3 install matplotlib >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[MetaCG] Installting Extra-P dependencies failed."
  exit 1
fi

mkdir -p $extsourcedir/extrap
cd $extsourcedir/extrap

# TODO check if extra-p is already there, if so, no download / no build?
if [ ! -f "extrap-3.0.tar.gz" ]; then
  echo "[MetaCG] Downloading and building Extra-P"
  wget http://apps.fz-juelich.de/scalasca/releases/extra-p/extrap-3.0.tar.gz
fi
tar xzf extrap-3.0.tar.gz
cd ./extrap-3.0
rm -rf build
mkdir build && cd build

# python3-config should be the preferred way to get the python include directory,
# but at least with python 3.9 on ubuntu it is a bit buggy and some distributions don't support it at all
pythonheader=$(python3 -c "from sysconfig import get_paths; print(get_paths()[\"include\"])")
if [ -z $pythonheader ]; then
  echo "[MetaCG] Python header not found."
  exit 1
fi
echo "[MetaCG] Found Python.h at " $pythonheader
../configure --prefix=$extinstalldir/extrap CPPFLAGS=-I$pythonheader >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[MetaCG] Configuring Extra-P failed."
  exit 1
fi

make -j $parallel_jobs >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[MetaCG] Building Extra-P failed."
  exit 1
fi

make install >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[MetaCG] Installing Extra-P failed."
  exit 1
fi

mkdir $extinstalldir/extrap/include
cp $extsourcedir/extrap/extrap-3.0/include/* $extinstalldir/extrap/include
