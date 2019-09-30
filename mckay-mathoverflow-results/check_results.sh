#!/bin/bash

cat $1 | while read a b c d; do
  diff <(echo $a $b $c $d) <(cat table_reformatted.txt | grep '^'$a' '$b' ')
done
