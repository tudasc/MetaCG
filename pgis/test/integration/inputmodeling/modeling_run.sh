
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Static Selection\nRunning $binary"
extrapDef="${testNo/ipcg/json}"
echo "${binary} -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo"
${binary} -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo

