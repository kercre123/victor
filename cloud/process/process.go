package main

import (
	"anki/ipc"
	"context"
	"fmt"
	"strings"

	api "github.com/anki/sai-go-util/http/apiclient"

	"github.com/google/uuid"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

type socketMsg struct {
	n   int
	buf []byte
}

const (
	chunkHz        = 10                                      // samples every 100ms
	sampleRate     = 16000                                   // audio sample rate
	sampleBits     = 16                                      // size of each sample
	streamSize     = sampleRate / chunkHz * (sampleBits / 8) // how many bytes will be sent per stream
	hotwordMessage = "hotword"                               // message that signals to us the hotword was triggered
)

// eternally reads messages from a socket and places them on the channel
func socketReader(s ipc.Socket, ch chan<- socketMsg) {
	for {
		n, buf := s.ReadBlock()
		ch <- socketMsg{n, buf}
	}
}

type voiceContext struct {
	client  *chipper.ChipperClient
	samples []byte
	context context.Context
}

func (ctx *voiceContext) addSamples(samples []byte, cloudChan chan<- string) {
	ctx.samples = append(ctx.samples, samples...)
	if len(ctx.samples) >= streamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		samples := ctx.samples[:streamSize]
		ctx.samples = ctx.samples[streamSize:]
		go stream(ctx, samples, cloudChan)
	}
}

func stream(ctx *voiceContext, samples []byte, cloudChan chan<- string) {
	err := ctx.client.SendAudio(samples)
	if err != nil {
		fmt.Println("Cloud error:", err)
		return
	}

	// set up response routine if this is the first stream
	if ctx.context == nil {
		ctx.context = context.Background()
		go func() {
			resp, err := ctx.client.WaitForIntent(ctx.context)
			if err != nil {
				fmt.Println("CCE error:", err)
				cloudChan <- ""
				return
			}
			fmt.Println("Intent response ->", resp)
			cloudChan <- resp.Action
		}()
	}
}

// bufToGoString converts byte buffers that may be null-terminated if created in C
// to Go strings by trimming off null chars
func bufToGoString(buf []byte) string {
	return strings.Trim(string(buf), "\x00")
}

func runProcess(micSock ipc.Socket, aiSock ipc.Socket) {
	micChan := make(chan socketMsg)
	go socketReader(micSock, micChan)

	cloudChan := make(chan string)

	var ctx *voiceContext
	for {
		select {
		case msg := <-micChan:
			// handle mic messages: hotword = get ready to stream data, otherwise add samples to
			// our buffer
			if bufToGoString(msg.buf) == hotwordMessage {
				if ctx != nil {
					fmt.Println("Got hotword event while already streaming, weird...")
				}
				client, err := chipper.NewClient("", "my-device-id", uuid.New().String()[:16],
					api.WithServerURL("http://127.0.0.1:8000"))
				if err != nil {
					fmt.Println("Error creating Chipper:", err)
					continue
				}
				ctx = &voiceContext{client, make([]byte, 0, 4000), nil}
			} else if ctx != nil {
				ctx.addSamples(msg.buf, cloudChan)
			}
		case intent := <-cloudChan:
			// we got an answer from the cloud, tell mic to stop...
			n, err := micSock.Write([]byte{0, 0})
			if n != 2 || err != nil {
				fmt.Println("Mic write error:", n, err)
			}
			// and send intent to AI (unless it's empty, server error)
			if intent != "" {
				aiSock.Write([]byte(intent))
			}
			// stop streaming until we get another hotword event
			ctx = nil
		}
	}
}
