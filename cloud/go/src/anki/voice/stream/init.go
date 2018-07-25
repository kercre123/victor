package stream

func (strm *Streamer) init(streamSize int) {
	// start routine to buffer communication between main routine and upload routine
	go strm.bufferRoutine(streamSize)

	// connect to server
	if err := strm.connect(); err != nil {
		strm.cancel()
		return
	}

	// start routine to upload audio via GRPC until response or error
	go func() {
		responseInited := false
		for data := range strm.audioStream {
			if err := strm.sendAudio(data); err != nil {
				return
			}
			if !responseInited {
				go strm.responseRoutine()
				responseInited = true
			}
		}
	}()
}
