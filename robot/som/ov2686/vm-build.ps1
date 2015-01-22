clear
python code_timestamp.py
scp ov2686_code_timestamp.h vm:dev_bsp/ov2686/ov2686_code_timestamp.h
scp ov2686.c vm:dev_bsp/ov2686/ov2686.c
ssh --% vm 'cd dev_bsp; ./ltib -m shell <<< "cd ov2686 && make && exit"'
