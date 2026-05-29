#!/usr/bin/env bash

timeStamp=$(date +%s)
: ${CI_CONCURRENT_ID:=$timeStamp}

build_dir=build # may be changed with opt 'b'
while getopts ":b:h" opt; do
  case $opt in
    b)
      if [ -z $OPTARG ]; then
        echo "no build directory given, assuming \"build\""
      fi
      build_dir=$OPTARG
      ;;
    h)
      echo "use -b to provide a build directory NAME"
      echo "use -h to print this help"
      exit 0
      ;;
    \?)
      echo "Invalid option -$OPTARG"
      exit 1
      ;;
  esac
done

echo "Running integration test for TargetCollector script"

echo "{}" > src/wholeProgramCG-${CI_CONCURRENT_ID}.ipcg
test_command=(python3 ../../../../utils/TargetCollector.py \
     -a=".." \
     -g=both \
     -b=build-${CI_CONCURRENT_ID} \
     -c="$PWD"/../../../../"${build_dir}"/cgcollector/tools/cgcollector \
     -m="$PWD"/../../../../"${build_dir}"/cgcollector/tools/cgmerge \
     -o="src/wholeProgramCG-${CI_CONCURRENT_ID}.ipcg" \
     -t testExe \
     --ci-concurrent-suffix=$CI_CONCURRENT_ID
   )

echo "${test_command[@]}"
"${test_command[@]}"

echo "--- Now running diffs ---"

diff -q ./src/foo.cpp${CI_CONCURRENT_ID}.ipcg ./src/foo.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
  echo "diff for foo failed"
	exit 1
fi

diff -q ./src/main.cpp${CI_CONCURRENT_ID}.ipcg ./src/main.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
  echo "diff for main failed"
	exit 1
fi

diff -q ./src/wholeProgramCG-${CI_CONCURRENT_ID}.ipcg ./src/wholeProgramCG.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
  echo "diff for wholeProgram failed"
	exit 1
fi

rm ./src/foo.cpp${CI_CONCURRENT_ID}.ipcg
rm ./src/main.cpp${CI_CONCURRENT_ID}.ipcg
rm ./src/wholeProgramCG-${CI_CONCURRENT_ID}.ipcg
rm -rf build-${CI_CONCURRENT_ID}
