package stream

import (
	"anki/log"
	"anki/util"
	"bytes"
	"clad/cloud"
	"context"
	"encoding/json"
	"fmt"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

func (strm *Streamer) sendAudio(samples []byte) error {
	var err error
	sendTime := util.TimeFuncMs(func() {
		err = strm.stream.SendAudio(samples)
	})
	if err != nil {
		log.Println("Cloud error:", err)
		strm.respOnce.Do(func() {
			strm.receiver.OnError(errorReason(err), err)
		})
		return err
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took", int(sendTime), "ms)")
	return nil
}

// bufferRoutine uses channels to ensure that the main routine can always add bytes to
// the current buffer without stalling if the GRPC streaming routine is blocked on
// something and not ready for more audio yet
func (strm *Streamer) bufferRoutine(streamSize int) {
	defer close(strm.byteChan)
	defer close(strm.audioStream)
	audioBuf := make([]byte, 0, streamSize*2)
	// function to enable/disable streaming case depending on whether we have enough bytes
	// to send audio
	streamData := func() (chan<- []byte, []byte) {
		if len(audioBuf) >= streamSize {
			return strm.audioStream, audioBuf[:streamSize]
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
		case buf := <-strm.byteChan:
			audioBuf = append(audioBuf, buf...)
		case <-strm.ctx.Done():
			return
		}
	}
}

// responseRoutine should be started after streaming begins, and will wait for a response
// to send back to the main routine on the given channels
func (strm *Streamer) responseRoutine() {
	resp, err := strm.stream.WaitForResponse()
	strm.respOnce.Do(func() {
		if strm.closed {
			if err != nil {
				log.Println("Ignoring error on closed context")
			} else {
				log.Println("Ignoring response on closed context")
			}
			return
		}
		if err != nil {
			log.Println("CCE error:", err)
			strm.receiver.OnError(errorReason(err), err)
			return
		}
		log.Println("Intent response ->", resp)
		switch r := resp.(type) {
		case *chipper.IntentResult:
			sendIntentResponse(r, strm.receiver)
		case *chipper.KnowledgeGraphResponse:
			sendKGResponse(r, strm.receiver)
		}
	})
}

func sendIntentResponse(resp *chipper.IntentResult, receiver Receiver) {
	metadata := fmt.Sprintf("text: %s  confidence: %f  handler: %s",
		resp.QueryText, resp.IntentConfidence, resp.Service)
	var buf bytes.Buffer
	if resp.Parameters != nil && len(resp.Parameters) > 0 {
		encoder := json.NewEncoder(&buf)
		if err := encoder.Encode(resp.Parameters); err != nil {
			log.Println("JSON encode error:", err)
			receiver.OnError(cloud.ErrorType_Json, err)
			return
		}
	}

	receiver.OnIntent(&cloud.IntentResult{Intent: resp.Action, Parameters: buf.String(), Metadata: metadata})
}

func sendKGResponse(resp *chipper.KnowledgeGraphResponse, receiver Receiver) {
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
		receiver.OnError(cloud.ErrorType_Json, err)
		return
	}
	receiver.OnIntent(&cloud.IntentResult{
		Intent:     "intent_knowledge_response_extend",
		Parameters: buf.String(),
		Metadata:   "",
	})
}

func errorReason(err error) cloud.ErrorType {
	if err == context.DeadlineExceeded {
		return cloud.ErrorType_Timeout
	}
	return cloud.ErrorType_Server
}

var verbose bool

func logVerbose(a ...interface{}) {
	if !verbose {
		return
	}
	log.Println(a...)
}
