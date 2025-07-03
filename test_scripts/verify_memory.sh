#!/bin/bash
expected_column="$1"

WD="/Users/u7826985/Projects/gitworkflow/iqtree3/test_scripts/test_data"
input_file="${WD}/expected_memory.tsv"
reported_file="/Users/u7826985/Projects/gitworkflow/iqtree3/test_scripts/timelogs/time_log.tsv"
selected_columns_file="${WD}/selected_columns.tsv"
cut -f1,2,"$expected_column" "$input_file" > "${selected_columns_file}"
final_file="${WD}/combined_with_reported.tsv"

# assuming the reported file and expected file have the same order of commands
cut -f3 "$reported_file" > /tmp/reported_column.tsv
paste "$selected_columns_file" /tmp/reported_column.tsv > "$final_file"


fail_count=0

echo -e "Command\tExpected\tThreshold\tActual\tExceededBy"

# Skip header
tail -n +2 "$final_file" | while IFS=$'\t' read -r command threshold expected reported; do
    # Compute allowed = expected + threshold
    allowed=$(echo "$expected + $threshold" | bc -l)
    # Check if reported > allowed
    is_exceed=$(echo "$reported > $allowed" | bc -l)
    if [ "$is_exceed" = "1" ]; then
        diff=$(echo "$reported - $expected" | bc -l)
        echo "❌ $command exceeded the allowed memory usage."
        echo "Expected: $expected MB, Threshold: $threshold MB, Reported: $reported MB"
        ((fail_count++))
    else
        echo "✅ $command passed the memory check."
        echo -e "$command\t$expected\t$threshold\t$reported\t$diff"
    fi
done

echo
if [ "$fail_count" -eq 0 ]; then
    echo "✅ All runtime checks passed."
else
    echo "❌ $fail_count checks failed."
    exit 1
fi
