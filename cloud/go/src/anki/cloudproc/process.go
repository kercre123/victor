package cloudproc

import (
	"anki/ipc"
	"anki/util"
	"fmt"
	"strings"
	"sync"

	"github.com/anki/sai-chipper-voice/client/chipper"
	api "github.com/anki/sai-go-util/http/apiclient"
	"github.com/google/uuid"
)

const (
	// ChipperURL is the location of the Chipper service
	ChipperURL = "https://chipper-dev.api.anki.com"
)

var ChipperSecret string

type socketMsg struct {
	buf []byte
}

const (
	chunkHz        = 10                                      // samples every 100ms
	sampleRate     = 16000                                   // audio sample rate
	sampleBits     = 16                                      // size of each sample
	streamSize     = sampleRate / chunkHz * (sampleBits / 8) // how many bytes will be sent per stream
	hotwordMessage = "hotword"                               // message that signals to us the hotword was triggered
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
	client      *chipper.ChipperClient
	samples     []byte
	audioStream chan []byte
	once        sync.Once
}

func (ctx *voiceContext) addSamples(samples []byte) {
	ctx.samples = append(ctx.samples, samples...)
	if len(ctx.samples) >= streamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		samples := ctx.samples[:streamSize]
		ctx.samples = ctx.samples[streamSize:]
		ctx.audioStream <- samples
	}
}

func newVoiceContext(client *chipper.ChipperClient, cloudChan chan<- string) *voiceContext {
	audioStream := make(chan []byte, 10)
	ctx := &voiceContext{
		client:      client,
		samples:     make([]byte, 0, streamSize*2),
		audioStream: audioStream}

	go func() {
		for data := range ctx.audioStream {
			stream(ctx, data, cloudChan)
		}
	}()

	return ctx
}

func stream(ctx *voiceContext, samples []byte, cloudChan chan<- string) {
	err := ctx.client.SendAudio(samples)
	if err != nil {
		fmt.Println("Cloud error:", err)
		return
	}

	// set up response routine if this is the first stream
	ctx.once.Do(func() {
		go func() {
			resp, err := ctx.client.WaitForIntent(nil)
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

// RunProcess starts the cloud process, which will read data when available
// from the given mic connection and pass resulting intents to the given AI
// connection until the given stop channel is triggered
func RunProcess(micSock ipc.Conn, aiSock ipc.Conn, stop <-chan struct{}) {
	micChan := make(chan socketMsg)
	killreader := make(chan struct{})
	go socketReader(micSock, micChan, killreader)
	defer close(killreader)

	cloudChan := make(chan string)

	var ctx *voiceContext
procloop:
	for {
		select {
		case msg := <-micChan:
			// handle mic messages: hotword = get ready to stream data, otherwise add samples to
			// our buffer
			if bufToGoString(msg.buf) == hotwordMessage {
				if ctx != nil {
					fmt.Println("Got hotword event while already streaming, weird...")
				}
				client, err := chipper.NewClient("", ChipperSecret, "device-id",
					uuid.New().String()[:16],
					api.WithServerURL(ChipperURL))
				if err != nil {
					fmt.Println("Error creating Chipper:", err)
					continue
				}
				ctx = newVoiceContext(client, cloudChan)
			} else if ctx != nil {
				ctx.addSamples(msg.buf)
			}
		case intent := <-cloudChan:
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
