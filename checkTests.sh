#!/bin/bash

cgformat="${1:-cgformat}"

num_failures=0

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

target_folders=("graph/test/integration" "pgis/test/integration" "pymetacg/tests/resources" "tools/cgpatch/test/integration/input" "cgcollector/test/input")

for dir in "${target_folders[@]}"; do
  full_dir="$script_dir/$dir"
  graph_tests=$(find "$full_dir" -regextype posix-extended -type f -regex '.*\.(gt)?(ipcg|mcg)$')

  echo "$graph_tests"
  echo "Checking ${#graph_tests[@]} MetaCG files in $dir..."

  for t in $graph_tests; do
    echo "Checking file: $t"
    grep -q '"_MetaCG"' $t
    if [[ $? -ne 0 ]]; then
      echo "File $t has no _MetaCG field - skipping..."
      continue
    fi
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
