package main

import (
	"anki/ipc"
	"fmt"
)

type socketMsg struct {
	n   int
	buf []byte
}

const (
	chunkHz    = 10                                      // samples every 100ms
	sampleRate = 16000                                   // audio sample rate
	sampleBits = 16                                      // size of each sample
	streamSize = sampleRate / chunkHz * (sampleBits / 8) // how many bytes will be sent per stream
)

// eternally reads messages from a socket and places them on the channel
func socketReader(s ipc.Socket, ch chan<- socketMsg) {
	for {
		n, buf := s.ReadBlock()
		ch <- socketMsg{n, buf}
	}
}

type voiceContext struct {
	cloud   *CloudContext
	samples []byte
}

func (ctx *voiceContext) addSamples(samples []byte, cloudChan chan<- string) {
	ctx.samples = append(ctx.samples, samples...)
	if len(ctx.samples) >= streamSize {
		// we have enough samples to stream - slice them off and pass them to another
		// thread for sending to server
		samples := ctx.samples[:streamSize]
		ctx.samples = ctx.samples[streamSize:]
		go stream(ctx.cloud, samples, cloudChan)
	}
}

func stream(ctx *CloudContext, samples []byte, cloudChan chan<- string) {
	resp := ctx.StreamData(samples)
	if resp.Err != "" {
		fmt.Println("Cloud error:", resp.Err)
		cloudChan <- ""
	} else if resp.IsFinal {
		cloudChan <- resp.Result.Action
	}
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
			if string(msg.buf) == "hotword" {
				if ctx != nil {
					fmt.Println("Got hotword event while already streaming, weird...")
				}
				ctx = &voiceContext{NewCloudContext("MyDevice"), make([]byte, 0, 4000)}
			} else if ctx != nil {
				ctx.addSamples(msg.buf, cloudChan)
			}
		case intent := <-cloudChan:
			// we got an answer from the cloud, tell mic to stop...
			n, err := micSock.Write([]byte{0})
			if n != 1 || err != nil {
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
