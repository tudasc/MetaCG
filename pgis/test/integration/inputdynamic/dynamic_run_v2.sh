
buildDir=$1
outDir=$2
testNo=$3
binary=${buildDir}/tool/pgis_pira

echo -e "Dynamic Selection\nRunning $binary"
echo -e "For now skipping MetaCG file format 2 test"
cubeTest="${testNo/mcg/cubex}"
#echo "${binary} --out-file $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest "
${binary} --metacg-format 2 --out-file $outDir ${PWD}/$testNo --cube ${PWD}/$cubeTest

