#!/bin/bash
#cd "$(dirname ${BASH_SOURCE[0]})"

#BREIF: parse ROBOT1,3 fixture logfiles to collect motor range test data

#sample log section of interest:
#[009.390339] LIFT range test. power 75
#...
#[010.251291] HEAD range test. power 100
#...
#[011.267224] LIFT POS start:1 passive:1 delta:0
#[011.267474] LIFT UP  speed:+2397 avg:+2233 travel:+598
#[011.267586] LIFT DN  speed:-2571 avg:-2289 travel:-562
#[011.267693] HEAD POS start:0 passive:0 delta:0
#[011.267800] HEAD UP  speed:+2884 avg:+2675 travel:+920
#[011.267906] HEAD DN  speed:-2966 avg:-2761 travel:-890
#[011.268010] LIFT range test. power 50
#...
#[012.430094] HEAD range test. power 55
#...
#[014.003483] LIFT POS start:-128 passive:-128 delta:0
#[014.003741] LIFT UP  speed:+1540 avg:+1455 travel:+612
#[014.003858] LIFT DN  speed:-1548 avg:-1473 travel:-590
#[014.003966] HEAD POS start:-130 passive:-130 delta:0
#[014.004072] HEAD UP  speed:+1343 avg:+1295 travel:+804
#[014.004177] HEAD DN  speed:-1315 avg:-1293 travel:-780
#...

directory="$(dirname ${BASH_SOURCE[0]})"
rm -f $directory/lift*.csv $directory/head*.csv

columnNames=("pwr" "POS-start" "POS-passive" "POS-delta" "UP-speed" "DN-speed" "UP-avg" "DN-avg" "UP-travel" "DN-travel")
declare -a liftdat
declare -a headdat
for val in "${columnNames[@]}"; do liftdat+=(""); headdat+=(""); done

function write_liftdat()
{
  result=$1; row=""; blanks=0;
  for val in "${liftdat[@]}"; do
    row="$row,$val";
    if [ "$val" == "" ]; then blanks=$((blanks+1)); fi
  done
  
  if [ "${liftdat[0]}" == "" ]; then liftdat[0]="-1"; fi
  outfile=$(printf "lift_%03i.csv" "${liftdat[0]}")
  if [ ! -e "$outfile" ]; then
    row1=""; #first row is column labels
    for val in "${columnNames[@]}"; do row1="$row1,$val"; done
    echo "$row1,result,blanks" >> $outfile
  fi
  
  echo "$row,$result,$blanks" >> $outfile
  for i in "${!liftdat[@]}"; do liftdat[$i]=""; done #clear
}

function write_headdat()
{
  result=$1; row=""; blanks=0;
  for val in "${headdat[@]}"; do
    row="$row,$val";
    if [ "$val" == "" ]; then blanks=$((blanks+1)); fi
  done
  
  if [ "${headdat[0]}" == "" ]; then headdat[0]="-1"; fi
  outfile=$(printf "head_%03i.csv" "${headdat[0]}")
  if [ ! -e "$outfile" ]; then
    row1=""; #first row is column labels
    for val in "${columnNames[@]}"; do row1="$row1,$val"; done
    echo "$row1,result,blanks" >> $outfile
  fi
  
  echo "$row,$result,$blanks" >> $outfile
  for i in "${!headdat[@]}"; do headdat[$i]=""; done #clear
}

function parse_line()
{
  line=$1;
  
  #strip timestamp prefix '[###.######] '
  #if [[ "$line" == *"LIFT"* ]] || [[ "$line" == *"HEAD"* ]]; then
  #  if [ "${line:0:1}" == "[" ]; then line=$(echo $line | grep -oP '\[[0-9]+\.[0-9]+\]\s\K(.)*'); fi
  #fi
  
  if [[ "$line" == *"LIFT"* ]]; then
    if [[ "$line" == *"LIFT range test"* ]]; then
      liftdat[0]=$(echo $line | grep -oP 'power \K[+-]*([0-9]+)');
    elif [[ "$line" == *"LIFT POS"* ]]; then
      liftdat[1]=$(echo $line | grep -oP 'start:\K[+-]*([0-9]+)');
      liftdat[2]=$(echo $line | grep -oP 'passive:\K[+-]*([0-9]+)');
      liftdat[3]=$(echo $line | grep -oP 'delta:\K[+-]*([0-9]+)');
    elif [[ "$line" == *"LIFT UP"* ]]; then
      liftdat[4]=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      liftdat[6]=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      liftdat[8]=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
    elif [[ "$line" == *"LIFT DN"* ]]; then
      liftdat[5]=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      liftdat[7]=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      liftdat[9]=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      #write_liftdat -1
    fi
  
  elif [[ "$line" == *"HEAD"* ]]; then
    if [[ "$line" == *"HEAD range test"* ]]; then
      headdat[0]=$(echo $line | grep -oP 'power \K[+-]*([0-9]+)');
    elif [[ "$line" == *"HEAD POS"* ]]; then
      headdat[1]=$(echo $line | grep -oP 'start:\K[+-]*([0-9]+)');
      headdat[2]=$(echo $line | grep -oP 'passive:\K[+-]*([0-9]+)');
      headdat[3]=$(echo $line | grep -oP 'delta:\K[+-]*([0-9]+)');
    elif [[ "$line" == *"HEAD UP"* ]]; then
      headdat[4]=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      headdat[6]=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      headdat[8]=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
    elif [[ "$line" == *"HEAD DN"* ]]; then
      headdat[5]=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      headdat[7]=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      headdat[9]=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      #write_headdat -1
    fi
  
  elif [[ "$line" == *"[RESULT:"* ]]; then
    result=$(echo "$line" | grep -oP 'RESULT:\K[0-9]+');
    write_liftdat $result
    write_headdat $result
  fi
}

gFileCnt=0; gLineCnt=0;
function parse_file()
{
  infile=$1;
  echo processing "$infile"
  dos2unix --quiet "$infile"
  lineCnt=0
  while IFS='' read -r line || [[ -n "$line" ]]; do #https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable
    lineCnt=$((lineCnt+1))
    parse_line "$line"
  done < "$infile"
  
  gFileCnt=$(($gFileCnt+1))
  gLineCnt=$(($gLineCnt+$lineCnt))
}

#parse logfiles (*.log or *.txt formats)
Tstart=$(($(date +%s%N)/1000000))
for infile in ./*.log; do if [ "$infile" != "./*.log" ]; then parse_file "$infile"; fi done
for infile in ./*.txt; do if [ "$infile" != "./*.txt" ]; then parse_file "$infile"; fi done
Tend=$(($(date +%s%N)/1000000))
Tproc=$(($Tend-$Tstart))

echo processed $gFileCnt files $gLineCnt lines in $(($Tproc))ms. avg $(($Tproc/$gLineCnt))ms per line
exit 0

