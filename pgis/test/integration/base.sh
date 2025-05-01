#!/usr/bin/env bash

function check_selection {
  testSuite=$1
	testNo=$(echo $2 | cut -d'.' -f 1)
	outDir=$3
  ciConcurrentId=$4
	aflFileExt=""

	cat "${outDir}"/instrumented-"${testNo}".txt >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "No result file"
		return 255
	fi

	cat "${PWD}"/"${testNo}".afl > /dev/null 2>&1 
	if [ $? -eq 0 ]; then
		aflFileExt="afl"
	else
	  cat "${PWD}"/"${testNo}".spl > /dev/null 2>&1 
		if [ $? -eq 0 ]; then
			aflFileExt="spl"
		fi
	fi
	if [ -z "$aflFileExt" ]; then
		echo "No awaited-function-list file"
		return 255
	fi

  sortedTmpICFile="/tmp/pgis_temp_${testSuite}_res-${ciConcurrentId}.txt"
  sortedTmpBLFile="/tmp/pgis_temp_${testSuite}_bl-${ciConcurrentId}.txt"

	sort "${outDir}"/instrumented-"${testNo}".txt | uniq > "${sortedTmpICFile}"
	sort "${PWD}"/"${testNo}"."${aflFileExt}" | uniq > "${sortedTmpBLFile}"
	diff -q "${sortedTmpICFile}" "${sortedTmpBLFile}" >/dev/null 2>&1 
  resultOfTest=$?
	if [ $resultOfTest -eq 0 ]; then
	  rm "${outDir}"/instrumented-"${testNo}".txt
	fi
  rm "${sortedTmpBLFile}" "${sortedTmpICFile}"
	return $resultOfTest
}

function error_exit {
	echo "An error occured with argument $1"
	exit 1
}

function check_and_print {
	fails=$1
	testFile=$2
	errCode=$3
	failStr=' PASS'

  if [ ${errCode} -ne 0 ]; then
    echo "Error: ${errCode}"
		fails=$((fails+1))
		failStr=' FAIL'
	fi

	echo "Test ${testFile} | ${failStr}"

	return ${fails}
}
