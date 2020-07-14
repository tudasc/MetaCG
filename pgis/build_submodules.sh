#! /usr/bin/env bash
#"""
# File: build_submodules.sh
# License: Part of the PIRA project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/jplehr/pira/LICENSE.txt
# Description: Helper script to build the git submodules useed in PIRA.
#"""

scriptdir="$( cd "$(dirname "$0")" ; pwd -P )"
mkdir -p $scriptdir/deps/src
mkdir -p $scriptdir/deps/install
extsourcedir=$scriptdir/deps/src
extinstalldir=$scriptdir/deps/install

# TODO Make this actually working better!
# Allow configure options (here for Score-P, bc I want to build it w/o MPI)
parallel_jobs="$1"
add_flags="$2"

# Extra-P (https://www.scalasca.org/software/extra-p/download.html)
echo "[PIRA] Building Extra-P (for PIRA II modeling)"
echo "[PIRA] Getting prerequisites ..."
pip3 install --user PyQt5 2>&1 > /dev/null
pip3 install --user matplotlib 2>&1 > /dev/null
if [ $? -ne 0 ]; then
	echo "[PIRA] Installting Extra-P dependencies failed."
	exit 1
fi

mkdir -p $extsourcedir/extrap
cd $extsourcedir/extrap

# TODO check if extra-p is already there, if so, no download / no build?
if [ ! -f "extrap-3.0.tar.gz" ]; then
    echo "[PIRA] Downloading and building Extra-P"
    wget http://apps.fz-juelich.de/scalasca/releases/extra-p/extrap-3.0.tar.gz
fi
tar xzf extrap-3.0.tar.gz
cd ./extrap-3.0
rm -rf build
mkdir build && cd build
# On my Ubuntu machine, the locate command is available, on the CentOS machine it wasn't
# TODO This should be done just a little less fragile
command -v locate "/Python.h"
if [ $? -eq 1 ]; then
	pythonheader=$(dirname $(which python))/../include/python3.7m
else
	pythonlocation=$(locate "/Python.h" | grep "python3.")
	if [ -z $pythonlocation ]; then
	  pythonheader=$(dirname $(which python))/../include/python3.7m
	else
    pythonheader=$(dirname $pythonlocation)
	fi
fi
echo "[PIRA] Found Python.h at " $pythonheader
../configure --prefix=$extinstalldir/extrap CPPFLAGS=-I$pythonheader 2>&1 > /dev/null
if [ $? -ne 0 ]; then
	echo "[PIRA] Configuring Extra-P failed."
	exit 1
fi
make -j $parallel_jobs 2>&1 > /dev/null
if [ $? -ne 0 ]; then
	echo "[PIRA] Building Extra-P failed."
	exit 1
fi
make install 2>&1 > /dev/null

# CXX Opts
echo "[PIRA] Getting cxxopts library"
cd $extsourcedir
if [ ! -d "$extsourcedir/cxxopts" ]; then
    git clone https://github.com/jarro2783/cxxopts cxxopts 2>&1 > /dev/null
fi
cd cxxopts
echo "[PIRA] Select release branch 2_1 for cxxopts."
git checkout 2_1 2>&1 > /dev/null

# JSON library
echo "[PIRA] Getting json library"
cd $extsourcedir
if [ ! -d "$extsourcedir/json" ]; then
    git clone https://github.com/nlohmann/json json 2>&1 > /dev/null
fi

