#!/usr/bin/env bash

cgcollectorExe=cgcollector
testerExe=cgsimpletester
cgmergeExe=cgmerge

mkdir -p log

#if [ command -v $testerExe ]; then
if [[ $(type -P $testerExe) ]]; then
  echo "The CGSimpleTester binary (cgsimpletester) could not be found in path, testing with relative path."
fi
stat ../build/test/cgsimpletester >>log/testrun.log 2>&1 
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../build/test. Aborting test. Failure! Please build the tester first."
  exit 1
else
  testerExe=../build/test/cgsimpletester
fi

if [[ $(type -P $cgcollectorExe) ]]; then
  echo "No cgcollector in PATH. Trying relative path ../build/tools"
fi
stat ../build/tools/cgcollector >>log/testrun.log 2>&1 
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../build/tools. Aborting test. Failure! Please build the collector first."
  exit 1
else
  cgcollectorExe=../build/tools/cgcollector
fi

if [[ $(type -P $cgmergeExe) ]]; then
  echo "No cgcollector in PATH. Trying relative path ../build/tools"
fi
stat ../build/tools/cgmerge >>log/testrun.log 2>&1 
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../build/tools. Aborting test. Failure! Please build the collector first."
  exit 1
else
  cgmergeExe=../build/tools/cgmerge
fi

# Single-file tests
tests=(0001 0002 0003 0004 0005 0006 0007 0008 0009 0010 0011 0012 0013 0014 0015 0016 0017 0018 0019 0020 0021 0022 0023 0024 0025 0026 0027 0028 0029 0030 0031 0032 0033 0034 0035 0036 0037 0038 0039 0040 0041 0045 0046 0047 0048 0049 0051 0052 0100 0101 0102 0103 0201 0202 0203 0204 0205 0206 0207 0208 0209 0210 0211 0212 0214)

# Multi-file tests
multiTests=(0042 0043 0044 0050 0053)

fails=0

# Single File 
echo " --- Running single file tests [file format version 1.0]---"
for tc in ${tests[@]}; do
  echo "Running test ${tc}"
  tfile=$tc.cpp
  gfile=$tc.ipcg
  tgt=$tc.gtipcg

  $cgcollectorExe ./input/singleTU/$tfile -- >>log/testrun.log 2>&1
  cat ./input/singleTU/$gfile | python3 -m json.tool > ./input/singleTU/${gfile}_
  mv ./input/singleTU/${gfile}_ ./input/singleTU/${gfile}
  $testerExe ./input/singleTU/$tgt ./input/singleTU/$gfile >>log/testrun.log 2>&1 

  if [ $? -ne 0 ]; then 
    echo "Failure for file: $gfile. Keeping generated file for inspection" 
    fails=$((fails + 1))
  else 
    rm ./input/singleTU/$gfile 
  fi

done
  echo "Single file test failuers: $fails"

# Single File and full Ctor/Dtor coverage
echo " --- Running single file full ctor/dtor tests ---"
for tc in ./input/allCtorDtor/*.cpp ; do
  echo "Running test ${tc}"
  tfile=$tc
  gfile="${tc/cpp/ipcg}"
  tgt="${tc/cpp/gtipcg}"

  $cgcollectorExe --capture-ctors-dtors $tfile -- >>log/testrun.log 2>&1 
  #$cgcollectorExe --capture-ctors-dtors $tfile -- 
  $testerExe $tgt $gfile >>log/testrun.log 2>&1 

  if [ $? -ne 0 ]; then 
    echo "Failure for file: $gfile. Keeping generated file for inspection" 
    fails=$((fails + 1))
  else 
    rm $gfile 
  fi

done
  echo "Single file test failuers: $fails"


# Multi File
echo " --- Running multi file tests ---"
for tc in ${multiTests[@]}; do
  echo "Running test ${tc}"
  # Input files
  taFile=${tc}_a.cpp
  tbFile=${tc}_b.cpp

  # Result files
  ipcgTaFile="${taFile/cpp/ipcg}"
  ipcgTbFile="${tbFile/cpp/ipcg}"

  # Groundtruth files
  gtaFile="${taFile/cpp/gtipcg}"
  gtbFile="${tbFile/cpp/gtipcg}"
  gtCombFile="${tc}_combined.gtipcg"

  # Translation-unit-local
  $cgcollectorExe ./input/multiTU/$taFile -- >>log/testrun.log 2>&1
  $cgcollectorExe ./input/multiTU/$tbFile -- >>log/testrun.log 2>&1

  $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile} >>log/testrun.log 2>&1
  aErr=$?
  $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile} >>log/testrun.log 2>&1
  bErr=$?

  combFile=${tc}_combined.ipcg
  echo "null" > ./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1 
  mErr=$?
  ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >>log/testrun.log 2>&1 
  cErr=$?

  #echo "$aErr or $bErr or $mErr or $cErr"

  if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then 
    echo "Failure for file: $combFile. Keeping generated file for inspection" 
    fails=$((fails + 1))
  else 
    rm ./input/multiTU/$combFile ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile}
  fi

done

echo "Multi file test failuers: $fails"

echo -e "$fails failures occured when running tests"
exit $fails
