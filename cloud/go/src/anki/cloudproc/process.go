package cloudproc

import (
	"anki/log"
	"anki/util"
	"context"
	"encoding/json"
	"io"
	"net"
	"strings"

	"github.com/anki/sai-chipper-voice/client/chipper"
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
	platformOpts  []chipper.ConnOpt
)

type socketMsg struct {
	buf []byte
}

const (
	// DefaultChunkMs is the default value for how often audio is sent to the cloud
	DefaultChunkMs = 120
	// SampleRate defines how many samples per second should be sent
	SampleRate = 16000
	// SampleBits defines how many bits each sample should contain
	SampleBits = 16
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
	msg       chan messageEvent
	audio     chan socketMsg
	opts      options
}

// AddReceiver adds the given Receiver to the list of sources the
// cloud process will listen to for data
func (p *Process) AddReceiver(r *Receiver) {
	if p.receivers == nil {
		p.receivers = make([]*Receiver, 0, 4)
		p.msg = make(chan messageEvent)
		p.audio = make(chan socketMsg)
	}
	if p.kill == nil {
		p.kill = make(chan struct{})
	}
	p.addMultiplexRoutine(r)
	p.receivers = append(p.receivers, r)
}

// AddTestReceiver adds the given Receiver to the list of sources the
// cloud process will listen to for data. Additionally, it will be
// marked as a test receiver, which means that data sent on this
// receiver will send a message to the mic requesting it notify the
// AI of a hotword event on our behalf.
func (p *Process) AddTestReceiver(r *Receiver) {
	r.isTest = true
	p.AddReceiver(r)
}

type messageEvent struct {
	msg    string
	isTest bool
}

func (p *Process) addMultiplexRoutine(r *Receiver) {
	go func() {
		for {
			select {
			case <-p.kill:
				return
			case msg := <-r.msg:
				p.msg <- messageEvent{msg: msg, isTest: r.isTest}
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
func (p *Process) Run(options ...Option) {
	if verbose {
		log.Println("Verbose logging enabled")
	}
	// set default options before processing user options
	p.opts.chunkMs = DefaultChunkMs
	for _, opt := range options {
		opt(&p.opts)
	}

	cloudIntent := make(chan string)
	cloudError := make(chan string)
	cloudChan := cloudChans{intent: cloudIntent, err: cloudError}

	var ctx *voiceContext
procloop:
	for {
		select {
		case msg := <-p.msg:
			switch {
			case msg.msg == HotwordMessage:
				// hotword = get ready to stream data
				if ctx != nil {
					log.Println("Got hotword event while already streaming, weird...")
					close(ctx.audioStream)
					ctx.close()
				}

				// if this is from a test receiver, notify the mic to send the AI a hotword on our behalf
				if msg.isTest {
					p.writeMic([]byte{0, 1})
				}

				var stream *chipper.Stream
				var chipperConn *chipper.Conn
				var err error
				ctxTime := util.TimeFuncMs(func() {
					chipperConn, err = chipper.NewConn(ChipperURL, ChipperSecret, platformOpts...)
					if err != nil {
						log.Println("Error getting chipper connection:", err)
						p.writeError("connecting", err.Error())
						return
					}
					stream, err = chipperConn.NewStream(chipper.StreamOpts{
						CompressOpts: chipper.CompressOpts{
							Compress:   p.opts.compress,
							Bitrate:    66 * 1024,
							Complexity: 0,
							FrameSize:  60},
						SessionId: uuid.New().String()[:16],
						Handler:   p.opts.handler,
						SaveAudio: p.opts.saveAudio})
					if err != nil {
						p.writeError("newstream", err.Error())
						// debug cause of lookup failure
						var debug string
						addrs, err := net.DefaultResolver.LookupHost(context.Background(), "chipper-dev.api.anki.com")
						if err != nil {
							debug += "lookup_err: " + err.Error()
						} else {
							debug += "lookup_ips: " + strings.Join(addrs, "/")
						}
						debug += "   "
						_, err = net.Dial("tcp", "chipper-dev.api.anki.com:443")
						if err != nil {
							debug += "dial_err: " + err.Error()
						} else {
							debug += "dial_success"
						}
						p.writeError("newstream_debug", debug)
					}
				})
				if err != nil {
					log.Println("Error creating Chipper:", err)
					continue
				}

				ctx = p.newVoiceContext(chipperConn, stream, cloudChan)

				logVerbose("Received hotword event, created context in", int(ctxTime), "ms")

			case len(msg.msg) > 4 && msg.msg[:4] == "file":
				p.writeResponse([]byte("{\"debug\":\"" + msg.msg[4:] + "\"}"))
			}

		case msg := <-p.audio:
			// add samples to our buffer
			if ctx != nil {
				ctx.addSamples(msg.buf)
			} else {
				logVerbose("No active context, discarding", len(msg.buf), "bytes")
			}

		case intent := <-cloudIntent:
			logVerbose("Received intent from cloud:", intent)
			// we got an answer from the cloud, tell mic to stop...
			p.signalMicStop()

			// send intent to AI
			p.writeResponse([]byte(intent))

			// stop streaming until we get another hotword event
			close(ctx.audioStream)
			ctx.close()
			ctx = nil

		case err := <-cloudError:
			logVerbose("Received error from cloud:", err)
			p.signalMicStop()
			p.writeError("server", err)
			close(ctx.audioStream)
			ctx.close()
			ctx = nil

		case <-p.opts.stop:
			logVerbose("Received stop notification")
			if p.kill != nil {
				close(p.kill)
			}
			break procloop
		}
	}
}

// ChunkSamples is the number of samples that should be in each chunk
func (p *Process) ChunkSamples() int {
	return SampleRate * int(p.opts.chunkMs) / 1000
}

// StreamSize is the size in bytes of each chunk
func (p *Process) StreamSize() int {
	return p.ChunkSamples() * (SampleBits / 8)
}

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}

func (p *Process) writeError(reason string, extra string) {
	jsonMap := map[string]string{"error": reason, "extra": extra}
	buf, err := json.Marshal(jsonMap)
	if err != nil {
		log.Println("Couldn't encode json error for "+reason+": ", err)
	}
	p.writeResponse(buf)
}

func (p *Process) writeResponse(response []byte) {
	for _, r := range p.intents {
		n, err := r.Write(response)
		if n != len(response) || err != nil {
			log.Println("AI write error:", n, err)
		}
	}
}

func (p *Process) signalMicStop() {
	p.writeMic([]byte{0, 0})
}

func (p *Process) writeMic(buf []byte) {
	for _, r := range p.receivers {
		n, err := r.writeBack(buf)
		if n != len(buf) || err != nil {
			log.Println("Mic write error:", n, err)
		}
	}
}

func logVerbose(a ...interface{}) {
	if !verbose {
		return
	}
	log.Println(a...)
}
