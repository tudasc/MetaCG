#!/bin/bash

# This script goes through all test call graphs and performs basic checks using cgformat.

cgformat="${1:-cgformat}"

if [[ ! -x "$(which $cgformat)" ]]; then
  echo "Need to pass 'cgformat' executable as argument - exiting."
  exit 1
fi

num_failures=0

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

target_folders=("graph/test/integration" "pgis/test/integration" "pymetacg/tests/resources" "tools/cgpatch/test/integration/input" "cgcollector/test/input")

for dir in "${target_folders[@]}"; do
  full_dir="$script_dir/$dir"
  graph_tests=$(find "$full_dir" -regextype posix-extended -type f -regex '.*\.(gt)?(ipcg|mcg)$')

  echo "Checking ${#graph_tests[@]} MetaCG files in $dir..."

  for t in $graph_tests; do
    echo "Checking file: $t"

    # Skipping v1 files
    grep -q '"_MetaCG"' $t
    if [[ $? -ne 0 ]]; then
      echo "File $t has no _MetaCG field - skipping..."
      continue
    fi

    # Skipping null graphs (because otherwise the reader in cgformat fails).
    grep -q '"_CG": null' $t
    if [[ $? -eq 0 ]]; then
      echo "File $t contains 'null' call graph - skipping..."
      continue
    fi

    # Invoke cgformat to check for canonical origin prefix
    "$cgformat" --origin_prefix "/opt/metacg" "$t" > /dev/null

    if [[ $? -ne 0 ]]; then
      echo "Formatting check failed"
      ((num_failures++))
    fi
  done
done

echo "Failures: $num_failures"

if ((num_failurs)); then
  exit 1
fi
