#!/usr/bin/env bash 

buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira
if [ -z $4 ]; then
	scorepOut=""
else
  scorepOut="--scorep-out"
fi

echo -e "Static Selection\nRunning $binary"
cmd="${binary} ${scorepOut} --out-dir $outDir --use-cs-instrumentation --static ${PWD}/$testNo"
echo $cmd
$cmd

