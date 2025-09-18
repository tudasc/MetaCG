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

run_cgdiff_test ./input/cgA_basic.mcg ./input/cgA_basic.mcg 0 # no diff
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgB_basic.mcg 1 # node missing
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgC_basic.mcg 1 # edge missing
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgC_basic.mcg 0 "--ignore-edges" # edge missing
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgD_basic.mcg 1 # body missing
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgD_basic.mcg 0 "--ignore-body" # body missing
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgE_basic.mcg 1 # edge different
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgE_basic.mcg 0 "--ignore-edges" # edge different

run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgA_basic.mcg 1    # md missing
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgA_basic.mcg 0 "--ignore-md"    # md missing
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgB_metadata.mcg 1 # md missing
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgB_metadata.mcg 0 "--ignore-md" # md missing

run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgC_no_global_md.mcg 1 # md missing
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgC_no_global_md.mcg 0 "--ignore-global-md" # md missing
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgB_global_md.mcg 1  # md different
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgB_global_md.mcg 0 "--ignore-global-md" # md different
