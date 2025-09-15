#!/bin/bash


build_dir="build"
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
            echo "use -h to print this help"
            exit 0;
            ;;
        \?)
            echo "Invalid option -$OPTARG"
            exit 1;
            ;;
    esac
done


build="../../../../${build_dir}"
cgdiff_exe=${build}/tools/cgdiff/cgdiff


run_cgdiff_test() {
  local fileA="$1"
  local fileB="$2"
  local expected_exit_code="$3"
  shift 3
  local options=("$@")

  # echo "Running cgdiff on '$fileA' and '$fileB' with options: $options"
  
  $cgdiff_exe $options "$fileA" "$fileB" &> /dev/null
  local actual_exit_code=$?

  if [ "$actual_exit_code" -eq "$expected_exit_code" ]; then
    echo "Test passed for '$fileA' vs '$fileB'."
  else
    echo "Test FAILED for '$fileA' vs '$fileB': expected exit code $expected_exit_code but got $actual_exit_code."
  fi
}

# full comparison
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1

# ignoring "hasBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 0 "--ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignore-body"

# ignoring "callees"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignore-edges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignore-edges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 0 "--ignore-edges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignore-edges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignore-edges"

# ignoring "metadata"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignore-md"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignore-md"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignore-md"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 0 "--ignore-md"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignore-md"

# ignoring multiple

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 0 "--ignore-md --ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 0 "--ignore-edges --ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 0 "--ignore-body --ignore-edges"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignore-edges --ignore-body"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignore-body --ignore-md"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignore-md --ignore-edges"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignore-body --ignore-edges --ignore-md"


# emit json
#mkdir testOutput
#run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--emit-diff-as-json -o testOutput/test1.json"
#diff test1.json test2.json
