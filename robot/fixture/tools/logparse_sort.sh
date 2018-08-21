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

fileappend=0; started=0; result=-1; outfile=""; debug=0;

function do_start() {
  lineCnt=$1; line=$2; infile=$3;
  if [ ! $started -gt 0 ]; then
    started=1; fileappend=$(($fileappend+1))
    
    #infile=$(basename -- "$infile") #remove path
    fext="${infile##*.}"
    fname="${infile%.*}"
    outfile=$(printf "%s-%04i.%s" "$fname" $fileappend "$fext")
    
    if [ $debug -gt 0 ]; then echo ".do_start: \"$outfile\" line $lineCnt" ; fi
    echo $(printf "origin %s line %i\n" $infile $lineCnt) >> "$outfile"
    echo "" >> "$outfile"
    echo "$line" >> "$outfile"
  fi
}

function do_stop() {
  if [ $started -gt 0 ]; then
    if [ $debug -gt 0 ]; then echo ".do_stop" ; fi
    #echo "" >> "$outfile"
    folder=$( printf result%03i $((result)) )
    if [ ! -d "$folder" ]; then mkdir $folder; fi
    mv "$outfile" ./$folder
    started=0; result=-1; outfile="";
  fi
}

gFileCnt=0; gLineCnt=0
function parse_file()
{
  infile=$1;
  Tfilestart=$(($(date +%s%N)/1000000))
  echo processing "$infile"
  #dos2unix "$infile"
  
  #parse file
  fileappend=0; lineCnt=0
  while IFS='' read -r line || [[ -n "$line" ]]; do #https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable
    lineCnt=$(($lineCnt+1));
    if [ $started -gt 0 ]; then echo "$line" >> "$outfile" ; fi
    #if [ $debug -gt 0 ]; then echo ".$lineCnt $line" ; fi #DEBUG PRINT ALL LINES
    
    #scan for log markers [TEST:START], [RESULT:###], [TEST:END]
    if [[ "$line" == *"[TEST:START]"* ]]; then #echo "$line" | grep -q "[TEST:START]"
      if [ $debug -gt 0 ]; then echo ".$lineCnt $line" ; fi
      if [ $started -gt 0 ]; then do_stop ; fi #tag sequence error. truncate current test
      do_start $lineCnt "$line" "$infile"
    elif [[ "$line" == *"[TEST:END]"* ]]; then #echo "$line" | grep -q "[TEST:END]"
      if [ $debug -gt 0 ]; then echo ".$lineCnt $line" ; fi
      if [ ! $started -gt 0 ]; then do_start $lineCnt "$line" "$infile" ; fi #tag sequence error. quick-start to log this tag
      do_stop
    elif [[ "$line" == *"[RESULT:"* ]]; then #echo "$line" | grep -q "RESULT:"; then
      if [ $debug -gt 0 ]; then echo ".$lineCnt $line" ; fi
      if [ "$result" -ge 0 ]; then do_stop ; fi #tag sequence error, multiple result tags
      if [ ! $started -gt 0 ]; then do_start $lineCnt "$line" "$infile" ; fi #tag sequence error
      result=$(echo "$line" | grep -oP 'RESULT:\K[0-9]+');
      tally "result" "1";
    fi
    
  done < "$infile"
  if [ $started -gt 0 ]; then echo "----ERROR missing end tag" ; do_stop ; fi
  
  gFileCnt=$(($gFileCnt+1))
  gLineCnt=$(($gLineCnt+$lineCnt))
  
  Tfileend=$(($(date +%s%N)/1000000))
  if [ $debug -gt 0 ]; then echo ".$lineCnt lines in $(($Tfileend-$Tfilestart))ms, $((($Tfileend-$Tfilestart)/$lineCnt))ms per line" ; fi
}

#parse logfiles (*.log or *.txt formats)
Tstart=$(($(date +%s%N)/1000))
for infile in ./*.log; do if [ "$infile" != "./*.log" ]; then parse_file "$infile"; fi done
for infile in ./*.txt; do if [ "$infile" != "./*.txt" ]; then parse_file "$infile"; fi done
Tend=$(($(date +%s%N)/1000))
Tproc=$(($Tend-$Tstart))

#print results
show_tally
echo processed $gFileCnt files $gLineCnt lines in $(($Tproc/1000))ms. avg $(($Tproc/$gLineCnt))us per line

exit 0

