#!/usr/bin/env bash

# include basic functions
. base.sh

buildDirParam=build # Can be changed via -b
timeStamp=$(date +%s)

while getopts ":t:b:h" opt; do
  case $opt in
    b)
      echo "$OPTARG"
      if [ -z $OPTARG ]; then
        echo "no build directory given, assuming \"build\""
      else
        echo "Using ${OPTARG} as build dir"
        buildDirParam="${OPTARG}"
      fi
      ;;
    t)
      echo "Running testsuite ${OPTARG}"
      testSuite=${OPTARG}
      ;;
    h)
      echo "use -b to provide a build directory NAME"
      echo "use -t to provide a test suite name <static|dynamic|modeling|imbalance>"
      echo "use -h to print this help"
      exit 0
      ;;
    \?)
      echo "Invalid option -$opt"
      exit 1
      ;;
  esac
done

# To run these tests in parallel, we use the CI_CONCURRENT_ID in files that are written
# by any of the tools invoked here. The variables is set automatically by Gitlab Runner
# and in case it is unset, we simply set it to the current timestamp in seconds since epoch
: ${CI_CONCURRENT_ID:=$timeStamp}

buildDir="../../../../${buildDirParam}/pgis"

outDir=$PWD/out$testSuite-${CI_CONCURRENT_ID}
logDir=$PWD/logging-${CI_CONCURRENT_ID}
logFile=${logDir}/${testSuite}-${CI_CONCURRENT_ID}.log

echo "Running Tests with build directory: ${buildDir}"


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

  # We now overwrite logs with each execution. On error, the log is echo'ed inside
  # this script.
	bash "${testSuite}_run.sh" $buildDir $outDir $testNo 2>&1 > "$logFile"
	#bash "${testSuite}_run.sh" $buildDir $outDir $testNo

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	
	check_selection $testSuite $testNo $outDir $CI_CONCURRENT_ID

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	if [ $thisFail -eq 1 ]; then
		failStr=' FAIL'
    # In case of error, print the log file
    echo ">>> ERROR OCCURRED -- Dumping log <<<"
    cat $logFile
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

	bash "${testSuite}_run_v2.sh" $buildDir $outDir $testNo > "$logFile" 2>&1
	#bash "${testSuite}_run_v2.sh" $buildDir $outDir $testNo

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	
	check_selection $testSuite $testNo $outDir $CI_CONCURRENT_ID

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	if [ $thisFail -eq 1 ]; then
		failStr=' FAIL'
    # in case of error, print the log file
    echo ">>> ERROR OCCURRED -- Dumping log <<<"
    cat $logFile
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

	bash "${testSuite}_run.sh" $buildDir $outDir $testNo "spl" 2>&1 > "$logFile"
	#bash "${testSuite}_run.sh" $buildDir $outDir $testNo

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	
	check_selection $testSuite $testNo $outDir $CI_CONCURRENT_ID

	if [ $? -ne 0 ]; then
		fails=$(($fails+1))
		thisFail=1
	fi
	if [ $thisFail -eq 1 ]; then
		failStr=' FAIL'
    # In case of error, print the log file
    cat $logFile
	else
		failStr=' PASS'
	fi
	echo -e "\n[ --------------------------------- ] \n" >> "$logFile"
	echo "Running PGIS Integration Test $testNo : $testFile | $failStr"
done

echo "Failed tests: $fails"
exit $fails

