#!/usr/bin/env bash

buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Load imbalance Selection\nRunning $binary"
cubeTest="${testNo/ipcg/cubex}"
cmd="${binary} --out-dir $outDir --cube ${PWD}/$cubeTest --parameter-file ${PWD}/parameters.json --lide 1 --debug 1 ${PWD}/$testNo"
echo $cmd
$cmd

