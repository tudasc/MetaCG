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
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 0 "--ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignoreBody"

# ignoring "callees"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignoreEdges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignoreEdges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 0 "--ignoreEdges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignoreEdges"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignoreEdges"

# ignoring "metadata"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG.json 0 "--ignoreMD"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignoreMD"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignoreMD"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 0 "--ignoreMD"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignoreMD"

# ignoring multiple

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 0 "--ignoreMD --ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 0 "--ignoreEdges --ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 0 "--ignoreBody --ignoreEdges"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_metadata.json 1 "--ignoreEdges --ignoreBody"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_edge.json 1 "--ignoreBody --ignoreMD"
run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_body.json 1 "--ignoreMD --ignoreEdges"

run_cgdiff_test ./input/simpleCG.json ./input/simpleCG_node_missing.json 1 "--ignoreBody --ignoreEdges --ignoreMD"
