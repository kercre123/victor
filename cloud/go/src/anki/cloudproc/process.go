package cloudproc

import (
	"anki/chipper"
	"anki/util"
	"fmt"
	"io"

	"github.com/google/uuid"
)

const (
	// ChipperURL is the location of the Chipper service
	ChipperURL = "chipper-dev.api.anki.com:443"
)

var (
	// ChipperSecret should be baked in at build time and serves as the access
	// key for Chipper use
	ChipperSecret string
	verbose       bool
)

type socketMsg struct {
	buf []byte
}

const (
	// ChunkHz defines how many times per second samples should be sent
	ChunkHz = 10
	// SampleRate defines how many samples per second should be sent
	SampleRate = 16000
	// SampleBits defines how many bits each sample should contain
	SampleBits = 16
	// ChunkSamples is the number of samples that should be in each chunk
	ChunkSamples = SampleRate / ChunkHz
	// StreamSize is the size in bytes of each chunk
	StreamSize = ChunkSamples * (SampleBits / 8)
	// HotwordMessage is the sequence of bytes that defines a hotword signal over IPC
	HotwordMessage = "hotword"
)

// Process contains the data associated with an instance of the cloud process,
// and can have receivers and callbacks associated with it before ultimately
// being started with Run()
type Process struct {
	receivers []*Receiver
	intents   []io.Writer
	kill      chan struct{}
	hotword   chan struct{}
	audio     chan socketMsg
}

// AddReceiver adds the given Receiver to the list of sources the
// cloud process will listen to for data
func (p *Process) AddReceiver(r *Receiver) {
	if p.receivers == nil {
		p.receivers = make([]*Receiver, 0, 4)
	}
	if p.kill == nil {
		p.kill = make(chan struct{})
	}
	if len(p.receivers) == 0 {
		p.hotword = r.hotword
		p.audio = r.audio
	} else {
		if len(p.receivers) == 1 {
			p.hotword = make(chan struct{})
			p.audio = make(chan socketMsg)
			p.addMultiplexRoutine(p.receivers[0])
		}
		p.addMultiplexRoutine(r)
	}
	p.receivers = append(p.receivers, r)
}

func (p *Process) addMultiplexRoutine(r *Receiver) {
	go func() {
		for {
			select {
			case <-p.kill:
				return
			case <-r.hotword:
				p.hotword <- struct{}{}
			case msg := <-r.audio:
				p.audio <- msg
			}
		}
	}()
}

// AddIntentWriter adds the given Writer to the list of writers that will receive
// intents from the cloud
func (p *Process) AddIntentWriter(w io.Writer) {
	if p.intents == nil {
		p.intents = make([]io.Writer, 0, 4)
	}
	p.intents = append(p.intents, w)
}

// Run starts the cloud process, which will run until stopped on the given channel
func (p *Process) Run(stop <-chan struct{}) {
	if verbose {
		fmt.Println("Verbose logging enabled")
	}

	cloudChan := make(chan string)

	var ctx *voiceContext
procloop:
	for {
		select {
		case <-p.hotword:
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

		case msg := <-p.audio:
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
			for _, r := range p.receivers {
				n, err := r.writeBack([]byte{0, 0})
				if n != 2 || err != nil {
					fmt.Println("Mic write error:", n, err)
				}
			}

			if intent == "" {
				intent = "error"
			}

			// send intent to AI
			for _, r := range p.intents {
				n, err := r.Write([]byte(intent))
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
			if p.kill != nil {
				close(p.kill)
			}
			break procloop
		}
	}
}

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}

func logVerbose(a ...interface{}) {
	if !verbose {
		return
	}
	fmt.Println(a...)
}
