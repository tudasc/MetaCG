
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira
if [ -z $4 ]; then
	scorepOut=""
else
  scorepOut="--scorep-out"
fi

echo -e "Static Selection\nRunning $binary"
echo "${binary} ${scorepOut} --out-file $outDir --static ${PWD}/$testNo"
${binary} ${scorepOut} --out-file $outDir --use-cs-instrumentation --static ${PWD}/$testNo

