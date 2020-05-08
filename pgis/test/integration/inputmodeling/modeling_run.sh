
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Modeling-based selection\nRunning $binary"
extrapDef="${testNo/ipcg/json}"
echo "${binary} -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo"
${binary} --debug 2 -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo

