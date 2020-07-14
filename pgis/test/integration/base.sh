

function check_selection {
  testSuite=$1
	testNo=$(echo $2 | cut -d'.' -f 1)
	outDir=$3
	aflFileExt=""

	cat ${outDir}/instrumented-${testNo}.txt 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "No result file"
		return 255
	fi

	cat ${PWD}/${testNo}.afl 2>&1 > /dev/null
	if [ $? -eq 0 ]; then
		aflFileExt="afl"
	else
	  cat ${PWD}/${testNo}.spl 2>&1 > /dev/null
		if [ $? -eq 0 ]; then
			aflFileExt="spl"
		fi
	fi
	if [ -z "$aflFileExt" ]; then
		echo "No awaited-function-list file"
		return 255
	fi
	cat ${outDir}/instrumented-${testNo}.txt | sort | uniq > /tmp/pgis_temp_${testSuite}_res.txt
	cat ${PWD}/${testNo}.${aflFileExt} | sort | uniq > /tmp/pgis_temp_${testSuite}_bl.txt
	diff -q /tmp/pgis_temp_${testSuite}_res.txt /tmp/pgis_temp_${testSuite}_bl.txt 2>&1 > /dev/null
  resultOfTest=$?
	if [ $resultOfTest -eq 0 ]; then
	  rm ${outDir}/instrumented-${testNo}.txt
	fi
  rm /tmp/pgis_temp_${testSuite}_bl.txt /tmp/pgis_temp_${testSuite}_res.txt 
	return $resultOfTest
}


function error_exit {
	echo "An error occured with argument $1"
	exit -1
}
