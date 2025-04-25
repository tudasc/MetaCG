
cgcollectorExe=cgcollector
testerExe=cgsimpletester
cgmergeExe=cgmerge
build_dir=build # may be changed with opt 'b'

timeStamp=$(date +%s)
: ${CI_CONCURRENT_ID:=$timeStamp}

mkdir -p log

# Function to invoke the CGCollector to a target source code
# Param 1: The relative path name to the test case.
# Param 2: Parameter to steer certain features on / off
function applyFileFormatOneToSingleTU {
  testCaseFile=$1
  addFlags=$2
  gtvariant=$3
  fail=0

  local infix=""
  if [[ -n "$gtvariant" ]]; then
    infix="${gtvariant}."
  fi

  # Set up the different data files, we need:
  # - The test case
  # - Tehe groundtruth data for reconciling the CG constructed by MetaCG
  tfile=$testCaseFile
  gfile=${testCaseFile/cpp/${infix}ipcg}-${CI_CONCURRENT_ID}
  tgt=${testCaseFile/cpp/${infix}gtipcg}

  echo "Running ${testerExe} on ${tfile}"
  $cgcollectorExe --metacg-format-version=1 ${addFlags} --output ${gfile} $tfile -- >>log/testrun.log 2>&1
  cat $gfile | python3 -m json.tool > ${gfile}_
  mv ${gfile}_ ${gfile}
  $testerExe $tgt $gfile >>log/testrun.log 2>&1

  if [ $? -ne 0 ]; then
    echo "Failure for file: $gfile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
    rm $gfile
  fi

  return $fail
}

# Function to invoke the CGCollector with file format version 2 to a target source code
# Param 1: The relative path name to the test case.
# Param 2: Parameter to steer certain features on / off
function applyFileFormatTwoToSingleTU {
  testCaseFile=$1
  addFlags=$2
  gtvariant=$3
  fail=0

  local infix=""
  if [[ -n "$gtvariant" ]]; then
    infix="${gtvariant}."
  fi

  # Set up the different data files, we need:
  # - The test case
  # - Tehe groundtruth data for reconciling the CG constructed by MetaCG
  tfile=$testCaseFile
  gfile=${testCaseFile/cpp/${infix}ipcg}-${CI_CONCURRENT_ID}
  tgt=${testCaseFile/cpp/${infix}gtmcg}

 echo "Running tester on ${tfile}"
  $cgcollectorExe --metacg-format-version=2 ${addFlags} --output ${gfile} $tfile -- >>log/testrun.log 2>&1
  cat $gfile | python3 -m json.tool > ${gfile}_
  mv ${gfile}_ ${gfile}
  $testerExe $tgt $gfile >>log/testrun.log 2>&1

  if [ $? -ne 0 ]; then
    echo "Failure for file: $gfile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
    rm $gfile
  fi

  return $fail
}

function applyFileFormatTwoToSingleTUWithAA {
  testCaseFile=$1
  addFlags=$2
  fail=0

  # Set up the different data files, we need:
  # - The test case
  # - Tehe groundtruth data for reconciling the CG constructed by MetaCG
  tfile=$testCaseFile
  gfile=${testCaseFile/cpp/ipcg}-${CI_CONCURRENT_ID}
  tgt=${testCaseFile/cpp/gtaacg}

 echo "Running tester on ${tfile}"
  $cgcollectorExe --metacg-format-version=2 --capture-ctors-dtors --capture-stack-ctors-dtors --enable-AA ${addFlags} --output ${gfile} $tfile -- >>log/testrun.log 2>&1
  cat $gfile | python3 -m json.tool > ${gfile}_
  mv ${gfile}_ ${gfile}
  $testerExe $tgt $gfile >>log/testrun.log 2>&1

  if [ $? -ne 0 ]; then
    echo "Failure for file: $gfile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
    rm $gfile
  fi

  return $fail
}

function applyFileFormatOneToMultiTU {
  fail=0
  tc=$1
  taFile=${tc}_a.cpp
  tbFile=${tc}_b.cpp

  # Result files
  ipcgTaFile="${taFile/cpp/ipcg}-${CI_CONCURRENT_ID}"
  ipcgTbFile="${tbFile/cpp/ipcg}-${CI_CONCURRENT_ID}"

  # Groundtruth files
  gtaFile="${taFile/cpp/gtipcg}"
  gtbFile="${tbFile/cpp/gtipcg}"
  gtCombFile="${tc}_combined.gtipcg"

  # Translation-unit-local
  $cgcollectorExe --output ./input/multiTU/${ipcgTaFile} ./input/multiTU/$taFile -- >>log/testrun.log 2>&1
  $cgcollectorExe --output ./input/multiTU/${ipcgTbFile} ./input/multiTU/$tbFile -- >>log/testrun.log 2>&1

  cat ./input/multiTU/${ipcgTaFile} | python3 -m json.tool >./input/multiTU/${ipcgTaFile}_
  mv ./input/multiTU/${ipcgTaFile}_ ./input/multiTU/${ipcgTaFile}
  cat ./input/multiTU/${ipcgTbFile} | python3 -m json.tool >./input/multiTU/${ipcgTbFile}_
  mv ./input/multiTU/${ipcgTbFile}_ ./input/multiTU/${ipcgTbFile}

  $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile} >>log/testrun.log 2>&1
  aErr=$?
  $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile} >>log/testrun.log 2>&1
  bErr=$?

  combFile=${tc}_combined-${CI_CONCURRENT_ID}.ipcg
  echo "null" >./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1
  mErr=$?

  cat ./input/multiTU/${combFile} | python3 -m json.tool >./input/multiTU/${combFile}_
  mv ./input/multiTU/${combFile}_ ./input/multiTU/${combFile}

  ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >>log/testrun.log 2>&1
  cErr=$?

  #echo "$aErr or $bErr or $mErr or $cErr"

  if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then
    echo "Failure for file: $combFile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
    rm ./input/multiTU/$combFile ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile}
  fi
  return $fail
}

function applyFileFormatTwoToMultiTU {
  fail=0
  tc=$1
  taFile=${tc}_a.cpp
  tbFile=${tc}_b.cpp

  # Result files
  ipcgTaFile="${taFile/cpp/ipcg}-${CI_CONCURRENT_ID}"
  ipcgTbFile="${tbFile/cpp/ipcg}-${CI_CONCURRENT_ID}"

  # Groundtruth files
  gtaFile="${taFile/cpp/gtmcg}"
  gtbFile="${tbFile/cpp/gtmcg}"
  gtCombFile="${tc}_combined.gtmcg"

  # Translation-unit-local
  $cgcollectorExe --metacg-format-version=2 --output ./input/multiTU/${ipcgTaFile} ./input/multiTU/$taFile -- >>log/testrun.log 2>&1
  $cgcollectorExe --metacg-format-version=2 --output ./input/multiTU/${ipcgTbFile} ./input/multiTU/$tbFile -- >>log/testrun.log 2>&1

  cat ./input/multiTU/${ipcgTaFile} | python3 -m json.tool >./input/multiTU/${ipcgTaFile}_
  mv ./input/multiTU/${ipcgTaFile}_ ./input/multiTU/${ipcgTaFile}
  cat ./input/multiTU/${ipcgTbFile} | python3 -m json.tool >./input/multiTU/${ipcgTbFile}_
  mv ./input/multiTU/${ipcgTbFile}_ ./input/multiTU/${ipcgTbFile}

  $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile} >>log/testrun.log 2>&1
  aErr=$?
  $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile} >>log/testrun.log 2>&1
  bErr=$?

  combFile=${tc}_combined-${CI_CONCURRENT_ID}.ipcg
  echo "null" >./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1
  mErr=$?

  cat ./input/multiTU/${combFile} | python3 -m json.tool >./input/multiTU/${combFile}_
  mv ./input/multiTU/${combFile}_ ./input/multiTU/${combFile}

  ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >>log/testrun.log 2>&1
  cErr=$?

  echo "$aErr or $bErr or $mErr or $cErr"

  if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then
    echo "Failure for file: $combFile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
    rm ./input/multiTU/$combFile ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile}
  fi
  return $fail
}

function applyFileFormatTwoToMultiTUWithAA {
  fail=0
  tc=$1
  taFile=${tc}_a.cpp
  tbFile=${tc}_b.cpp

  # Result files
  ipcgTaFile="${taFile/cpp/ipcg}-${CI_CONCURRENT_ID}"
  ipcgTbFile="${tbFile/cpp/ipcg}-${CI_CONCURRENT_ID}"

  # Groundtruth files
  gtaFile="${taFile/cpp/gtaacg}"
  gtbFile="${tbFile/cpp/gtaacg}"
  gtCombFile="${tc}_combined.gtaacg"

  # Translation-unit-local
  $cgcollectorExe --metacg-format-version=2 --capture-ctors-dtors --capture-stack-ctors-dtors --enable-AA --output ./input/multiTU/${ipcgTaFile} ./input/multiTU/$taFile -- >>log/testrun.log 2>&1
  $cgcollectorExe --metacg-format-version=2 --capture-ctors-dtors --capture-stack-ctors-dtors --enable-AA --output ./input/multiTU/${ipcgTbFile}  ./input/multiTU/$tbFile -- >>log/testrun.log 2>&1

  cat ./input/multiTU/${ipcgTaFile} | python3 -m json.tool >./input/multiTU/${ipcgTaFile}_
  mv ./input/multiTU/${ipcgTaFile}_ ./input/multiTU/${ipcgTaFile}
  cat ./input/multiTU/${ipcgTbFile} | python3 -m json.tool >./input/multiTU/${ipcgTbFile}_
  mv ./input/multiTU/${ipcgTbFile}_ ./input/multiTU/${ipcgTbFile}

  $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile} >>log/testrun.log 2>&1
  aErr=$?
  $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile} >>log/testrun.log 2>&1
  bErr=$?

  combFile=${tc}_combined-${CI_CONCURRENT_ID}.ipcg
  echo "null" >./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1
  mErr=$?

  cat ./input/multiTU/${combFile} | python3 -m json.tool >./input/multiTU/${combFile}_
  mv ./input/multiTU/${combFile}_ ./input/multiTU/${combFile}

  ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >>log/testrun.log 2>&1
  cErr=$?

  echo "$aErr or $bErr or $mErr or $cErr"

  if [[ ${aErr} -ne 0 || ${bErr} -ne 0 || ${mErr} -ne 0 || ${cErr} -ne 0 ]]; then
    echo "Failure for file: $combFile. Keeping generated file for inspection"
    fail=$((fail + 1))
  else
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
