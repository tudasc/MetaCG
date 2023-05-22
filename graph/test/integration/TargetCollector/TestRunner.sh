#!/usr/bin/env bash

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

test_command=(python3 ../../../../TargetCollector.py \
     -a=".." \
     -g=both \
     -b=build \
     -c="$PWD"/../../../../"${build_dir}"/cgcollector/tools/cgcollector \
     -m="$PWD"/../../../../"${build_dir}"/cgcollector/tools/cgmerge \
     -o="src/wholeProgramCG.ipcg" \
     -t testExe)

echo "${test_command[@]}"
"${test_command[@]}"

diff -q ./src/foo.ipcg ./src/foo.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
	exit 1
fi

diff -q ./src/main.ipcg ./src/main.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
	exit 1
fi

diff -q ./src/wholeProgramCG.ipcg ./src/wholeProgramCG.expected > /dev/null 2>&1
resultOfTest=$?
if [ $resultOfTest -ne 0 ]; then
	exit 1
fi

rm ./src/foo.ipcg
rm ./src/main.ipcg
rm ./src/wholeProgramCG.ipcg
echo "{}" > ./src/wholeProgramCG.ipcg