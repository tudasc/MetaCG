#!/usr/bin/env bash

#
# Multi-Pattern Search and Replace Script
# 
# This script reads input from stdin and performs multiple search-and-replace operations
# on the content using sed. It accepts search/replace patterns as command-line argument
# pairs and processes them sequentially.
#
# Usage: 
#   cat file.txt | $0 <search1> <replace1> [search2] [replace2] ...
#   echo "text" | $0 'pattern1' 'replacement1' 'pattern2' 'replacement2'
#
# Arguments:
#   - Must be provided in pairs (search pattern followed by replacement string)
#   - Each search pattern is treated as a sed regular expression
#   - Replacements are applied globally (all occurrences per line)
#   - Patterns are processed in the order they are provided
#
# Example:
#   cat file.txt | $0 'old' 'new' 'foo' 'bar'
#   # This replaces all 'old' with 'new', then all 'foo' with 'bar'
#
# Note: Special regex characters in search patterns should be escaped appropriately
#

# Check if arguments are provided in pairs
if [ $# -eq 0 ] || [ $(($# % 2)) -ne 0 ]; then
    echo "Usage: $0 <search1> <replace1> [search2] [replace2] ..." >&2
    echo "Example: cat file.txt | $0 'old' 'new' 'foo' 'bar'" >&2
    exit 1
fi

# Read stdin into a variable
content=$(cat)

# Process each search/replace pair
while [ $# -gt 0 ]; do
    search="$1"
    replace="$2"
    
    # Perform the replacement using sed
    content=$(echo "$content" | sed "s/$search/$replace/g")
    
    shift 2
done

# Output the result
echo "$content"
