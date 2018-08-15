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
rm -f $directory/data_*.csv

gDebug=0;
gCurrentFile="";
gCurrentLine=0;
gDataSel=0; #dataset selector

columnIndex=( "0"    "1"     "2"       "3"     "4"     "5"    "6"   "7"    "8"      "9"    "10" "11"   "12"    "13"      "14"    "15"    "16"   "17"  "18"   "19"     "20"   "21" "22"     "23"    "24")
columnGroups=("LIFT" "POS"   ""        ""      "SPEED" ""     "AVG" ""     "TRAVEL" ""     ""   "HEAD" "POS"   ""        ""      "SPEED" ""     "AVG" ""     "TRAVEL" ""     ""   ""       ""      "")
columnNames=( "pwr"  "start" "passive" "delta" "up"    "down" "up"  "down" "up"     "down" ""   "pwr"  "start" "passive" "delta" "up"    "down" "up"  "down" "up"     "down" ""   "result" "line#" "file")
declare -a dathigh
declare -a datlow
for val in "${columnNames[@]}"; do dathigh+=(""); datlow+=(""); done

#function aryClear() { #usage: aryClear myArray[@]
#  declare -a ary1=("${!1}"); #echo "${ary1[@]}"
#  for i in "${!ary1[@]}"; do ary1[$i]=""; done
#}

function dClear() { 
  if [ $gDataSel -gt 0 ]; then 
    for i in "${!dathigh[@]}"; do dathigh[$i]=""; done
    dathigh[10]=" "; dathigh[21]=" ";
  else
    for i in "${!datlow[@]}"; do datlow[$i]=""; done
    datlow[10]=" "; datlow[21]=" ";
  fi
}

function dSetResult()  { result=$1;                               dathigh[22]=$result;       datlow[22]=$result; }
function dSetLineNum() { linenum=$1; if [ $gDataSel -gt 0 ]; then dathigh[23]=$linenum; fi;  datlow[23]=$linenum; }
function dSetFile()    { fname=$1;   if [ $gDataSel -gt 0 ]; then dathigh[24]=$fname;   fi;  datlow[24]=$fname; }
function dSetLiftPwr() { pwr=$1; if [ $gDataSel -gt 0 ]; then dathigh[0]=$pwr;  else datlow[0]=$pwr; fi }
function dSetHeadPwr() { pwr=$1; if [ $gDataSel -gt 0 ]; then dathigh[11]=$pwr; else datlow[11]=$pwr; fi }

function dSetLiftPos() {
  start=$1; passv=$2; delta=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[1]=$start; else datlow[1]=$start; fi
  if [ $gDataSel -gt 0 ]; then dathigh[2]=$passv; else datlow[2]=$passv; fi
  if [ $gDataSel -gt 0 ]; then dathigh[3]=$delta; else datlow[3]=$delta; fi
}

function dSetLiftUp() {
  speed=$1; spdavg=$2; travel=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[4]=$speed;  else datlow[4]=$speed; fi
  if [ $gDataSel -gt 0 ]; then dathigh[6]=$spdavg; else datlow[6]=$spdavg; fi
  if [ $gDataSel -gt 0 ]; then dathigh[8]=$travel; else datlow[8]=$travel; fi
}

function dSetLiftDn() {
  speed=$1; spdavg=$2; travel=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[5]=$speed;  else datlow[5]=$speed; fi
  if [ $gDataSel -gt 0 ]; then dathigh[7]=$spdavg; else datlow[7]=$spdavg; fi
  if [ $gDataSel -gt 0 ]; then dathigh[9]=$travel; else datlow[9]=$travel; fi
}

function dSetHeadPos() {
  start=$1; passv=$2; delta=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[12]=$start; else datlow[12]=$start; fi
  if [ $gDataSel -gt 0 ]; then dathigh[13]=$passv; else datlow[13]=$passv; fi
  if [ $gDataSel -gt 0 ]; then dathigh[14]=$delta; else datlow[14]=$delta; fi
}

function dSetHeadUp() {
  speed=$1; spdavg=$2; travel=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[15]=$speed;  else datlow[15]=$speed; fi
  if [ $gDataSel -gt 0 ]; then dathigh[17]=$spdavg; else datlow[17]=$spdavg; fi
  if [ $gDataSel -gt 0 ]; then dathigh[19]=$travel; else datlow[19]=$travel; fi
}

function dSetHeadDn() {
  speed=$1; spdavg=$2; travel=$3;
  if [ $gDataSel -gt 0 ]; then dathigh[16]=$speed;  else datlow[16]=$speed; fi
  if [ $gDataSel -gt 0 ]; then dathigh[18]=$spdavg; else datlow[18]=$spdavg; fi
  if [ $gDataSel -gt 0 ]; then dathigh[20]=$travel; else datlow[20]=$travel; fi
}

function dWriteSel()
{
  dsel=$1; row=""; blanks=0; declare -a datary;
  if [ $dsel -gt 0 ]; then datary=("${dathigh[@]}"); else datary=("${datlow[@]}"); fi
  
  for val in "${datary[@]}"; do
    row="$row,$val";
    if [ "$val" == "" ]; then blanks=$((blanks+1)); fi
  done
  
  if [ $dsel -gt 0 ]; then fappend="high"; else fappend="low"; fi
  outfile=$(printf "data_%s.csv" "$fappend")
  
  if [ ! -e "$outfile" ]; then
    row1=""; row2=""; #first rows are column lables
    for val in "${columnGroups[@]}"; do row1="$row1,$val"; done
    for val in "${columnNames[@]}"; do row2="$row2,$val"; done
    echo "$row1,ECHECK" >> $outfile
    echo "$row2,blanks" >> $outfile
  fi
  
  echo "$row,$blanks" >> $outfile
}
function dWrite() { dWriteSel "0"; dWriteSel "1"; }

function parse_line()
{
  line=$1;
  
  #strip timestamp prefix '[###.######] '
  #if [[ "$line" == *"LIFT"* ]] || [[ "$line" == *"HEAD"* ]]; then
  #  if [ "${line:0:1}" == "[" ]; then line=$(echo $line | grep -oP '\[[0-9]+\.[0-9]+\]\s\K(.)*'); fi
  #fi
  
  if [[ "$line" == *"LIFT"* ]]; then
    if [[ "$line" == *"LIFT range test"* ]]; then
      #NOTE: this is the first log line of any dataset. use to re-sync
      power=$(echo $line | grep -oP 'power \K[+-]*([0-9]+)');
      if [ "$power" -ge 65 ]; then gDataSel=1; else gDataSel=0; fi
      if [ $gDebug -gt 0 ]; then echo "  found ds=$gDataSel"; fi
      dClear
      dSetLiftPwr $power
      dSetLineNum $gCurrentLine
      dSetFile $gCurrentFile
    elif [[ "$line" == *"LIFT POS"* ]]; then
      posStart=$(echo $line | grep -oP 'start:\K[+-]*([0-9]+)');
      posPassv=$(echo $line | grep -oP 'passive:\K[+-]*([0-9]+)');
      posDelta=$(echo $line | grep -oP 'delta:\K[+-]*([0-9]+)');
      dSetLiftPos $posStart $posPassv $posDelta
    elif [[ "$line" == *"LIFT UP"* ]]; then
      speed=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      spdavg=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      travel=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      dSetLiftUp $speed $spdavg $travel
    elif [[ "$line" == *"LIFT DN"* ]]; then
      speed=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      spdavg=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      travel=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      dSetLiftDn $speed $spdavg $travel
    fi
  
  elif [[ "$line" == *"HEAD"* ]]; then
    if [[ "$line" == *"HEAD range test"* ]]; then
      power=$(echo $line | grep -oP 'power \K[+-]*([0-9]+)');
      dSetHeadPwr $power
    elif [[ "$line" == *"HEAD POS"* ]]; then
      posStart=$(echo $line | grep -oP 'start:\K[+-]*([0-9]+)');
      posPassv=$(echo $line | grep -oP 'passive:\K[+-]*([0-9]+)');
      posDelta=$(echo $line | grep -oP 'delta:\K[+-]*([0-9]+)');
      dSetHeadPos $posStart $posPassv $posDelta
    elif [[ "$line" == *"HEAD UP"* ]]; then
      speed=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      spdavg=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      travel=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      dSetHeadUp $speed $spdavg $travel
    elif [[ "$line" == *"HEAD DN"* ]]; then
      speed=$(echo $line | grep -oP 'speed:\K[+-]*([0-9]+)');
      spdavg=$(echo $line | grep -oP 'avg:\K[+-]*([0-9]+)');
      travel=$(echo $line | grep -oP 'travel:\K[+-]*([0-9]+)');
      dSetHeadDn $speed $spdavg $travel
    fi
  
  elif [[ "$line" == *"[RESULT:"* ]]; then
    result=$(echo "$line" | grep -oP 'RESULT:\K[0-9]+');
    dSetResult $result
    if [ $gDebug -gt 0 ]; then echo "  writing result=$result"; fi
    dWrite
    dClear
  fi
}

gFileCnt=0; gLineCnt=0;
function parse_file()
{
  gCurrentFile=$1;
  gCurrentLine=0;
  echo processing "$gCurrentFile"
  dos2unix --quiet "$gCurrentFile"
  while IFS='' read -r line || [[ -n "$line" ]]; do #https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable
    gCurrentLine=$((gCurrentLine+1))
    parse_line "$line"
  done < "$gCurrentFile"
  
  gFileCnt=$(($gFileCnt+1))
  gLineCnt=$(($gLineCnt+$gCurrentLine))
}

#parse logfiles (*.log or *.txt formats)
Tstart=$(($(date +%s%N)/1000000))
for infile in ./*.log; do if [ "$infile" != "./*.log" ]; then parse_file "$infile"; fi done
for infile in ./*.txt; do if [ "$infile" != "./*.txt" ]; then parse_file "$infile"; fi done
Tend=$(($(date +%s%N)/1000000))
Tproc=$(($Tend-$Tstart))

echo processed $gFileCnt files $gLineCnt lines in $(($Tproc))ms. avg $(($Tproc/$gLineCnt))ms per line
exit 0

