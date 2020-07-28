#!/usr/bin/env bash

cgcollectorExe=cgcollector
testerExe=cgsimpletester
cgmergeExe=cgmerge


if [ command -v $testerExe ]; then
	echo "The CGSimpleTester binary (cgsimpletester) could not be found in path, testing with relative path."
fi
stat ../build/test/cgsimpletester > /dev/null
if [ $? -eq 1 ]; then
	echo "The file seems also non-present in ../build/test. Aborting test. Failure! Please build the tester first."
	exit 1
else
	testerExe=../build/test/cgsimpletester
fi

if [ command -v $cgcollectorExe ]; then
	echo "No cgcollector in PATH. Trying relative path ../build/tools"
fi
stat ../build/tools/cgcollector > /dev/null
if [ $? -eq 1 ]; then
	echo "The file seems also non-present in ../build/tools. Aborting test. Failure! Please build the collector first."
	exit 1
else
  cgcollectorExe=../build/tools/cgcollector
fi

if [ command -v $cgmergeExe ]; then
	echo "No cgcollector in PATH. Trying relative path ../build/tools"
fi
stat ../build/tools/cgmerge > /dev/null
if [ $? -eq 1 ]; then
	echo "The file seems also non-present in ../build/tools. Aborting test. Failure! Please build the collector first."
	exit 1
else
  cgmergeExe=../build/tools/cgmerge
fi

# Single-file tests
tests=(0001 0002 0003 0004 0005 0006 0007 0008 0009 0010 0011 0012 0013 0014 0015 0016 0017 0018 0019 0020 0021 0022 0023 0024 0025 0026 0027 0028 0029 0030 0031 0032 0033 0034 0035 0036 0037 0038 0039 0040 0041 0045 0046 0047 0048 0049 0051 0052 0100 0101 0102 0103)

# Multi-file tests
multiTests=(0042 0043 0044 0050)

fails=0

# Single File 
echo "Running single file tests"
for tc in ${tests[@]}; do
	echo "Running test ${tc}"
  tfile=$tc.cpp
	gfile=$tc.ipcg
	tgt=$tc.gtipcg

	$cgcollectorExe ./input/$tfile --
	$testerExe ./input/$tgt ./input/$gfile

	if [ $? -ne 0 ]; then 
		echo "Failure for file: $gfile. Keeping generated file for inspection" 
		fails=$((fails + 1))
  else 
		rm ./input/$gfile 
	fi

	echo "Failuers: $fails"

done


# Multi File
echo "Running multi file tests"
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
	$cgcollectorExe ./input/$taFile --
	$cgcollectorExe ./input/$tbFile --

	$testerExe ./input/${ipcgTaFile} ./input/${gtaFile}
	aErr=$?
	$testerExe ./input/${ipcgTbFile} ./input/${gtbFile}
	bErr=$?

	combFile=${tc}_combined.ipcg
	echo "null" > ./input/${combFile}

	${cgmergeExe} ./input/${combFile} ./input/${ipcgTaFile} ./input/${ipcgTbFile} 
	mErr=$?
	${testerExe} ./input/${combFile} ./input/${gtCombFile} 
	cErr=$?

	echo "$aErr or $bErr or $mErr or $cErr"

	if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then 
		echo "Failure for file: $combFile. Keeping generated file for inspection" 
		fails=$((fails + 1))
  else 
		rm ./input/$combFile ./input/${ipcgTaFile} ./input/${ipcgTbFile}
	fi

	echo "Failuers: $fails"

done

echo -e "$fails failures occured when running tests"
exit $fails
