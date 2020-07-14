#!/bin/bash


CCG=./CubeCallGraphTool
# output is dumped here
S_OUT=spec-output-stats
# cubex profiles of spec benchmarks
S_IN=spec-centos

rm -rf $S_OUT
mkdir $S_OUT

PARAMS="--mangled --samples-file"

while [[ $# > 0 ]]
do
key="$1"

case $key in
    -s|--samples)
    PARAMS+=" --samples $2"
	shift # past argument
    ;;
    -i|--ignore-sampling)
    PARAMS+=" --ignore-sampling"
    ;;
    -t|--tiny)
    PARAMS+=" --tiny"
    ;;
    -f|--samples-file)
    PARAMS+=" --samples-file $2"
    ;;
    *)
    # unknown option
    ;;
esac
shift # past argument or value
done

$CCG $S_IN/429.mcf.clang.cubex        -g -h 105 -r 230.6 $PARAMS 2>&1 | tee $S_OUT/429.mcf.clang.log
$CCG $S_IN/433.milc.clang.cubex       -g -h 105 -r 418.8 $PARAMS 2>&1 | tee $S_OUT/433.milc.clang.log
$CCG $S_IN/444.namd.clang.cubex       -g -h 105 -r 425.7 $PARAMS 2>&1 | tee $S_OUT/444.namd.clang.log
$CCG $S_IN/450.soplex.clang.cubex     -g -h 105 -r 102.4 $PARAMS 2>&1 | tee $S_OUT/450.soplex.clang.log
$CCG $S_IN/456.hmmer.clang.cubex      -g -h 105 -r 332.6 $PARAMS 2>&1 | tee $S_OUT/456.hmmer.clang.log
$CCG $S_IN/458.sjeng.clang.cubex      -g -h 105 -r 508.8 $PARAMS 2>&1 | tee $S_OUT/458.sjeng.clang.log
$CCG $S_IN/462.libquantum.clang.cubex -g -h 105 -r 396.8 $PARAMS 2>&1 | tee $S_OUT/462.libquantum.clang.log
$CCG $S_IN/464.h264ref.clang.cubex    -g -h 105 -r 71.0 $PARAMS 2>&1 | tee $S_OUT/464.h264ref.clang.log
$CCG $S_IN/470.lbm.clang.cubex        -g -h 105 -r 359.0  $PARAMS 2>&1 | tee $S_OUT/470.lbm.clang.log
$CCG $S_IN/473.astar.clang.cubex      -g -h 105 -r 156.0 $PARAMS 2>&1 | tee $S_OUT/473.astar.clang.log
$CCG $S_IN/482.sphinx3.clang.cubex    -g -h 105 -r 521.6 $PARAMS 2>&1 | tee $S_OUT/482.sphinx3.clang.log
$CCG $S_IN/453.povray.gcc.cubex       -h 105 -r 167.1 $PARAMS 2>&1 | tee $S_OUT/453.povray.gcc.log
	
$CCG $S_IN/447.dealII.clang.cubex     -h 105 -r 26.4  $PARAMS 2>&1 | tee $S_OUT/447.dealII.clang.log
$CCG $S_IN/403.gcc.clang.cubex        -h 105 -r 40.1  $PARAMS 2>&1 | tee $S_OUT/403.gcc.clang.log
#	mv Instrument-callgraph.dot $SPEC_OUTPUT/$FILE.dot

#	generate the dot
#	dottopng.sh Instrument-callgraph.dot spec-png/$FILE.png

