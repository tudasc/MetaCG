
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Dynamic Selection\nRunning $binary"
cubeTest="${testNo/ipcg/cubex}"
echo "${binary} -o $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest "
${binary} -o $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest

