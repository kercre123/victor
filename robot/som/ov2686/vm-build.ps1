scp ov2686.c vm:dev_bsp/ov2686/ov2686.c
ssh --% vm 'cd dev_bsp; ./ltib -m shell <<< "cd ov2686 && make && exit"'
scp vm:dev_bsp/ov2686/ov2686.ko ./ov2686.ko
scp ov2686.ko torpedo:ov2686.ko
