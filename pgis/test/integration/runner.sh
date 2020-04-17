#!/usr/bin/env bash

# include basic functions
. base.sh

testSuite=$1
buildDirParam=$2
buildDir="${buildDirParam:-$PWD/../../build}"
outDir=$PWD/out$testSuite
logDir=$PWD/logging
logFile=${logDir}/${testSuite}.log

rm -rf $outDir && mkdir $outDir
rm -rf $logDir && mkdir ${logDir}

cd input$testSuite || error_exit "cd input$testSuite"


echo "Running $testSuite tests with build directory: $buildDir"
fails=0

for testNo in *.ipcg; do
	if [ "$testNo" == "*.ipcg" ]; then
		echo "No tests to run"
#		exit $fails
	fi
	echo "Running $testNo"
	thisFail=0

	bash "${testSuite}_run.sh" $buildDir $outDir $testNo 2>&1 >> "$logFile"
	#bash "${testSuite}_run.sh" $buildDir $outDir $testNo

	echo -e "\n[ --------------------------------- ] \n" >> "$logFile"

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	
	check_selection $testSuite $testNo $outDir

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	if [ $thisFail -eq 1 ]; then
		failStr=' FAIL'
	else
		failStr=' PASS'
	fi
	echo "Running PGIS Integration Test $testNo : $testFile | $failStr"
done

echo "Failed tests: $fails"
exit $fails

