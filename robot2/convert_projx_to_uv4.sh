#!/bin/bash
python ./tools/convert_keil_uv5_to_uv4.py  syscon/sysboot.uvprojx    syscon/sysboot.uvproj
python ./tools/convert_keil_uv5_to_uv4.py  syscon/syscon.uvprojx      syscon/syscon.uvproj
