buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Modeling-based selection\nRunning $binary"
extrapDef="${testNo/mcg/json}"
cmd="${binary} --use-cs-instrumentation --metacg-format 2 --debug 1 --out-dir $outDir --parameter-file ${PWD}/parameters.json --extrap ${PWD}/${extrapDef} ${PWD}/$testNo"
echo $cmd
$cmd

#echo -e "\n [ ------------- EXPORT FEATURE ----------------- ]\n"

#outfile=${testNo/ipcg/2.ipcg}
#cp ${testNo} ${outfile}
#${binary} --debug 2 --export --out-file $outDir --extrap ${PWD}/${extrapDef} ${PWD}/$outfile
#rm ${PWD}/${outfile}
