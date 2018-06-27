#!/bin/bash
#cd "$(dirname ${BASH_SOURCE[0]})"

#BREIF: parse CUBE fixture logfiles to collect LED data (arrays of current usage)

#sample log section of interest:
#[004.699397] base current 2mA
#[004.699539] All.RED current 8mA ok
#[004.699679] All.GRN current 11mA ok
#[004.700083] All.BLU current 8mA ok
#[004.700222] D1.RED current 26mA ok
#[004.700344] D1.GRN current 27mA --FAIL--
#[004.700460] D1.BLU current 25mA ok
#[004.700577] D2.RED current 27mA ok
#[004.700694] D2.GRN current 28mA ok
#[004.700811] D2.BLU current 26mA ok
#[004.700929] D3.RED current 26mA ok
#[004.701044] D3.GRN current 28mA ok
#[004.701162] D3.BLU current 25mA ok
#[004.701280] D4.RED current 27mA ok
#[004.701396] D4.GRN current 27mA --FAIL--
#[004.701513] D4.BLU current 27mA ok
#[004.701629] [RESULT:707]

directory="$(dirname ${BASH_SOURCE[0]})"
outfile=$directory/logparse_cube1.csv
rm -f $outfile

declare -a leds
leds+=("base");
leds+=("All.RED"); leds+=("All.GRN"); leds+=("All.BLU");
leds+=("D1.RED");  leds+=("D1.GRN");  leds+=("D1.BLU");
leds+=("D2.RED");  leds+=("D2.GRN");  leds+=("D2.BLU");
leds+=("D3.RED");  leds+=("D3.GRN");  leds+=("D3.BLU");
leds+=("D4.RED");  leds+=("D4.GRN");  leds+=("D4.BLU");
leds+=("file");

declare -a values
for val in "${leds[@]}"; do values+=(""); done

function write_maths()
{
  numrows=$1;
  echo "MIN,=MIN(B2:B$numrows)" >> $outfile
  echo "MAX,=MAX(B2:B$numrows)" >> $outfile
  echo "AVG,=AVERAGE(B2:B$numrows)" >> $outfile
  echo "VAR,=VAR.P(B2:B$numrows)" >> $outfile
  
  echo total rows written: $numrows
}

numrows=0
function write_row()
{
  infile=$1; linenum=$2; hasdata=0;
  for val in "${values[@]}"; do if [ "val" != "" ]; then hasdata=1; fi done
  
  #dump a row to the outfile
  if [ $hasdata -gt 0 ]; then
    row="";
    echo "writing row"
    for i in "${!values[@]}"; do
      if [[ "${leds[$i]}" == "file" ]]; then values[$i]="$infile"; fi
      row="$row,${values[$i]}"
    done
    echo $row >> $outfile
    numrows=$((numrows+1))
  fi
  
  #XXX: grep/regex is randomly failing. varies on each run for the same dataset
  #XXX: fail if we're missing led data
  errcheck=1
  if [ $errcheck -gt 0 ]; then
    for i in "${!values[@]}"; do
      if [ "${values[$i]}" == "" ]; then
        echo "------PARSE FAIL? incomplete led dataset------"
        echo "$infile" line "$linenum"
        echo "${!values[@]}"
        echo "${values[@]}"
        exit 1
      fi
    done
  fi
  
  #clear data
  for i in "${!values[@]}"; do values[$i]=""; done
}

function log_current()
{
  led=$1; current=$2; infile=$3; line=$4; linenum=$5;
  
  #about to overwrite a value? write row and move to next dataset
  for i in "${!leds[@]}"; do
    if [ "${leds[$i]}" == "$led" ]; then
      if [ "${values[$i]}" != "" ]; then write_row "$infile" "$linenum" ; fi
      break
    fi
  done
  
  #record new measurement
  for i in "${!leds[@]}"; do
    if [ "${leds[$i]}" == "$led" ]; then
      values[$i]=$current
      break
    fi
  done
  
  echo "  $led ${current}mA -- $line (line $linenum)"
  
  #XXX: grep/regex is randomly failing. varies on each run for the same dataset
  #XXX: fail if led/group parse returned empty
  if [ "$led" == "" ]; then
    echo "------REGEX FAIL: led group is blank------"
    echo "$infile" line "$linenum"
    echo "${!leds[@]}"
    echo "${leds[@]}"
    exit 1
  fi
}

#init column lables
for i in "${!leds[@]}"; do values[${i}]="${leds[$i]}"; done
write_row "logfile"

#parse logfiles
for infile in ./*.log; do
  echo processing "$infile"
  linenum=0
  while IFS='' read -r line || [[ -n "$line" ]]; do #https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable
    linenum=$((linenum+1))
    
    #filter lines without current measurements (much faster than regex every line)
    if echo $line | grep -q "current"; then
      
      #strip timestamp prefix '[###.######] '
      if [ "${line:0:1}" == "[" ]; then 
        line=$(echo $line | grep -oP '\[[0-9]+\.[0-9]+\]\s\K(.)*');
      fi
      
      #parse the current measurement value
      current=$(echo $line | grep -oP 'current \K([0-9]+)')
      
      #skip CUBEBAT startup current measurements
      if [ "$current" != "" ]; then
        led=$(echo $line | grep -oP '^[\w\d\.]+') #parse the led name for this measurement
        log_current "$led" "$current" "$infile" "$line" "$linenum"
      fi
    fi
  done < "$infile"
  write_row "$infile"
done

write_maths $numrows

exit 0

