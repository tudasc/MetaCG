#!/usr/bin/env bash

. base.sh

fails=0

buildDirParam=build # Can be changed via -b

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

buildDir="../../../${buildDirParam}/pgis"
testExec=${buildDir}/tool/pgis_pira
testSuite='basic'
logDir=$PWD/logging
logFile=${logDir}/${testSuite}.log

rm -rf $logDir && mkdir ${logDir}

echo "Running Tests with build directory: ${buildDir}"

for testFile in ./ipcgread/*.ipcg; do
	$testExec --static $testFile >${logFile} 2>&1
	errCode=$?
	check_and_print $fails $testFile $errCode
	fails=$?

	echo "Running PGIS base test $testNo"
done

echo "Failed tests: ${fails}"
exit ${fails}
