

function check_selection {
  testSuite=$1
	testNo=$(echo $2 | cut -d'.' -f 1)
	outDir=$3

	cat ${outDir}/instrumented-${testNo}.txt 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "No result file"
		return 255
	fi
	cat ${PWD}/${testNo}.afl 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "No result file"
		return 255
	fi
	cat ${outDir}/instrumented-${testNo}.txt | sort | uniq > /tmp/pgis_temp_${testSuite}_res.txt
	cat ${PWD}/${testNo}.afl | sort | uniq > /tmp/pgis_temp_${testSuite}_bl.txt
	diff -q /tmp/pgis_temp_${testSuite}_res.txt /tmp/pgis_temp_${testSuite}_bl.txt 2>&1 > /dev/null
	return $?
}


function error_exit {
	echo "An error occured with argument $1"
	exit -1
}
