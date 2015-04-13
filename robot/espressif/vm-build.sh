REMOTE_DIR=esp8266/udp-throughput-test/

rsync -av --exclude=.git ./ l:$REMOTE_DIR
ssh l "cd $REMOTE_DIR && make build"
rsync -av l:$REMOTE_DIR/*.bin ./
