"c:\cygwin\bin\sed" -n '1,106p' ../boot/build/xsboot.hex > build/xstest.hex
echo :0D3FEE000AB01EAB11CA01FFFFEF04FF0077 >> build/xstest.hex
"c:\cygwin\bin\sed" -n '2,100p' build/xspatch.hex >> build/xstest.hex
"C:\Program Files (x86)\Nordic Semiconductor\nRFgo Studio\nrfgostudio" -defaultsettings
"C:\Program Files (x86)\Nordic Semiconductor\nRFgo Studio\nrfgocli" -p build/xstest.hex -b 1 -t isp -mt le1
pause
