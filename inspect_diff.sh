#!/bin/bash
while read -r filename; do
    [ -z "$filename" ] && continue
    paths=($(find Source/NeoEngine -type f -name "$filename"))
    if [ ${#paths[@]} -gt 1 ]; then
        if ! diff -q "${paths[0]}" "${paths[1]}" > /dev/null; then
            echo "=================================================="
            echo "FILE: $filename"
            echo "Path A: ${paths[0]}"
            echo "Path B: ${paths[1]}"
            echo "------------------- DIFF SUMMARY -------------------"
            diff -u "${paths[0]}" "${paths[1]}" | head -n 30
            echo ""
        fi
    fi
done < ./duplicate_files.txt
