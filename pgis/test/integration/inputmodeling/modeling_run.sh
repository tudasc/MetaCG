
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Modeling-based selection\nRunning $binary"
extrapDef="${testNo/ipcg/json}"
cmd="${binary} --use-cs-instrumentation --debug 2 --out-file $outDir --parameter-file ${PWD}/parameters.json --extrap ${PWD}/${extrapDef} ${PWD}/$testNo"
echo $cmd
$cmd


echo -e "\n [ ------------- EXPORT FEATURE ----------------- ]\n"

outfile=${testNo/ipcg/2-${CI_CONCURRENT_ID}.ipcg}
cp ${testNo} ${outfile}
cmd="${binary} --use-cs-instrumentation --debug 1 --export --out-file $outDir --parameter-file ${PWD}/parameters.json --extrap ${PWD}/${extrapDef} ${PWD}/$outfile"
echo $cmd
$cmd

#rm ${PWD}/${outfile}

