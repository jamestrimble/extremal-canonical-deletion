#!/bin/bash

for min_deg in $(seq 2 7); do
  b=$((min_deg - 1))
  for max_deg in $(seq $min_deg 10); do
    a=$max_deg
    for n_targets in $(seq $((b * b)) $((b * b + 3))); do
      (cat <<EOF
a = $a;
b = $b;
n_targets = $n_targets;
EOF
) > girth6.dzn
      echo $min_deg $max_deg $n_targets $(timeout 1200 mzn-gecode girth6.mzn girth6.dzn)
    done
  done
done | grep 'UNSAT'
