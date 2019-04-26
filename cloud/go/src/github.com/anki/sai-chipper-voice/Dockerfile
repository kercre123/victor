FROM anki/busybox
ADD liblexvad.so /usr/local/lib/liblexvad.so
ADD libopus.so.0 /usr/local/lib/libopus.so.0
ENV LD_LIBRARY_PATH /usr/local/lib:${LD_LIBRARY_PATH}
ADD bin/sai-chipper-voice ./sai-chipper-voice
CMD ./sai-chipper-voice start
