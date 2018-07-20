package voice

import (
	"anki/log"
	"anki/token"
	"anki/voice/stream"
	"clad/cloud"
	"fmt"
	"strings"
	"time"

	"github.com/anki/sai-chipper-voice/client/chipper"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
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

type ctxReceiver struct {
	intent chan *cloud.IntentResult
	err    chan cloudError
	open   chan struct{}
}

func (c *ctxReceiver) OnIntent(r *cloud.IntentResult) {
	c.intent <- r
}

func (c *ctxReceiver) OnError(kind cloud.ErrorType, err error) {
	c.err <- cloudError{kind, err}
}

func (c *ctxReceiver) OnStreamOpen() {
	c.open <- struct{}{}
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

	cloudChans := &ctxReceiver{
		intent: make(chan *cloud.IntentResult),
		err:    make(chan cloudError),
		open:   make(chan struct{}),
	}

	var ctx *stream.Streamer
procloop:
	for {
		// the cases in this select should NOT block! if messages that others send us
		// are not promptly read, socket buffers can fill up and break voice processing
		select {
		case msg := <-p.msg:
			switch msg.msg.Tag() {
			case cloud.MessageTag_Hotword:
				// hotword = get ready to stream data
				if ctx != nil {
					log.Println("Got hotword event while already streaming, weird...")
					if err := ctx.Close(); err != nil {
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
					p.writeError(cloud.ErrorType_InvalidConfig, fmt.Errorf("unknown mode %d", mode))
					continue
				}

				language, err := getLanguage(msg.msg.GetHotword().Locale)
				if err != nil {
					p.writeError(cloud.ErrorType_InvalidConfig, err)
					continue
				}

				var ctxopts []stream.Option
				ctxopts = append(ctxopts, stream.WithTokener(p.getToken, p.opts.requireToken),
					stream.WithChipperURL(ChipperURL),
					stream.WithChipperSecret(ChipperSecret))

				streamOpts := chipper.StreamOpts{
					CompressOpts: chipper.CompressOpts{
						Compress:   p.opts.compress,
						Bitrate:    66 * 1024,
						Complexity: 0,
						FrameSize:  60},
					SaveAudio: p.opts.saveAudio,
					Timeout:   DefaultTimeout,
					Language:  language,
				}
				if mode == cloud.StreamType_KnowledgeGraph {
					ctxopts = append(ctxopts, stream.WithKnowledgeGraphOptions(streamOpts))
				} else {
					ctxopts = append(ctxopts, stream.WithIntentOptions(chipper.IntentOpts{
						StreamOpts: streamOpts,
						Handler:    p.opts.handler,
						Mode:       serverMode,
					}, mode))
				}
				logVerbose("Got hotword event", serverMode)
				ctx = stream.NewStreamer(cloudChans, p.StreamSize(), ctxopts...)

			case cloud.MessageTag_DebugFile:
				p.writeResponse(msg.msg)

			case cloud.MessageTag_Audio:
				// add samples to our buffer
				buf := msg.msg.GetAudio().Data
				if ctx != nil {
					ctx.AddSamples(buf)
				} else {
					logVerbose("No active context, discarding", len(buf), "samples")
				}
			}

		case intent := <-cloudChans.intent:
			logVerbose("Received intent from cloud:", intent)
			// we got an answer from the cloud, tell mic to stop...
			p.signalMicStop()

			// send intent to AI
			p.writeResponse(cloud.NewMessageWithResult(intent))

			// stop streaming until we get another hotword event
			if err := ctx.Close(); err != nil {
				log.Println("Error closing context:")
			}
			ctx = nil

		case err := <-cloudChans.err:
			logVerbose("Received error from cloud:", err)
			p.signalMicStop()
			p.writeError(err.kind, err.err)
			if err := ctx.Close(); err != nil {
				log.Println("Error closing context:")
			}
			ctx = nil

		case <-cloudChans.open:
			p.writeResponse(cloud.NewMessageWithStreamOpen(&cloud.Void{}))

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
	stream.SetVerbose(value)
}

func (p *Process) getToken() (string, error) {
	req := cloud.NewTokenRequestWithJwt(&cloud.JwtRequest{})
	resp, err := token.HandleRequest(req)
	if err != nil {
		log.Println("Error getting jwt token:", err)
		return "", err
	}
	jwt := resp.GetJwt()
	if jwt.Error != cloud.TokenError_NoError {
		log.Println("Error code getting jwt token:", jwt.Error)
		return "", fmt.Errorf("jwt error code %d", jwt.Error)
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

func getLanguage(locale string) (pb.LanguageCode, error) {
	// split on _ and -
	strs := strings.Split(locale, "-")
	if len(strs) != 2 {
		strs = strings.Split(locale, "_")
	}
	if len(strs) != 2 {
		return 0, fmt.Errorf("invalid locale string %s", locale)
	}

	lang := strings.ToLower(strs[0])
	country := strings.ToLower(strs[1])

	switch lang {
	case "fr":
		return pb.LanguageCode_FRENCH, nil
	case "de":
		return pb.LanguageCode_GERMAN, nil
	case "en":
		break
	default:
		// unknown == default to en_US
		return pb.LanguageCode_ENGLISH_US, nil
	}

	switch country {
	case "gb": // ISO2 code for UK is 'GB'
		return pb.LanguageCode_ENGLISH_UK, nil
	case "au":
		return pb.LanguageCode_ENGLISH_AU, nil
	}
	return pb.LanguageCode_ENGLISH_US, nil
}

var modeMap = map[cloud.StreamType]pb.RobotMode{
	cloud.StreamType_Normal:    pb.RobotMode_VOICE_COMMAND,
	cloud.StreamType_Blackjack: pb.RobotMode_GAME,
}

type cloudError struct {
	kind cloud.ErrorType
	err  error
}
