package cloudproc

import (
	"anki/chipper"
	"anki/util"
	"bytes"
	"encoding/json"
	"fmt"
	"sync"
)

type voiceContext struct {
	stream      *chipper.Stream
	samples     []byte
	audioStream chan []byte
	once        sync.Once
}

func (ctx *voiceContext) addSamples(samples []byte) {
	ctx.samples = append(ctx.samples, samples...)
	if len(ctx.samples) >= StreamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		samples := ctx.samples[:StreamSize]
		ctx.samples = ctx.samples[StreamSize:]
		ctx.audioStream <- samples
	}
}

func (ctx *voiceContext) close() error {
	var err util.Errors
	err.Append(ctx.stream.Close())
	return err.Error()
}

func newVoiceContext(stream *chipper.Stream, cloudChan chan<- string) *voiceContext {
	audioStream := make(chan []byte, 10)
	ctx := &voiceContext{
		stream:      stream,
		samples:     make([]byte, 0, StreamSize*2),
		audioStream: audioStream}

	go func() {
		for data := range ctx.audioStream {
			ctx.sendAudio(data, cloudChan)
		}
	}()

	return ctx
}

func (ctx *voiceContext) sendAudio(samples []byte, cloudChan chan<- string) {
	var err error
	sendTime := util.TimeFuncMs(func() {
		err = ctx.stream.SendAudio(samples)
	})
	if err != nil {
		fmt.Println("Cloud error:", err)
		return
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took", int(sendTime), "ms)")

	// set up response routine if this is the first stream
	ctx.once.Do(func() {
		go func() {
			resp, err := ctx.stream.WaitForIntent()
			if err != nil {
				fmt.Println("CCE error:", err)
				cloudChan <- ""
				return
			}
			fmt.Println("Intent response ->", resp)
			sendJSONResponse(resp, cloudChan)
		}()
	})
}

func sendJSONResponse(resp *chipper.IntentResult, cloudChan chan<- string) {
	outResponse := make(map[string]interface{})
	outResponse["intent"] = resp.Action
	if resp.Parameters != nil && len(resp.Parameters) > 0 {
		outResponse["params"] = resp.Parameters
	}
	buf := bytes.Buffer{}
	encoder := json.NewEncoder(&buf)
	if err := encoder.Encode(outResponse); err != nil {
		fmt.Println("JSON encode error:", err)
		cloudChan <- ""
		return
	}
	cloudChan <- buf.String()
}
