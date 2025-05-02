buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Dynamic Selection\nRunning $binary"
echo -e "For now skipping MetaCG file format 2 test"
cubeTest="${testNo/mcg/cubex}"
cmd="${binary} --use-cs-instrumentation --metacg-format 2 --out-dir $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest"
echo $cmd
$cmd
