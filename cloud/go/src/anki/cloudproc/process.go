package cloudproc

import (
	"anki/log"
	"anki/util"
	"bytes"
	"clad/cloud"
	"context"
	"encoding/json"
	"fmt"
	"net"
	"strings"
	"time"

	"github.com/anki/sai-chipper-voice/client/chipper"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
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
	intents   []MsgSender
	kill      chan struct{}
	msg       chan messageEvent
	opts      options
}

// AddReceiver adds the given Receiver to the list of sources the
// cloud process will listen to for data
func (p *Process) AddReceiver(r *Receiver) {
	if p.receivers == nil {
		p.receivers = make([]*Receiver, 0, 4)
		p.msg = make(chan messageEvent)
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
	msg    *cloud.Message
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
			}
		}
	}()
}

// AddIntentWriter adds the given Writer to the list of writers that will receive
// intents from the cloud
func (p *Process) AddIntentWriter(s MsgSender) {
	if p.intents == nil {
		p.intents = make([]MsgSender, 0, 4)
	}
	p.intents = append(p.intents, s)
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

	cloudIntent := make(chan *cloud.IntentResult)
	cloudError := make(chan string)
	cloudChan := cloudChans{intent: cloudIntent, err: cloudError}

	var ctx *voiceContext
procloop:
	for {
		select {
		case msg := <-p.msg:
			switch msg.msg.Tag() {
			case cloud.MessageTag_Hotword:
				// hotword = get ready to stream data
				if ctx != nil {
					log.Println("Got hotword event while already streaming, weird...")
					if err := ctx.close(); err != nil {
						log.Println("Error closing context:")
					}
				}

				// if this is from a test receiver, notify the mic to send the AI a hotword on our behalf
				if msg.isTest {
					p.writeMic(cloud.NewMessageWithTestStarted(&cloud.Void{}))
				}

				mode := msg.msg.GetHotword().Mode
				serverMode, ok := modeMap[mode]
				if !ok {
					p.writeError("mode", fmt.Sprint("unknown mode:", mode))
					continue
				}

				// TEMP HACK: fake knowledge graph results until houndify is hooked up on server
				if mode == cloud.StreamType_KnowledgeGraph {
					p.fakeKnowledgeGraphResponse()
					continue
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
						SaveAudio: p.opts.saveAudio,
						Mode:      serverMode})
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

				logVerbose("Received hotword event", serverMode, "created context in", int(ctxTime), "ms")

			case cloud.MessageTag_DebugFile:
				p.writeResponse(msg.msg)

			case cloud.MessageTag_Audio:
				// add samples to our buffer
				buf := msg.msg.GetAudio().Data
				if ctx != nil {
					ctx.addSamples(buf)
				} else {
					logVerbose("No active context, discarding", len(buf), "samples")
				}
			}

		case intent := <-cloudIntent:
			logVerbose("Received intent from cloud:", intent)
			// we got an answer from the cloud, tell mic to stop...
			p.signalMicStop()

			// send intent to AI
			p.writeResponse(cloud.NewMessageWithResult(intent))

			// stop streaming until we get another hotword event
			if err := ctx.close(); err != nil {
				log.Println("Error closing context:")
			}
			ctx = nil

		case err := <-cloudError:
			logVerbose("Received error from cloud:", err)
			p.signalMicStop()
			p.writeError("server", err)
			if err := ctx.close(); err != nil {
				log.Println("Error closing context:")
			}
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
	p.writeResponse(cloud.NewMessageWithError(&cloud.IntentError{Error: reason, Extra: extra}))
}

func (p *Process) writeResponse(response *cloud.Message) {
	for _, r := range p.intents {
		err := r.Send(response)
		if err != nil {
			log.Println("AI write error:", err)
		}
	}
}

func (p *Process) signalMicStop() {
	p.writeMic(cloud.NewMessageWithStopSignal(&cloud.Void{}))
}

func (p *Process) writeMic(msg *cloud.Message) {
	for _, r := range p.receivers {
		err := r.writeBack(msg)
		if err != nil {
			log.Println("Mic write error:", err)
		}
	}
}

func logVerbose(a ...interface{}) {
	if !verbose {
		return
	}
	log.Println(a...)
}

var modeMap = map[cloud.StreamType]pb.RobotMode{
	cloud.StreamType_Normal:         pb.RobotMode_VOICE_COMMAND,
	cloud.StreamType_Blackjack:      pb.RobotMode_GAME,
	cloud.StreamType_KnowledgeGraph: pb.RobotMode_KNOWLEDGE_GRAPH,
}

func (p *Process) fakeKnowledgeGraphResponse() {
	// spawn goroutine to wait 3 seconds and fake response
	go func() {
		time.Sleep(3 * time.Second)
		fakeParams := map[string]string{
			"answer":      "The San Jose Sharks will win the Stanley Cup in 2019",
			"answer_type": "InformationCommand",
			"query_text":  "who will win the 2019 stanley cup",
			"domains.0":   "sportsball",
			"domains.1":   "Query Glue",
		}
		var buf bytes.Buffer
		json.NewEncoder(&buf).Encode(fakeParams)
		result := &cloud.IntentResult{
			Intent:     "intent_knowledge_response_extend",
			Parameters: buf.String(),
			Metadata:   "temp fake response",
		}
		msg := cloud.NewMessageWithResult(result)
		p.signalMicStop()
		logVerbose("Sending fake KG response", msg)
		p.writeResponse(msg)
	}()
}
