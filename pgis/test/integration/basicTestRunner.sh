#!/usr/bin/env bash

. base.sh

fails=0

buildDirParam=$1
buildDir="${buildDirParam:-$PWD/../../build}"
testExec=${buildDir}/tool/pgis_pira
testSuite='basic'
logDir=$PWD/logging
logFile=${logDir}/${testSuite}.log


for testFile in ./ipcgread/*.ipcg; do
	$testExec --static $testFile >${logFile} 2>&1
	errCode=$?
	check_and_print $fails $testFile $errCode
	fails=$?

	echo "Running PGIS base test $testNo"
done

echo "Failed tests: ${fails}"
