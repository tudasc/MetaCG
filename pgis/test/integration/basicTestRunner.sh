#!/usr/bin/env bash

. base.sh

fails=0

buildDirParam=build # Can be changed via -b
timeStamp=$(date +%s)

while getopts ":b:h" opt; do
  case $opt in
    b)
      if [ -z $OPTARG ]; then
        echo "no build directory given, assuming \"build\""
      fi
      buildDirParam=$OPTARG
      ;;
    h)
      echo "use -b to provide a build directory NAME"
      echo "use -h to print this help"
      exit 0
      ;;
    \?)
      echo "Invalid option -$OPTARG"
      exit 1
      ;;
  esac
done

: ${CI_CONCURRENT_ID:=$timeStamp}

buildDir="../../../${buildDirParam}/pgis"
testExec=${buildDir}/tool/pgis_pira
testSuite='basic'
logDir=$PWD/logging
logFile=${logDir}/${testSuite}-${CI_CONCURRENT_ID}.log
outDir=$PWD/outBasic-${CI_CONCURRENT_ID}

if [ ! -d ${logDir} ]; then
  mkdir ${logDir}
fi

echo "Running Tests with build directory: ${buildDir}"

for testFile in ./ipcgread/*.ipcg; do
	$testExec --out-file $outDir --static $testFile >${logFile} 2>&1
	errCode=$?
	check_and_print $fails $testFile $errCode
	fails=$?

	echo "Running PGIS base test $testNo"
done

echo "Failed tests: ${fails}"
exit ${fails}
