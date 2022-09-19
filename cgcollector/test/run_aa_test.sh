#!/usr/bin/env bash

. ./testBase.sh

#if [ command -v $testerExe ]; then
if [[ $(type -P $testerExe) ]]; then
  echo "The CGSimpleTester binary (cgsimpletester) could not be found in path, testing with relative path."
fi
stat ../../${build_dir}/cgcollector/test/mcgtester >>log/testrun.log 2>&1
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../${build_dir}/test. Aborting test. Failure! Please build the tester first."
  exit 1
else
  testerExe=../../${build_dir}/cgcollector/test/mcgtester
fi

if [[ $(type -P $cgcollectorExe) ]]; then
  echo "No cgcollector in PATH. Trying relative path ../${build_dir}/tools"
fi
stat ../../${build_dir}/cgcollector/tools/cgcollector >>log/testrun.log 2>&1
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../${build_dir}/tools. Aborting test. Failure! Please build the collector first."
  exit 1
else
  cgcollectorExe=../../${build_dir}/cgcollector/tools/cgcollector
fi

if [[ $(type -P $cgmergeExe) ]]; then
  echo "No cgcollector in PATH. Trying relative path ../${build_dir}/tools"
fi
stat ../../${build_dir}/cgcollector/tools/cgmerge >>log/testrun.log 2>&1
if [ $? -eq 1 ]; then
  echo "The file seems also non-present in ../${build_dir}/tools. Aborting test. Failure! Please build the collector first."
  exit 1
else
  cgmergeExe=../../${build_dir}/cgcollector/tools/cgmerge
fi

# Multi-file tests
multiTests=(0042 0043 0044 0050 0053 0060)

# Multi-file aa tests
multiTestsAA=(0070 0071 0072 0214 0240 0241)


fails=0

# Single File
echo " --- Running single file tests [file format version 2.0 with Alias Analysis]---"
echo " --- Running basic tests ---"
testGlob="./input/singleTU/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Single File and full Ctor/Dtor coverage
echo -e "\n --- Running single file full ctor/dtor tests ---"
testGlob="./input/allCtorDtor/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Single File and functionPointers
echo -e "\n --- Running single file functionPointers tests ---"
testGlob="./input/functionPointers/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Single File metaCollectors
echo -e "\n --- Running single file metaCollectors tests ---"
testGlob="./input/metaCollectors/numStatements/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Single File virtualCalls
echo -e "\n --- Running single file virtualCalls tests ---"
testGlob="./input/virtualCalls/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Single File AA
echo -e "\n --- Running single file alias analyis tests ---"
testGlob="./input/singleTUAA/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Single file test failures: $fails"

# Multi File
echo -e "\n --- Running multi file tests with AA ---"
for tc in "${multiTests[@]}"; do
  echo "Running test ${tc}"
  # Input files
  applyFileFormatTwoToMultiTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Multi file test failures: $fails"
for tc in "${multiTestsAA[@]}"; do
  echo "Running test ${tc}"
  # Input files
  applyFileFormatTwoToMultiTUWithAA ${tc}
  fail=$?
  fails=$((fails + fail))
done
echo "Multi file test failures: $fails"

echo -e "$fails failures occured when running tests"
exit $fails
