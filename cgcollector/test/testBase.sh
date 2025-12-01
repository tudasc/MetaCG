
cgcollectorExe=cgcollector
cgmergeExe=cgmerge
build_dir=build # may be changed with opt 'b'
diffFile=$(mktemp temp.json.XXX)
export diffFile

timeStamp=$(date +%s)
: ${CI_CONCURRENT_ID:=$timeStamp}

mkdir -p log

only_metadata_diff() {
    local diffFile="$1"

    if [ ! -f "$diffFile" ]; then
        echo "Diff file not found: $diffFile"
        return 2
    fi

    # Iterate over all nodes
    local nodes
    nodes=$(jq -r '.diff.nodeDiffs | keys[]' "$diffFile") || return 2

    for node in $nodes; do
        # Extract diffType array
        local diffTypes
        diffTypes=$(jq -r ".diff.nodeDiffs[\"$node\"].diffType[]" "$diffFile")

        # Skip nodes with other types of differences
        for dt in $diffTypes; do
            if [ "$dt" != "differentMetadata" ]; then
                return 1
            fi
        done

        # Extract metadataOnlyInA and metadataOnlyInB arrays
        local aJson bJson
        aJson=$(jq ".diff.nodeDiffs[\"$node\"].metadataOnlyInA" "$diffFile")
        bJson=$(jq ".diff.nodeDiffs[\"$node\"].metadataOnlyInB" "$diffFile")

        # Filter out entries that contain numberOfControlFlowOps
        local aFiltered bFiltered
        aFiltered=$(echo "$aJson" | jq '[.[] | select(test("numberOfControlFlowOps") | not)]')
        bFiltered=$(echo "$bJson" | jq '[.[] | select(test("numberOfControlFlowOps") | not)]')

        # If A is empty after filtering → continue
        local aLen
        aLen=$(echo "$aFiltered" | jq 'length')
        if [ "$aLen" -eq 0 ]; then
            continue
        fi

        # Extract keys (everything before colon)
        local aKeys bKeys
        aKeys=$(echo "$aFiltered" | jq -r '.[] | split(":")[0]' | sort)
        bKeys=$(echo "$bFiltered" | jq -r '.[] | split(":")[0]' | sort)

        # Check if all keys in A exist in B
        local missing
        missing=$(comm -23 <(echo "$aKeys") <(echo "$bKeys"))
        if [ -n "$missing" ]; then
            return 1
        fi
    done

    return 0
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

  $cgcollectorExe --metacg-format-version=2 ${addFlags} --output ${gfile} $tfile -- >>log/testrun.log 2>&1
  cat $gfile | python3 -m json.tool > ${gfile}_
  mv ${gfile}_ ${gfile}
  $testerExe $tgt $gfile >>log/testrun.log 2>&1

  if [ $? -ne 0 ]; then
    if only_metadata_diff $diffFile; then
        rm $gfile
    else 
        echo "Failure for file: $gfile. Keeping generated file for inspection"
        fail=$((fail + 1))
    fi
  else
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

  $testerExe  ./input/multiTU/${gtaFile} ./input/multiTU/${ipcgTaFile} >>log/testrun.log 2>&1
  aErr=$?

  if only_metadata_diff "$diffFile"; then
    aErr=0
  else
    echo "[Info] aErr metadata diff"
    echo "Running $testerExe ./input/multiTU/${ipcgTaFile} ./input/multiTU/${gtaFile}"
  fi

  $testerExe  ./input/multiTU/${gtbFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1
  bErr=$?

  if only_metadata_diff "$diffFile"; then
    bErr=0
  else
    echo "[Info] bErr metadata diff"
    echo "Running $testerExe ./input/multiTU/${ipcgTbFile} ./input/multiTU/${gtbFile}"
  fi

  combFile=${tc}_combined-${CI_CONCURRENT_ID}.ipcg
  echo "null" >./input/multiTU/${combFile}

  ${cgmergeExe} ./input/multiTU/${combFile} ./input/multiTU/${ipcgTaFile} ./input/multiTU/${ipcgTbFile} >>log/testrun.log 2>&1
  mErr=$?

  cat ./input/multiTU/${combFile} | python3 -m json.tool >./input/multiTU/${combFile}_
  mv ./input/multiTU/${combFile}_ ./input/multiTU/${combFile}

  ${testerExe} ./input/multiTU/${gtCombFile} ./input/multiTU/${combFile} >>log/testrun.log 2>&1
  cErr=$?

  if only_metadata_diff "$diffFile"; then
    cErr=0
  else
    echo "[Info] cErr metadata diff"
    echo "Was running: ${testerExe} ./input/multiTU/${combFile} ./input/multiTU/${gtCombFile} >>log/testrun.log 2>&1"
  fi

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
