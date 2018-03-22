package cloudproc

import (
	"anki/log"
	"anki/util"
	"bytes"
	"encoding/json"
	"sync"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

type voiceContext struct {
	conn        *chipper.Conn
	stream      *chipper.Stream
	process     *Process
	samples     []byte
	audioStream chan []byte
	once        sync.Once
}

type cloudChans struct {
	intent chan<- string
	err    chan<- string
}

func (ctx *voiceContext) addSamples(samples []byte) {
	ctx.samples = append(ctx.samples, samples...)
	streamSize := ctx.process.StreamSize()
	if len(ctx.samples) >= streamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		samples := ctx.samples[:streamSize]
		ctx.samples = ctx.samples[streamSize:]
		ctx.audioStream <- samples
	}
}

func (ctx *voiceContext) close() error {
	var err util.Errors
	err.Append(ctx.stream.Close())
	err.Append(ctx.conn.Close())
	return err.Error()
}

func (p *Process) newVoiceContext(conn *chipper.Conn, stream *chipper.Stream, cloudChan cloudChans) *voiceContext {
	audioStream := make(chan []byte, 10)
	ctx := &voiceContext{
		conn:        conn,
		stream:      stream,
		process:     p,
		samples:     make([]byte, 0, p.StreamSize()*2),
		audioStream: audioStream}

	go func() {
		for data := range ctx.audioStream {
			ctx.sendAudio(data, cloudChan)
		}
	}()

	return ctx
}

func (ctx *voiceContext) sendAudio(samples []byte, cloudChan cloudChans) {
	var err error
	sendTime := util.TimeFuncMs(func() {
		err = ctx.stream.SendAudio(samples)
	})
	if err != nil {
		log.Println("Cloud error:", err)
		cloudChan.err <- err.Error()
		return
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took", int(sendTime), "ms)")

	// set up response routine if this is the first stream
	ctx.once.Do(func() {
		go func() {
			resp, err := ctx.stream.WaitForIntent()
			if err != nil {
				log.Println("CCE error:", err)
				cloudChan.err <- err.Error()
				return
			}
			log.Println("Intent response ->", resp)
			sendJSONResponse(resp, cloudChan)
		}()
	})
}

func sendJSONResponse(resp *chipper.IntentResult, cloudChan cloudChans) {
	outResponse := make(map[string]interface{})
	outResponse["intent"] = resp.Action
	if resp.Parameters != nil && len(resp.Parameters) > 0 {
		outResponse["params"] = resp.Parameters
	}
	buf := bytes.Buffer{}
	encoder := json.NewEncoder(&buf)
	if err := encoder.Encode(outResponse); err != nil {
		log.Println("JSON encode error:", err)
		cloudChan.err <- "json"
		return
	}
	cloudChan.intent <- buf.String()
}
