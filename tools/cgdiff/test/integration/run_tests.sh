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
cgdiff_exe="${build}/tools/cgdiff/cgdiff"
output_dir="${build}/tools/cgdiff/output"
mkdir -p "$output_dir"

passed=0
failed=0

# Helper function for JSON diff comparison
compare_diff_output() {
  local output_file="$1"
  local groundtruth_file="$2"

  if diff -u <(python3 -m json.tool "$output_file" | jq -S . 2>/dev/null || python3 -c 'import json,sys; print(json.dumps(json.load(open(sys.argv[1])), sort_keys=True, indent=2))' "$output_file") \
             <(python3 -m json.tool "$groundtruth_file" | jq -S . 2>/dev/null || python3 -c 'import json,sys; print(json.dumps(json.load(open(sys.argv[1])), sort_keys=True, indent=2))' "$groundtruth_file") \
             >/dev/null; then
      return 0
  else
      return 1
  fi
}

run_cgdiff_test() {
  local fileA="$1"
  local fileB="$2"
  local expected_exit_code="$3"
  local flags="$4"
  local groundtruth="$5"

  local testname="$(basename "$fileA")_vs_$(basename "$fileB")"
  local output_file="${output_dir}/${testname}.json"

  echo -n "Running $testname $flags ... "

  # Run cgdiff and capture diff output
  if $cgdiff_exe $flags "$fileA" "$fileB" --emit-diff-on-success -o "$output_file" &>/dev/null; then
      actual_exit_code=0
  else
      actual_exit_code=$?
  fi

  # Check exit code
  if [ "$actual_exit_code" -ne "$expected_exit_code" ]; then
      echo "(wrong exit code: got $actual_exit_code, expected $expected_exit_code)"
      ((failed++))
      return
  fi

  # If diff is expected, compare JSON output
  if [ -n "$groundtruth" ] && [ -f "$groundtruth" ]; then
      if compare_diff_output "$output_file" "$groundtruth"; then
          echo "PASSED"
          ((passed++))
      else
          echo "FAILED (diff mismatch)"
          ((failed++))
      fi
  else
      echo "PASSED"
      ((passed++))
  fi
}

# ---------------------------
# Test cases
# ---------------------------

run_cgdiff_test ./input/cgA_basic.mcg ./input/cgB_basic.mcg 1 " " "./groundtruth/node_missing.md"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgC_basic.mcg 1 " " "./groundtruth/edge_missing_flag"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgC_basic.mcg 0 "--ignore-edges" "./groundtruth/edge_missing_flag"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgD_basic.mcg 1 " " "./groundtruth/body_different"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgD_basic.mcg 0 "--ignore-hasBody" "./groundtruth/body_different_flag"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgE_basic.mcg 1 " " "./groundtruth/edge_different"
run_cgdiff_test ./input/cgA_basic.mcg ./input/cgE_basic.mcg 0 "--ignore-edges" "./groundtruth/edge_different_flag"

run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgA_basic.mcg 1 " " "./groundtruth/metadata_missing"
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgA_basic.mcg 0 "--ignore-md" "./groundtruth/metadata_missing_flag"
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgB_metadata.mcg 1 " " "./groundtruth/metadata_different"
run_cgdiff_test ./input/cgA_metadata.mcg ./input/cgB_metadata.mcg 0 "--ignore-md" "./groundtruth/metadata_different_flag"

run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgA_basic.mcg 1 " " "./groundtruth/global_md_missing"
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgA_basic.mcg 1 "--ignore-global-md" "./groundtruth/global_md_missing_flag"
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgB_global_md.mcg 1 " " "./groundtruth/global_md_different"
run_cgdiff_test ./input/cgA_global_md.mcg ./input/cgB_global_md.mcg 0 "--ignore-global-md" "./groundtruth/global_md_different_flag"

# ---------------------------
# Summary
# ---------------------------

echo
echo "PASSED: $passed"
echo "FAILED: $failed"

if (( failed > 0 )); then
    exit 1
else
    exit 0
fi
