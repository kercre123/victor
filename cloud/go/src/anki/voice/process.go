package voice

import (
	"anki/log"
	"anki/token"
	"anki/util"
	"clad/cloud"
	"fmt"
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
	// DefaultTimeout is the length of time before the process will cancel a voice request
	DefaultTimeout = 9 * time.Second
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

	intentChan := make(chan *cloud.IntentResult)
	errChan := make(chan cloudError)
	cloudChan := cloudChans{intent: intentChan, err: errChan}

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
				if !ok && mode != cloud.StreamType_KnowledgeGraph {
					p.writeError(cloud.ErrorType_InvalidMode, fmt.Errorf("unknown mode %d", mode))
					continue
				}

				var jwtToken string
				tokenTime := util.TimeFuncMs(func() {
					jwtToken, _ = p.getToken()
				})
				if jwtToken == "" && p.opts.requireToken {
					log.Println("Canceling, didn't get token")
					continue
				}

				var stream chipper.Stream
				var chipperConn *chipper.Conn
				var err error
				sessionID := uuid.New().String()[:16]
				ctxTime := util.TimeFuncMs(func() {
					opts := platformOpts
					if jwtToken != "" {
						opts = append(opts, chipper.WithAccessToken(jwtToken))
					}
					opts = append(opts, chipper.WithSessionID(sessionID))
					chipperConn, err = chipper.NewConn(ChipperURL, ChipperSecret, opts...)
					if err != nil {
						log.Println("Error getting chipper connection:", err)
						p.writeError(cloud.ErrorType_Connecting, err)
						return
					}
					streamOpts := chipper.StreamOpts{
						CompressOpts: chipper.CompressOpts{
							Compress:   p.opts.compress,
							Bitrate:    66 * 1024,
							Complexity: 0,
							FrameSize:  60},
						SaveAudio: p.opts.saveAudio,
						Timeout:   DefaultTimeout,
					}
					if mode == cloud.StreamType_KnowledgeGraph {
						stream, err = chipperConn.NewKGStream(streamOpts)
					} else {
						stream, err = chipperConn.NewIntentStream(chipper.IntentOpts{
							StreamOpts: streamOpts,
							Handler:    p.opts.handler,
							Mode:       serverMode,
						})
					}
					if err != nil {
						p.writeError(cloud.ErrorType_NewStream, err)
					}
				})
				if err != nil {
					log.Println("Error creating Chipper:", err)
					continue
				}

				// signal to engine that we got a connection; the _absence_ of this will
				// be used to detect server timeout errors
				p.writeResponse(cloud.NewMessageWithStreamOpen(&cloud.Void{}))
				ctx = p.newVoiceContext(chipperConn, stream, cloudChan)

				logVerbose("Received hotword event", serverMode, "created session", sessionID, "in",
					int(ctxTime), "ms (token", int(tokenTime), "ms)")

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

		case intent := <-intentChan:
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

		case err := <-errChan:
			logVerbose("Received error from cloud:", err)
			p.signalMicStop()
			p.writeError(err.kind, err.err)
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

func (p *Process) getToken() (string, error) {
	req := cloud.NewTokenRequestWithJwt(&cloud.JwtRequest{})
	resp, err := token.HandleRequest(req)
	if err != nil {
		log.Println("Error getting jwt token:", err)
		if p.opts.requireToken {
			p.writeError(cloud.ErrorType_Token, err)
		}
		return "", err
	}
	jwt := resp.GetJwt()
	if jwt.Error != cloud.TokenError_NoError {
		log.Println("Error code getting jwt token:", jwt.Error)
		if p.opts.requireToken {
			p.writeError(cloud.ErrorType_Token, fmt.Errorf("jwt error code %d", jwt.Error))
		}
		return "", err
	}
	return resp.GetJwt().JwtToken, nil
}

func (p *Process) writeError(reason cloud.ErrorType, err error) {
	p.writeResponse(cloud.NewMessageWithError(&cloud.IntentError{Error: reason, Extra: err.Error()}))
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
	cloud.StreamType_Normal:    pb.RobotMode_VOICE_COMMAND,
	cloud.StreamType_Blackjack: pb.RobotMode_GAME,
}

type cloudError struct {
	kind cloud.ErrorType
	err  error
}
