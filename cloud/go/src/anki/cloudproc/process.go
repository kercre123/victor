package cloudproc

import (
	"anki/chipper"
	"anki/ipc"
	"anki/util"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/google/uuid"
)

const (
	// ChipperURL is the location of the Chipper service
	ChipperURL = "https://chipper-dev.api.anki.com"
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

// reads messages from a socket and places them on the channel
func socketReader(s ipc.Conn, ch chan<- socketMsg, kill <-chan struct{}) {
	for {
		if util.CanSelect(kill) {
			return
		}

		buf := s.ReadBlock()
		if buf != nil && len(buf) > 0 {
			ch <- socketMsg{buf}
		}
	}
}

type voiceContext struct {
	client      *chipper.Client
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

func newVoiceContext(client *chipper.Client, cloudChan chan<- string) *voiceContext {
	audioStream := make(chan []byte, 10)
	ctx := &voiceContext{
		client:      client,
		samples:     make([]byte, 0, StreamSize*2),
		audioStream: audioStream}

	go func() {
		for data := range ctx.audioStream {
			stream(ctx, data, cloudChan)
		}
	}()

	return ctx
}

func stream(ctx *voiceContext, samples []byte, cloudChan chan<- string) {
	callStart := time.Now()
	err := ctx.client.SendAudio(samples)
	if err != nil {
		fmt.Println("Cloud error:", err)
		return
	}
	logVerbose("Sent", len(samples), "bytes to Chipper (call took",
		time.Now().Sub(callStart).Nanoseconds()/int64(time.Millisecond/time.Nanosecond),
		"ms)")

	// set up response routine if this is the first stream
	ctx.once.Do(func() {
		go func() {
			resp, err := ctx.client.WaitForIntent()
			if err != nil {
				fmt.Println("CCE error:", err)
				cloudChan <- ""
				return
			}
			fmt.Println("Intent response ->", resp)
			cloudChan <- resp.Action
		}()
	})
}

// bufToGoString converts byte buffers that may be null-terminated if created in C
// to Go strings by trimming off null chars
func bufToGoString(buf []byte) string {
	return strings.Trim(string(buf), "\x00")
}

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}

// RunProcess starts the cloud process, which will read data when available
// from the given mic connection and pass resulting intents to the given AI
// connection until the given stop channel is triggered
func RunProcess(micSock ipc.Conn, aiSock ipc.Conn, testChan chan []byte, stop <-chan struct{}) {
	if verbose {
		fmt.Println("Verbose logging enabled")
	}
	micChan := make(chan socketMsg)
	killreader := make(chan struct{})
	go socketReader(micSock, micChan, killreader)
	defer close(killreader)

	// if testSock is provided, multiplex it with the mic channel
	var audioChan chan socketMsg
	if testChan != nil {
		audioChan = make(chan socketMsg)
		go func() {
			for {
				select {
				case msg := <-micChan:
					audioChan <- msg
				case msg := <-testChan:
					audioChan <- socketMsg{msg}
				case <-killreader:
					return
				}
			}
		}()
	} else {
		audioChan = micChan
	}

	cloudChan := make(chan string)

	var ctx *voiceContext
procloop:
	for {
		select {
		case msg := <-audioChan:
			// handle mic messages: hotword = get ready to stream data, otherwise add samples to
			// our buffer
			if bufToGoString(msg.buf) == HotwordMessage {
				if ctx != nil {
					fmt.Println("Got hotword event while already streaming, weird...")
				}
				client, err := chipper.NewClient(ChipperURL, ChipperSecret, "device-id",
					uuid.New().String()[:16])
				if err != nil {
					fmt.Println("Error creating Chipper:", err)
					continue
				}
				logVerbose("Received hotword event")
				ctx = newVoiceContext(client, cloudChan)
			} else if ctx != nil {
				logVerbose("Received", len(msg.buf), "bytes from mic")
				ctx.addSamples(msg.buf)
			} else {
				logVerbose("No active context, discarding", len(msg.buf), "bytes")
			}
		case intent := <-cloudChan:
			logVerbose("Received intent from cloud:", intent)
			// we got an answer from the cloud, tell mic to stop...
			n, err := micSock.Write([]byte{0, 0})
			if n != 2 || err != nil {
				fmt.Println("Mic write error:", n, err)
			}
			// and send intent to AI (unless it's empty, server error)
			if intent != "" {
				n, err := aiSock.Write([]byte(intent))
				if n != len(intent) || err != nil {
					fmt.Println("AI write error:", n, err)
				}
			}
			// stop streaming until we get another hotword event
			close(ctx.audioStream)
			ctx = nil
		case <-stop:
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
