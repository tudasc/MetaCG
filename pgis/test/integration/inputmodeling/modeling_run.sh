
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Modeling-based selection\nRunning $binary"
extrapDef="${testNo/ipcg/json}"
echo "${binary} -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo"
${binary} --debug 2 -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$testNo

echo -e "\n [ ------------- EXPORT FEATURE ----------------- ]\n"

outfile=${testNo/ipcg/2.ipcg}
cp ${testNo} ${outfile}
${binary} --debug 2 -x -o $outDir -e ${PWD}/${extrapDef} ${PWD}/$outfile
#rm ${PWD}/${outfile}

