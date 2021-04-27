
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Load imbalance Selection\nRunning $binary"
cubeTest="${testNo/ipcg/cubex}"
echo "${binary} --out-file $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest --load-imbalance ${PWD}/config.json --debug 1"
${binary} --out-file $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest --load-imbalance ${PWD}/config.json --debug 1

