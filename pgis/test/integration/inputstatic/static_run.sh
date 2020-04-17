
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Static Selection\nRunning $binary"
echo "${binary} -o $outDir --static ${PWD}/$testNo"
${binary} -o $outDir --static ${PWD}/$testNo

