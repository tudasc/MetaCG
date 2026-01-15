cgcollectorExe=cgcollector2
testerExe=cgdiff
cgmergeExe=cgmerge2
build_dir=build # may be changed with opt 'b'

timeStamp=$(date +%s)

: ${CI_CONCURRENT_ID:="${timeStamp}"}
LOGDIR="$(pwd)/log_${CI_CONCURRENT_ID}"
mkdir $LOGDIR

# Function to invoke the CGCollector with file format version 2 to a target source code
# Param 1: The relative path name to the test case.
# Param 2: Parameter to steer certain features on / off
function applyFileFormatTwoToSingleTU {
  testCaseFile=$1
  addFlags=$2
  fail=0

  # Set up the different data files, we need:
  # - The test case
  # - The groundtruth data for reconciling the CG constructed by MetaCG
  tfile=$testCaseFile
  gfile=${testCaseFile/cpp/ipcg}_$CI_CONCURRENT_ID
  tgt=${testCaseFile/cpp/gtmcg}

  $cgcollectorExe ${addFlags} $tfile --cg-file=$gfile --extra-arg=-std=c++17 -- >> $LOGDIR/testrun.log 2>&1
  cat $gfile | python3 -m json.tool > ${gfile}_
  mv ${gfile}_ ${gfile}

  $testerExe $tgt $gfile >> $LOGDIR/testrun.log 2>&1

  if [ $? -ne 0 ]; then
    echo "Failure for file: $gfile. Keeping generated file for inspection"
    mv $gfile $LOGDIR
    fail=$((fail + 1))
  else
    #echo "Success for file: $gfile. Deleting generated file"
    rm $gfile
  fi

  return $fail
}

function applyFileFormatTwoToMultiTU {
  fail=0
  tc=$1
  taFile=${tc}_a.cpp
  tbFile=${tc}_b.cpp

  # Result files
  ipcgTaFile="${taFile/cpp/ipcg}_${CI_CONCURRENT_ID}"
  ipcgTbFile="${tbFile/cpp/ipcg}_${CI_CONCURRENT_ID}"

  # Groundtruth files
  gtaFile="${taFile/cpp/gtmcg}"
  gtbFile="${tbFile/cpp/gtmcg}"
  gtCombFile="${tc}_combined.gtmcg"

  # Translation-unit-local
  # TODO: Ground truths currently only include numStatements metadata. Tests for old cgcollector also have fileProperties.
  #       What should be the general MD set tested here?
  #
  $cgcollectorExe --NumStatements --OverrideMD --whole-program ./input/multiTU/$taFile --cg-file=./input/multiTU/$ipcgTaFile -- >> $LOGDIR/testrun.log 2>&1
  $cgcollectorExe --NumStatements --OverrideMD --whole-program ./input/multiTU/$tbFile --cg-file=./input/multiTU/$ipcgTbFile -- >> $LOGDIR/testrun.log 2>&1

  cat ./input/multiTU/${ipcgTaFile} | python3 -m json.tool >./input/multiTU/${ipcgTaFile}_
  mv ./input/multiTU/${ipcgTaFile}_ ./input/multiTU/${ipcgTaFile}
  cat ./input/multiTU/${ipcgTbFile} | python3 -m json.tool >./input/multiTU/${ipcgTbFile}_
  mv ./input/multiTU/${ipcgTbFile}_ ./input/multiTU/${ipcgTbFile}

  $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile} >> $LOGDIR/testrun.log 2>&1
  aErr=$?
  $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile} >> $LOGDIR/testrun.log 2>&1
  bErr=$?

  combFile=${tc}_combined.ipcg_${CI_CONCURRENT_ID}
  echo "null" >./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >> $LOGDIR/testrun.log 2>&1
  mErr=$?

  cat ./input/multiTU/${combFile} | python3 -m json.tool >./input/multiTU/${combFile}_
  mv ./input/multiTU/${combFile}_ ./input/multiTU/${combFile}

  ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >> $LOGDIR/testrun.log 2>&1
  cErr=$?

  echo "$aErr or $bErr or $mErr or $cErr"

  if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then
    echo "Failure for file: $combFile. Keeping generated file for inspection"
    mv ./input/multiTU/$combFile $LOGDIR
    mv ./input/multiTU/${ipcgTaFile} $LOGDIR
    mv ./input/multiTU/${ipcgTbFile} $LOGDIR

    fail=$((fail + 1))
  else
    #echo "Success for file: $combFile. Deleting generated file"
    rm ./input/multiTU/$combFile ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile}
  fi
  return $fail
}

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


type -P $testerExe > /dev/null 2>&1
if [[ $? -eq 1 ]]; then
  echo "The CGDiff binary (cgdiff) could not be found in path, testing with relative path."
  stat ${PWD}/../../../${build_dir}/tools/cgdiff/cgdiff >> $LOGDIR/testrun.log 2>&1
  if [ $? -eq 1 ]; then
    echo "The file cgdiff seems also non-present in ../../../${build_dir}/tools/cgdiff/cgdiff. Aborting test. Failure! Please build the tester first."
    exit 1
  else
    testerExe=${PWD}/../../../${build_dir}/tools/cgdiff/cgdiff
  fi
fi


type -P $cgcollectorExe > /dev/null 2>&1
if [[ $? -eq 1 ]]; then
  echo "No cgcollector2 in PATH. Trying relative path ../../../${build_dir}/tools/cgcollector2/"

  stat ${PWD}/../../../${build_dir}/tools/cgcollector2/cgcollector2 >> $LOGDIR/testrun.log 2>&1
  if [ $? -eq 1 ]; then
    echo "The file seems also non-present in ../../../${build_dir}/tools/cgcollector2/ Aborting test. Failure! Please build the collector first."
    exit 1
  else
    cgcollectorExe=${PWD}/../../../${build_dir}/tools/cgcollector2/cgcollector2
  fi
fi

type -P $cgmergeExe > /dev/null 2>&1
if [[ $? -eq 1 ]]; then
  echo "No cgmerge2 in PATH. Trying relative path ../${build_dir}/test"
  stat ${PWD}/../../../${build_dir}/tools/cgmerge2/cgmerge2 >> $LOGDIR/testrun.log 2>&1
  if [ $? -eq 1 ]; then
    echo "The file seems also non-present in ../../../${build_dir}/tools/cgmerge2/. Aborting test. Failure! Please build the collector first."
    exit 1
  else
    cgmergeExe=${PWD}/../../../${build_dir}/tools/cgmerge2/cgmerge2
  fi
fi

sfails=0

# Multi-file tests
multiTests=(0042 0043 0044 0050 0053 0060)

echo " --- Running single file tests [file format version 2.0]---"
echo " --- Running basic tests ---"
testGlob="./input/singleTU/*.cpp"


for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTU ${tc} "--whole-program --NumStatements"
  fail=$?
  sfails=$((sfails + fail))
done
echo "Single file test failures: $sfails"

# Single File and full Ctor/Dtor coverage
echo -e "\n --- Running single file full ctor/dtor tests ---"
testGlob="./input/allCtorDtor/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  #we need to capture implicits here, as some calls are to implicit constructors/destructors
  applyFileFormatTwoToSingleTU ${tc} "--capture-ctors-dtors --capture-new-delete-calls --capture-implicits --infer-ctors-dtors --whole-program --prune --NumStatements"
  fail=$?
  sfails=$((sfails + fail))
done
echo "Single file test failures: $sfails"

# Single File and for CXXRecordCalls
echo -e "\n --- Running single file CXXRecord call tests ---"
testGlob="./input/cxxRecordCalls/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  #we need to capture implicits here, as some calls are to implicit constructors/destructors
  applyFileFormatTwoToSingleTU ${tc} "--capture-ctors-dtors --capture-new-delete-calls --capture-implicits --prune --infer-ctors-dtors"
  fail=$?
  sfails=$((sfails + fail))
done
echo "Single file test failures: $sfails"


# Single File and functionPointers
echo -e "\n --- Running single file functionPointers tests ---"
testGlob="./input/functionPointers/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTU ${tc} "--whole-program --NumStatements"
  fail=$?
  sfails=$((sfails + fail))
done
echo "Single file test failures: $sfails"



# Single File metaCollectors
echo -e "\n --- Running single file metaCollectors tests ---"
testGlob="./input/metaCollectors/numStatements/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTU ${tc}  "--whole-program --NumStatements"
  fail=$?
  sfails=$((sfails + fail))
done

echo "Single file test failures: $sfails"

# Single File virtualCalls
echo -e "\n --- Running single file virtualCalls tests ---"
testGlob="./input/virtualCalls/*.cpp"
for tc in ${testGlob}; do
  echo "Running test ${tc}"
  applyFileFormatTwoToSingleTU ${tc} "--whole-program --capture-ctors-dtors --NumStatements --OverrideMD"
  fail=$?
  sfails=$((sfails + fail))
done
echo "Single file test failures: $sfails"


# Multi File
mfails=0
echo -e "\n --- Running multi file tests ---"
for tc in "${multiTests[@]}"; do
  echo "Running test ${tc}"
  applyFileFormatTwoToMultiTU ${tc} ""
  fail=$?
  mfails=$((mfails + fail))
done
echo "Multi file test failures: $mfails"

tfails=$((sfails+mfails))
echo -e "$tfails failures occured when running tests"
exit $tfails
