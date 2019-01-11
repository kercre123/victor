#!/bin/bash
#cd "$(dirname ${BASH_SOURCE[0]})"

#use 2 arrays to make a crappy hash table
declare -a acode
declare -a acount
acode+=("000"); #acode+=("123");
for val in "${acode[@]}"; do acount+=("0"); done

function tally()
{
  arg1=$1; increment=$2;
  result=$( printf %03i $((arg1)) )
  
  addEntry=1
  for i in "${!acode[@]}"; do
    if [ "${acode[$i]}" == "$result" ]; then
      acount[$i]=$((${acount[$i]}+$increment))
      addEntry=0
      break
    fi
  done
  
  #new table entry when unique result codes are found
  if [ $addEntry -gt 0 ]; then acode+=("$result"); acount+=("1"); fi
}

function show_tally()
{
  for i in "${!acode[@]}"; do 
    echo "result${acode[$i]}: ${acount[$i]}"
  done
}

gFileCnt=0; gLineCnt=0
function parse_file()
{
  infile=$1;
  Tfilestart=$(($(date +%s%N)/1000000))
  echo processing "$infile"
  #dos2unix "$infile"
  
  #parse file to find test result
  result=0; resultCnt=0; lineCnt=0;
  while IFS='' read -r line || [[ -n "$line" ]]; do #https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable
    lineCnt=$(($lineCnt+1));
    #if echo "$line" | grep -q "RESULT:"; then
      result=$(echo "$line" | grep -oP 'RESULT:\K[0-9]+');
      resultCnt=$((resultCnt+1));
    #fi
  done < <(cat "$infile" | grep "RESULT:") # < "$infile"
  
  gFileCnt=$(($gFileCnt+1))
  gLineCnt=$(($gLineCnt+$lineCnt))
  
  if [ "$resultCnt" -lt 1 ]; then
    tally "-1" "1"
    echo ---- no test results detected
  else #one or more results found
    tally "result" "1";
    if [ "$resultCnt" -gt 1 ]; then echo echo ---- multiple test results detected: $resultCnt; fi #XXX: handle splitting logfiles??
    
    #copy logfile to proper results folder
    folder=$( printf result%03i $((result)) )
    if [ ! -d "$folder" ]; then mkdir $folder; fi
    cp "$infile" ./$folder
  fi
  
  Tfileend=$(($(date +%s%N)/1000000))
  #echo ".$(($Tfileend-$Tfilestart))ms" #, $((($Tfileend-$Tfilestart)/$lineCnt))ms per line"
}

#parse logfiles (*.log or *.txt formats)
Tstart=$(($(date +%s%N)/1000))
for infile in ./*.log; do if [ "$infile" != "./*.log" ]; then parse_file "$infile"; fi done
for infile in ./*.txt; do if [ "$infile" != "./*.txt" ]; then parse_file "$infile"; fi done
Tend=$(($(date +%s%N)/1000))
Tproc=$(($Tend-$Tstart))

#print results
show_tally
echo processed $gFileCnt files in $(($Tproc/1000))ms #$gLineCnt lines in $(($Tproc/1000))ms. avg $(($Tproc/$gLineCnt))us per line

exit 0

