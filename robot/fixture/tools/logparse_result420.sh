#!/bin/bash
#cd "$(dirname ${BASH_SOURCE[0]})"

#clean output directory
rm -rf ./result420
mkdir -p result420
rm -rf ./result000
mkdir -p result000


ntotal=0; n420=0; n000=0;
function parse_file()
{
  infile=$1;
  #echo processing "$infile"
  #dos2unix "$infile"
  ntotal=$((ntotal+1))
  
  if cat "$infile" | grep -q "RESULT:420"; then cp "$infile" ./result420/; n420=$((n420+1)); fi
  if cat "$infile" | grep -q "RESULT:000"; then cp "$infile" ./result000/; n000=$((n000+1)); fi
}

#parse logfiles (*.log or *.txt formats)
for infile in ./*.log; do if [ "$infile" != "./*.log" ]; then parse_file "$infile"; fi done
for infile in ./*.txt; do if [ "$infile" != "./*.txt" ]; then parse_file "$infile"; fi done

echo $ntotal files, E420=$n420, E000=$n000

exit 0

