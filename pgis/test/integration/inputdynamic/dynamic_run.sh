
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Dynamic Selection\nRunning $binary"
cubeTest="${testNo/ipcg/cubex}"
cmd="${binary} --use-cs-instrumentation --out-dir $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest"
echo $cmd
$cmd

