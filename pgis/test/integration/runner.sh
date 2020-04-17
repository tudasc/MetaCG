#!/usr/bin/env bash

buildDirParam=$1
buildDir="${buildDirParam:-$PWD/../../build}"

tests=( 001 002 003 004 )

echo "Running tests with build directory: $buildDir"

rm -rf $PWD/out && mkdir $PWD/out
rm -f /tmp/pgis_integration_test_res.txt
rm -f /tmp/pgis_integration_test_bl.txt

fails=0

for testNo in "${tests[@]}"; do
	thisFail=0
	testFile="${testNo}_run.sh"
	bash $testFile $buildDir 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	cat $PWD/out/instrumented-${testNo}.txt | sort | uniq > /tmp/pgis_integration_test_res.txt
	cat $PWD/input/${testNo}.afl | sort | uniq > /tmp/pgis_integration_test_bl.txt
	diff -q /tmp/pgis_integration_test_bl.txt  /tmp/pgis_integration_test_res.txt 2>&2 > /dev/null
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

