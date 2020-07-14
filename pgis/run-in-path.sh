#!/bin/bash


CCG=./CubeCallGraphTool
# output is dumped here
SPEC_OUTPUT=spec-output-stats
# cubex profiles of spec benchmarks
SPEC_PATH=spec-centos

rm -rf $SPEC_OUTPUT
mkdir $SPEC_OUTPUT

BENCHMARKS=`find $SPEC_PATH -name "*.cubex"`

echo $BENCHMARKS

for bm in $BENCHMARKS ;do
	FILE=$(basename $bm)
	
	echo "running $SPEC_OUTPUT/$FILE.log"

	$CCG $SPEC_PATH/$FILE $1 &> $SPEC_OUTPUT/$FILE.log

	# mv Instrument-callgraph.dot $SPEC_OUTPUT/$FILE.dot

	# generate the dot
#	dottopng.sh Instrument-callgraph.dot spec-png/$FILE.png
done

