
function check_selection {
  testSuite=$1
	testNo=$(echo $2 | cut -d'.' -f 1)
	outDir=$3
  ciConcurrentId=$4
	aflFileExt=""

	cat ${outDir}/instrumented-${testNo}.txt 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "No result file"
		return 255
	fi

	cat ${PWD}/${testNo}.afl > /dev/null 2>&1 
	if [ $? -eq 0 ]; then
		aflFileExt="afl"
	else
	  cat ${PWD}/${testNo}.spl > /dev/null 2>&1 
		if [ $? -eq 0 ]; then
			aflFileExt="spl"
		fi
	fi
	if [ -z "$aflFileExt" ]; then
		echo "No awaited-function-list file"
		return 255
	fi

  sortedTmpICFile="/tmp/pgis_temp_${testSuite}_res-$ciConcurrentId.txt"
  sortedTmpBLFile="/tmp/pgis_temp_${testSuite}_bl-$ciConcurrentId.txt"

	cat ${outDir}/instrumented-${testNo}.txt | sort | uniq > $sortedTmpICFile
	cat ${PWD}/${testNo}.${aflFileExt} | sort | uniq > $sortedTmpBLFile
	diff -q $sortedTmpICFile $sortedTmpBLFile 2>&1 > /dev/null
  resultOfTest=$?
	if [ $resultOfTest -eq 0 ]; then
	  rm ${outDir}/instrumented-${testNo}.txt
	fi
  rm $sortedTmpBLFile $sortedTmpICFile
	return $resultOfTest
}


function error_exit {
	echo "An error occured with argument $1"
	exit -1
}

function check_and_print {
	fails=$1
	testFile=$2
	errCode=$3
	thisFail=0
	failStr=' PASS'

  if [ ${errCode} -ne 0 ]; then
    echo "Error: ${errCode}"
		fails=$((fails+1))
		failStr=' FAIL'
	fi

	echo "Test ${testFile} | ${failStr}"

	return ${fails}
}

