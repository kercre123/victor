package cloudproc

import (
	"anki/chipper"
	"anki/util"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"sync"

	"github.com/google/uuid"
)

const (
	// ChipperURL is the location of the Chipper service
	ChipperURL = "chipper-dev.api.anki.com:443"
)

var (
	ChipperSecret string
	verbose       bool
)

type socketMsg struct {
	buf []byte
}

const (
	ChunkHz        = 10                              // samples every 100ms
	SampleRate     = 16000                           // audio sample rate
	SampleBits     = 16                              // size of each sample
	ChunkSamples   = SampleRate / ChunkHz            // how many samples are in each chunk
	StreamSize     = ChunkSamples * (SampleBits / 8) // how many bytes will be sent per stream
	HotwordMessage = "hotword"                       // message that signals to us the hotword was triggered
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
			sendAudio(ctx, data, cloudChan)
		}
	}()

	return ctx
}

func sendAudio(ctx *voiceContext, samples []byte, cloudChan chan<- string) {
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
			sendJsonResponse(resp, cloudChan)
		}()
	})
}

func sendJsonResponse(resp *chipper.IntentResult, cloudChan chan<- string) {
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

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}

// RunProcess starts the cloud process, which will read data when available
// from the given mic connection and pass resulting intents to the given AI
// connection until the given stop channel is triggered
func RunProcess(micPipe MicPipe, aiSock io.Writer, stop <-chan struct{}) {
	if verbose {
		fmt.Println("Verbose logging enabled")
	}

	cloudChan := make(chan string)

	var ctx *voiceContext
procloop:
	for {
		select {
		case <-micPipe.Hotword():
			// hotword = get ready to stream data
			if ctx != nil {
				fmt.Println("Got hotword event while already streaming, weird...")
			}
			var stream *chipper.Stream
			var chipperConn *chipper.Conn
			var err error
			ctxTime := util.TimeFuncMs(func() {
				chipperConn, err = chipper.NewConn(ChipperURL, ChipperSecret, "device-id")
				if err != nil {
					fmt.Println("Error getting chipper connection:", err)
					return
				}
				stream, err = chipperConn.NewStream(chipper.StreamOpts{
					SessionId: uuid.New().String()[:16]})
			})
			if err != nil {
				fmt.Println("Error creating Chipper:", err)
				continue
			}

			ctx = newVoiceContext(stream, cloudChan)

			logVerbose("Received hotword event, created context in", int(ctxTime), "ms")

		case msg := <-micPipe.Audio():
			// add samples to our buffer
			if ctx != nil {
				logVerbose("Received", len(msg.buf), "bytes from mic")
				ctx.addSamples(msg.buf)
			} else {
				logVerbose("No active context, discarding", len(msg.buf), "bytes")
			}

		case intent := <-cloudChan:
			logVerbose("Received intent from cloud:", intent)
			// we got an answer from the cloud, tell mic to stop...
			n, err := micPipe.writeMic([]byte{0, 0})
			if n != 2 || err != nil {
				fmt.Println("Mic write error:", n, err)
			}

			if intent == "" {
				intent = "error"
			}

			// send intent to AI
			if aiSock != nil {
				n, err = aiSock.Write([]byte(intent))
				if n != len(intent) || err != nil {
					fmt.Println("AI write error:", n, err)
				}
			}

			// stop streaming until we get another hotword event
			close(ctx.audioStream)
			ctx.close()
			ctx = nil
		case <-stop:
			logVerbose("Received stop notification")
			break procloop
		}
	}
}

func logVerbose(a ...interface{}) {
	if !verbose {
		return
	}
	fmt.Println(a...)
}
