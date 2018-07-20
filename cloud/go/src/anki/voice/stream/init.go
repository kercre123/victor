package stream

func (ctx *Streamer) init(streamSize int) {
	// start routine to buffer communication between main routine and upload routine
	go ctx.bufferRoutine(streamSize)

	// connect to server
	if err := ctx.connect(); err != nil {
		// error will have been written back to client, who should close us
		return
	}

	// start routine to upload audio via GRPC until response or error
	go func() {
		responseInited := false
		for data := range ctx.audioStream {
			if err := ctx.sendAudio(data); err != nil {
				return
			}
			if !responseInited {
				go ctx.responseRoutine()
				responseInited = true
			}
		}
	}()
}
