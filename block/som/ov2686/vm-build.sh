python code_timestamp.py
touch ov2686.c
rsync -v *.{h,c} vm:dev_bsp/ov2686/
ssh vm 'cd dev_bsp; ./ltib -m shell <<< "cd ov2686 && make && exit"; cp -v ov2686/*.{c,h} rpm/BUILD/linux-3.0/drivers/media/video/'
