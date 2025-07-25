#!/bin/bash

function pretty {
	python3 -m json.tool < "$1" > "$1".json
	mv "$1".json "$1"
}

export OMPI_CXX=$(which clang++)

build_dir=build # default
debug=0

while getopts ":b:hd" opt; do
	case $opt in
		b)
			if [ -z "$OPTARG" ]; then
				echo "no build directory given, assuming \"build\""
			fi
			build_dir=$OPTARG
			;;
		h)
			echo "use -b to provide build directory NAME"
			echo "use -d to enable debug output"
			echo "use -h to print this help"
			exit 0;
			;;
		d)
			debug=1
			;;
		\?)
			echo "Invalid option -$OPTARG"
			exit 1;
			;;
	esac
done

function log {
	if [ $debug -eq 1 ]; then
		echo "$@"
	fi
}

fails=0
logDir=$PWD/logging
logFile=$logDir/cgpatch-${CI_CONCURRENT_ID}.log
inputRoot=$PWD/input
buildDir=$PWD/../../../../${build_dir}/
cgpatchExe=$buildDir/tools/cgpatch/wrapper/patchcxx
testerExe=$buildDir/tools/cgpatch/test/cgtester
cgmerge2Exe="$buildDir/tools/cgmerge2/cgmerge2"

# load config file, required to check if using MPI
if [ -f "${buildDir}/config.sh" ]; then
  source "${buildDir}/config.sh"
fi

if [ ! -d "${logDir}" ]; then
	mkdir "${logDir}"
fi
echo "" > "${logFile}"

if ! type "$cgpatchExe" > /dev/null; then
	echo "Cannot find $cgpatchExe"
	exit 1
else
	log "Found $cgpatchExe"
fi

function build_and_run_mpi_testcase {
	local testName="$1"
	local testSources="$2"
	local testExe="${testName}.out"
	local testPG="${testName}.pg"
	local testGT="${testName}.gtpg"
	local testMCG="${testName}.mcg"
	local testSCG="${testName}.ipcg"

	export CGPATCH_CG_NAME="${testPG}"

	log "Compiling MPI testcase: $testSources"
	$cgpatchExe mpicxx $testSources -o "$testExe" >> "$logFile"
	if [ $? -ne 0 ]; then
		echo "Compilation failed for $testName"
		fails=$((fails + 1))
		return
	fi

	log "Running with mpirun: $testExe"
	mpirun -np 2 "$testExe" >> "$logFile"
	if [ $? -ne 0 ]; then
		echo "Execution failed for $testExe"
		fails=$((fails + 1))
		return
	fi

	evaluate_and_merge "$testName" "$testPG" "$testGT" "$testMCG" "$testSCG" "$testExe"
}

function build_and_run_regular_testcase {
	local testName="$1"
	local testSources="$2"
	local testExe="${testName}.out"
	local testPG="${testName}.pg"
	local testGT="${testName}.gtpg"
	local testMCG="${testName}.mcg"
	local testSCG="${testName}.ipcg"

	export CGPATCH_CG_NAME="${testPG}"

	log "Compiling regular testcase: $testSources"
	$cgpatchExe mpicxx $testSources -o "$testExe" >> "$logFile"
	if [ $? -ne 0 ]; then
		echo "Compilation failed for $testName"
		fails=$((fails + 1))
		return
	fi

	log "Running: $testExe"
	"$testExe"

	log "Evaluating: $testName"
	evaluate_and_merge "$testName" "$testPG" "$testGT" "$testMCG" "$testSCG" "$testExe"
}

function evaluate_and_merge {
	local testName="$1"
	local testPG="$2"
	local testGT="$3"
	local testMCG="$4"
	local testSCG="$5"
	local testExe="$6"

	log "Running $testerExe $testPG $testGT"
	$testerExe "$testPG" "$testGT" >> "$logFile"
	if [ $? -ne 0 ]; then
		echo "$testPG and $testGT do not match!"
		fails=$((fails + 1))
		return
	fi

	log "Merging with $cgmerge2Exe $testMCG $testGT $testSCG"
	$cgmerge2Exe "$testMCG" "$testGT" "$testSCG" >> "$logFile"
	if [ $? -ne 0 ]; then
		echo "Merging static call-graph for $testName failed"
		fails=$((fails + 1))
		return
	fi

	log "Pretty printing $testPG"
	if [ $? -ne 0 ]; then
		echo "Failure for file: $testPG. Keeping it for inspection"
		pretty "$testPG"
		fails=$((fails + 1))
	else
		log "$testName was successful"
		rm -f "$testExe" "$testPG" "$testMCG"
	fi
}

function handle_testcase {
	local testName="$1"
	local testSources="$2"

	if [[ "$testSources" == *"/mpi/"* ]]; then
		if [ "$CGPATCH_USE_MPI" == "ON" ]; then
			build_and_run_mpi_testcase "$testName" "$testSources"
		else
			log "Skipping MPI testcase $testName because CGPATCH_USE_MPI != ON"
		fi
	else
		build_and_run_regular_testcase "$testName" "$testSources"
	fi
}

function collect_testcases {
	find "$inputRoot" -name '*.cpp' | sed -E 's|.*/([0-9]+)_[^/]+\.cpp|\1|' | sort -u
}

for prefix in $(collect_testcases); do
	testSources=$(find "$inputRoot" -name "${prefix}_*.cpp" | sort)
	firstSource=$(echo "$testSources" | head -n 1)
	testDir=$(dirname "$firstSource")
	testName="${testDir}/${prefix}"

	log "Handling testcase $testName"
	handle_testcase "$testName" "$testSources"
done

echo "Finished running tests. Failures: $fails"
exit $fails
