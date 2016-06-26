"c:\cygwin\bin\sed" -n '1,110p' ../boot/build/xsboot.hex > build/xstest.hex
echo :0F3FEE000AB01EAB11CA01FFFFDF06FF200E0055 >> build/xstest.hex
"c:\cygwin\bin\sed" -n '2,100p' build/xspatch.hex >> build/xstest.hex
rem "C:\Program Files (x86)\Nordic Semiconductor\nRFgo Studio\nrfgostudio" -defaultsettings
"C:\Program Files (x86)\Nordic Semiconductor\nRFgo Studio\nrfgocli" -p build/xstest.hex -b 1 -t isp -mt le1
pause
