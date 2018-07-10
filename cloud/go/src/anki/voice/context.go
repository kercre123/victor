package voice

import (
	"anki/log"
	"anki/util"
	"bytes"
	"clad/cloud"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"sync"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

type voiceContext struct {
	conn        *chipper.Conn
	stream      chipper.Stream
	process     *Process
	byteChan    chan []byte
	audioStream chan []byte
	respOnce    sync.Once
	closed      bool
	done        chan struct{}
}

type cloudChans struct {
	intent chan<- *cloud.IntentResult
	err    chan<- string
}

func (ctx *voiceContext) addSamples(samples []int16) {
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, samples)
	ctx.addBytes(buf.Bytes())
}

func (ctx *voiceContext) addBytes(bytes []byte) {
	ctx.byteChan <- bytes
}

func (ctx *voiceContext) close() error {
	ctx.closed = true
	close(ctx.done)
	var err util.Errors
	err.Append(ctx.stream.Close())
	err.Append(ctx.conn.Close())
	return err.Error()
}

func (p *Process) newVoiceContext(conn *chipper.Conn, stream chipper.Stream, cloudChan cloudChans) *voiceContext {
	ctx := &voiceContext{
		conn:        conn,
		stream:      stream,
		process:     p,
		byteChan:    make(chan []byte),
		audioStream: make(chan []byte, 10),
		done:        make(chan struct{})}

	// start routine to buffer communication between main routine and upload routine
	go ctx.bufferRoutine(p.StreamSize())
	// start routine to upload audio via GRPC until response or error
	go func() {
		responseInited := false
		for data := range ctx.audioStream {
			if err := ctx.sendAudio(data, cloudChan); err != nil {
				return
			}
			if !responseInited {
				go ctx.responseRoutine(cloudChan)
				responseInited = true
			}
		}
	}()

	return ctx
}

func (ctx *voiceContext) sendAudio(samples []byte, cloudChan cloudChans) error {
	var err error
	sendTime := util.TimeFuncMs(func() {
		err = ctx.stream.SendAudio(samples)
	})
	if err != nil {
		log.Println("Cloud error:", err)
		ctx.respOnce.Do(func() {
			cloudChan.err <- err.Error()
		})
		return err
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took", int(sendTime), "ms)")
	return nil
}

// bufferRoutine uses channels to ensure that the main routine can always add bytes to
// the current buffer without stalling if the GRPC streaming routine is blocked on
// something and not ready for more audio yet
func (ctx *voiceContext) bufferRoutine(streamSize int) {
	defer close(ctx.byteChan)
	defer close(ctx.audioStream)
	audioBuf := make([]byte, 0, streamSize*2)
	// function to enable/disable streaming case depending on whether we have enough bytes
	// to send audio
	streamData := func() (chan<- []byte, []byte) {
		if len(audioBuf) >= streamSize {
			return ctx.audioStream, audioBuf[:streamSize]
		}
		return nil, nil
	}
	for {
		// depending on who's ready, either add more bytes to our buffer from main routine
		// or send audio to upload routine (assuming our validation functions allow it)
		streamChan, streamBuf := streamData()
		select {
		case streamChan <- streamBuf:
			audioBuf = audioBuf[streamSize:]
		case buf := <-ctx.byteChan:
			audioBuf = append(audioBuf, buf...)
		case <-ctx.done:
			return
		}
	}
}

// responseRoutine should be started after streaming begins, and will wait for a response
// to send back to the main routine on the given channels
func (ctx *voiceContext) responseRoutine(cloudChan cloudChans) {
	resp, err := ctx.stream.WaitForResponse()
	ctx.respOnce.Do(func() {
		if ctx.closed {
			if err != nil {
				log.Println("Ignoring error on closed context")
			} else {
				log.Println("Ignoring response on closed context")
			}
			return
		}
		if err != nil {
			log.Println("CCE error:", err)
			cloudChan.err <- err.Error()
			return
		}
		log.Println("Intent response ->", resp)
		switch r := resp.(type) {
		case *chipper.IntentResult:
			sendIntentResponse(r, cloudChan)
		case *chipper.KnowledgeGraphResponse:
			sendKGResponse(r, cloudChan)
		}
	})
}

func sendIntentResponse(resp *chipper.IntentResult, cloudChan cloudChans) {
	metadata := fmt.Sprintf("text: %s  confidence: %f  handler: %s",
		resp.QueryText, resp.IntentConfidence, resp.Service)
	var buf bytes.Buffer
	if resp.Parameters != nil && len(resp.Parameters) > 0 {
		encoder := json.NewEncoder(&buf)
		if err := encoder.Encode(resp.Parameters); err != nil {
			log.Println("JSON encode error:", err)
			cloudChan.err <- "json"
			return
		}
	}

	cloudChan.intent <- &cloud.IntentResult{Intent: resp.Action, Parameters: buf.String(), Metadata: metadata}
}

func sendKGResponse(resp *chipper.KnowledgeGraphResponse, cloudChan cloudChans) {
	var buf bytes.Buffer
	params := map[string]string{
		"answer":      resp.SpokenText,
		"answer_type": resp.CommandType,
		"query_text":  resp.QueryText,
	}
	for i, d := range resp.DomainsUsed {
		params[fmt.Sprintf("domains.%d", i)] = d
	}
	encoder := json.NewEncoder(&buf)
	if err := encoder.Encode(params); err != nil {
		log.Println("JSON encode error:", err)
		cloudChan.err <- "json"
		return
	}
	cloudChan.intent <- &cloud.IntentResult{
		Intent:     "intent_knowledge_response_extend",
		Parameters: buf.String(),
		Metadata:   "",
	}
}
