package cloudproc

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
	bytes       []byte
	audioStream chan []byte
	initOnce    sync.Once
	respOnce    sync.Once
	closed      bool
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
	ctx.bytes = append(ctx.bytes, bytes...)
	streamSize := ctx.process.StreamSize()
	if len(ctx.bytes) >= streamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		data := ctx.bytes[:streamSize]
		ctx.bytes = ctx.bytes[streamSize:]
		ctx.audioStream <- data
	}
}

func (ctx *voiceContext) close() error {
	ctx.closed = true
	close(ctx.audioStream)
	var err util.Errors
	err.Append(ctx.stream.Close())
	err.Append(ctx.conn.Close())
	return err.Error()
}

func (p *Process) newVoiceContext(conn *chipper.Conn, stream chipper.Stream, cloudChan cloudChans) *voiceContext {
	audioStream := make(chan []byte, 10)
	ctx := &voiceContext{
		conn:        conn,
		stream:      stream,
		process:     p,
		bytes:       make([]byte, 0, p.StreamSize()*2),
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
		ctx.respOnce.Do(func() {
			cloudChan.err <- err.Error()
		})
		return
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took", int(sendTime), "ms)")

	// set up response routine if this is the first stream
	ctx.initOnce.Do(func() {
		go func() {
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
		}()
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
