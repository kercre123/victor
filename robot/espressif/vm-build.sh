REMOTE_DIR=esp8266/espressif-fw/

rsync -av --exclude=.git ./ l:$REMOTE_DIR
ssh l "cd $REMOTE_DIR && make build"
rsync -av l:$REMOTE_DIR/*.bin ./
