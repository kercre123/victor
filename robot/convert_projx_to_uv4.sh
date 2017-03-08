#!/bin/bash
python ./tools/convert_keil_uv5_to_uv4.py  robot_boot.uvprojx         robot_boot.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  robot41.uvprojx            robot41.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  syscon/ota_bin.uvprojx     syscon/ota_bin.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  syscon/radio_read.uvprojx  syscon/radio_read.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  syscon/sys_boot.uvprojx    syscon/sys_boot.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  syscon/syscon.uvprojx      syscon/syscon.uvproj
