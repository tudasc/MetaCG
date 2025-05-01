#!/usr/bin/env bash

timeStamp=$(date +%s)
: ${CI_CONCURRENT_ID:=$timeStamp}

mkdir -p log

build_dir=build
generate_gt=0

function merge {
  fail=0
  tc=$1
  suffix="$2"

  ipcgTaFile="${tc}_a${suffix}.ipcg"
  ipcgTbFile="${tc}_b${suffix}.ipcg"
  gtCombFile="${tc}_both${suffix}.gtmcg"

  combFile=${tc}_both${suffix}-${CI_CONCURRENT_ID}.ipcg

  ${PWD}/../../../../${build_dir}/graph/test/integration/CallgraphMerge/mergetester ./input/${ipcgTaFile} ./input/${ipcgTbFile} ./input/${gtCombFile} ./input/${combFile} >>log/testrun.log 2>&1
  mErr=$?

  if [ ${generate_gt} -eq 1 ]; then
    mv ./input/${combFile} ./input/${gtCombFile}
  fi

  if [ ${mErr} -ne 0 ]; then
    fail=$((fail + 1))
    echo "Failure for file: $combFile. Keeping generated file for inspection"
  fi

  return $fail
}

while getopts ":b:h:g" opt; do
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
  g)
    echo "Regenerating ground truth files"
    generate_gt=1
    ;;
  \?)
    echo "Invalid option -$OPTARG"
    exit 1
    ;;
  esac
done

echo "Running integration test for CallgraphMerge"

tests=(0042 0043 0044 0050 0053 0060)
fails=0

for tc in "${tests[@]}"; do
  echo "Running test ${tc}"
  # Input files
  merge ${tc} ""
  fail=$?
  fails=$((fails + fail))
done

echo "Running integration test for CallgraphMerge with preloaded metadata"

for tc in "${tests[@]}"; do
  echo "Running test ${tc} (preload)"
  # Input files
  LD_PRELOAD="${PWD}/../../../../${build_dir}/graph/test/integration/CallgraphMerge/libMergeMD.so" merge "${tc}" "_preload"
  fail=$?
  fails=$((fails + fail))
done
echo "Test failures: $fails"

exit $fails
