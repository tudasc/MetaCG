#!/usr/bin/env bash

# include basic functions
. base.sh

testSuite=$1
buildDirParam=$2
buildDir="${buildDirParam:-$PWD/../../../build/pgis}"
outDir=$PWD/out$testSuite
logDir=$PWD/logging
logFile=${logDir}/${testSuite}.log


rm -rf $outDir && mkdir $outDir
rm -rf $logDir && mkdir ${logDir}

cd input$testSuite || error_exit "cd input$testSuite"


echo "Running $testSuite tests with build directory: $buildDir"
fails=0

for testNoInit in *.afl; do
	echo -e "\n== AFL-based MetaCG v1.0 tests =="
	if [ "$testNoInit" == "*.afl" ]; then
		echo "No tests to run"
#		exit $fails
	fi
	testNo="${testNoInit/afl/ipcg}"
	echo "Running $testNo"
	thisFail=0

	bash "${testSuite}_run.sh" $buildDir $outDir $testNo 2>&1 >> "$logFile"
	#bash "${testSuite}_run.sh" $buildDir $outDir $testNo

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
	echo -e "\n[ --------------------------------- ] \n" >> "$logFile"
	echo "Running PGIS Integration Test $testNo : $testFile | $failStr"
done

for testNoInit in *.afl; do
	echo -e "\n== AFL-based MetaCG v2.0 tests =="
	if [ "$testNoInit" == "*.afl" ]; then
		echo "No tests to run"
#		exit $fails
	fi
	testNo="${testNoInit/afl/mcg}"
	echo "Running $testNo"
	thisFail=0

	#bash "${testSuite}_run_v2.sh" $buildDir $outDir $testNo >> "$logFile" 2>&1
	bash "${testSuite}_run_v2.sh" $buildDir $outDir $testNo

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
	echo -e "\n[ --------------------------------- ] \n" >> "$logFile"
	echo "Running PGIS Integration Test $testNo : $testFile | $failStr"
done

for testNoInit in *.spl; do
	echo -e "\n== SPL-based tests =="
	if [ "$testNoInit" == "*.spl" ]; then
		echo "No test to run"
		echo "So far failed tests $fails"
		exit $fails
	fi
	testNo="${testNoInit/spl/ipcg}"
	echo "Running $testNo"
	thisFail=0

	bash "${testSuite}_run.sh" $buildDir $outDir $testNo "spl" 2>&1 >> "$logFile"
	#bash "${testSuite}_run.sh" $buildDir $outDir $testNo

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
	echo -e "\n[ --------------------------------- ] \n" >> "$logFile"
	echo "Running PGIS Integration Test $testNo : $testFile | $failStr"
done

echo "Failed tests: $fails"
exit $fails

