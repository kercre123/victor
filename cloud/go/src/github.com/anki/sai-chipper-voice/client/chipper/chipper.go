package chipper

import (
	"context"
	"fmt"
	"time"

	"github.com/anki/opus-go/opus"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

var (
	defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

type (
	// IntentResult aliases the protobuf IntentResult type
	IntentResult = pb.IntentResult
	// IntentResponse aliases the protobuf IntentResponse type
	IntentResponse = pb.IntentResponse
	// KnowledgeGraphResponse aliases the protobuf KnowledgeGraphResponse type
	KnowledgeGraphResponse = pb.KnowledgeGraphResponse
	// ConnectionCheckResponse aliases the protobuf ConnectionCheckResponse type
	ConnectionCheckResponse = pb.ConnectionCheckResponse
)

type rpcMetadata struct {
	opts *connOpts
}

func (r *rpcMetadata) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	ret := make(map[string]string)
	addFrom := func(m map[string]string) {
		for k, v := range m {
			ret[k] = v
		}
	}
	if r.opts.creds != nil {
		subdata, err := r.opts.creds.GetRequestMetadata(ctx, uri...)
		if err != nil {
			return nil, err
		}
		addFrom(subdata)
	}
	addFrom(map[string]string{
		"device-id":  r.opts.deviceID,
		"session-id": r.opts.sessionID,
	})
	return ret, nil
}

func (r *rpcMetadata) RequireTransportSecurity() bool {
	return true
}

// Conn represents an underlying GRPC connection to the Chipper server
type Conn struct {
	conn   *grpc.ClientConn
	client pb.ChipperGrpcClient
	opts   *connOpts
}

// Stream represents an open stream of various possible types (intent, knowledge graph) to Chipper
type Stream interface {
	SendAudio(audioData []byte) error
	WaitForResponse() (interface{}, error)
	Close() error
	CloseSend() error
}

type baseStream struct {
	conn        *Conn
	opts        *StreamOpts
	encoder     *opus.OggStream
	client      grpc.ClientStream
	hasStreamed bool
	cancel      func()
}

type intentStream struct {
	baseStream
	opts *IntentOpts
}

type kgStream struct {
	baseStream
	opts *KGOpts
}

type connectionStream struct {
	baseStream
	opts *ConnectOpts
}

// StreamOpts defines options common to all possible stream types
type StreamOpts struct {
	CompressOpts
	SaveAudio bool
	Timeout   time.Duration
	Language  pb.LanguageCode
	NoDas     bool
}

// IntentOpts extends StreamOpts with options unique to intent streams
type IntentOpts struct {
	StreamOpts
	Handler    pb.IntentService
	Mode       pb.RobotMode
	SpeechOnly bool
}

// ConnectOpts extends StreamOpts with options unique to connection check streams
type ConnectOpts struct {
	StreamOpts
	TotalAudioMs      uint32
	AudioPerRequestMs uint32
}

// KGOpts extends StreamOpts with options unique to knowledge graph streams
type KGOpts struct {
	StreamOpts
	Timezone string
}

// CompressOpts specifies whether compression should be used and, if so, allows
// the specification of parameters related to it
type CompressOpts struct {
	Compress   bool
	Bitrate    uint
	Complexity uint
	FrameSize  float32
	PreEncoded bool
}

// NewConn creates a new connection to the given GRPC server
func NewConn(ctx context.Context, serverURL string, options ...ConnOpt) (*Conn, error) {
	// create default options and apply user provided overrides
	opts := connOpts{deviceID: "device-id"}
	for _, opt := range options {
		opt(&opts)
	}

	dialOpts := opts.grpcOpts
	if opts.insecure {
		dialOpts = append(dialOpts, grpc.WithInsecure())
	} else {
		dialOpts = append(dialOpts, grpc.WithTransportCredentials(defaultTLSCert))
	}
	dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(&rpcMetadata{&opts}))

	rpcConn, err := grpc.DialContext(ctx, serverURL, dialOpts...)
	if err != nil {
		return nil, err
	}

	rpcClient := pb.NewChipperGrpcClient(rpcConn)

	conn := &Conn{
		conn:   rpcConn,
		client: rpcClient,
		opts:   &opts}
	return conn, nil
}

func (c *Conn) newStream(opts *StreamOpts, client grpc.ClientStream, cancel func()) baseStream {
	var encoder *opus.OggStream
	if opts.Compress {
		encoder = &opus.OggStream{
			SampleRate: 16000,
			Channels:   1,
			FrameSize:  opts.FrameSize,
			Bitrate:    opts.Bitrate,
			Complexity: opts.Complexity}
	}

	return baseStream{
		conn:    c,
		client:  client,
		opts:    opts,
		encoder: encoder,
		cancel:  cancel,
	}
}

// NewIntentStream opens a new stream on the given connection to stream audio for the purpose of getting
// an intent response
func (c *Conn) NewIntentStream(ctx context.Context, opts IntentOpts) (Stream, error) {
	ctx, cancel := getContext(ctx, &opts.StreamOpts)
	client, err := c.client.StreamingIntent(ctx)
	if err != nil {
		fmt.Println("GRPC Stream creation error:", err)
		return nil, err
	}
	return &intentStream{
		baseStream: c.newStream(&opts.StreamOpts, client, cancel),
		opts:       &opts,
	}, nil
}

// NewKGStream opens a new stream on the given connection to stream audio for the purpose of getting
// a knowledge graph response
func (c *Conn) NewKGStream(ctx context.Context, opts KGOpts) (Stream, error) {
	ctx, cancel := getContext(ctx, &opts.StreamOpts)
	client, err := c.client.StreamingKnowledgeGraph(ctx)
	if err != nil {
		fmt.Println("GRPC Stream creation error:", err)
		return nil, err
	}
	return &kgStream{
		baseStream: c.newStream(&opts.StreamOpts, client, cancel),
		opts:       &opts,
	}, nil
}

// NewConnectionStream opens a stream for doing connection checks
func (c *Conn) NewConnectionStream(ctx context.Context, opts ConnectOpts) (Stream, error) {
	ctx, cancel := getContext(ctx, &opts.StreamOpts)
	client, err := c.client.StreamingConnectionCheck(ctx)
	if err != nil {
		fmt.Println("GRPC Stream creation error:", err)
		return nil, err
	}
	return &connectionStream{
		baseStream: c.newStream(&opts.StreamOpts, client, cancel),
		opts:       &opts,
	}, nil

}

// Close closes the underlying connection
func (c *Conn) Close() error {
	return c.conn.Close()
}

func (c *intentStream) SendAudio(audioData []byte) error {
	return c.sendAudio(audioData, c.createMessage)
}

func (c *kgStream) SendAudio(audioData []byte) error {
	return c.sendAudio(audioData, c.createMessage)
}

func (c *connectionStream) SendAudio(audioData []byte) error {
	return c.sendAudio(audioData, c.createMessage)
}

func (c *baseStream) sendAudio(audioData []byte, messageFunc func([]byte, pb.AudioEncoding) interface{}) error {
	encoding := pb.AudioEncoding_LINEAR_PCM

	if c.opts.Compress {
		encoding = pb.AudioEncoding_OGG_OPUS
		var err error
		audioData, err = c.encoder.EncodeBytes(audioData)
		if err != nil {
			return err
		}
		flushData := c.encoder.Flush()
		audioData = append(audioData, flushData...)
	} else if c.opts.PreEncoded {
		encoding = pb.AudioEncoding_OGG_OPUS
	}

	toSend := messageFunc(audioData, encoding)
	c.hasStreamed = true
	return c.client.SendMsg(toSend)
}

func (c *intentStream) createMessage(audioData []byte, encoding pb.AudioEncoding) interface{} {
	ret := new(pb.StreamingIntentRequest)
	if !c.hasStreamed {
		*ret = pb.StreamingIntentRequest{
			DeviceId:        c.conn.opts.deviceID,
			Session:         c.conn.opts.sessionID,
			LanguageCode:    c.opts.Language,
			SingleUtterance: false,
			IntentService:   c.opts.Handler,
			AudioEncoding:   encoding,
			SpeechOnly:      c.opts.SpeechOnly,
			Mode:            c.opts.Mode,
			SaveAudio:       c.opts.SaveAudio,
			FirmwareVersion: c.conn.opts.firmware,
			BootId:          c.conn.opts.bootID,
			SkipDas:         c.opts.NoDas,
		}
	}
	ret.InputAudio = audioData
	return ret
}

func (c *kgStream) createMessage(audioData []byte, encoding pb.AudioEncoding) interface{} {
	ret := new(pb.StreamingKnowledgeGraphRequest)
	if !c.hasStreamed {
		*ret = pb.StreamingKnowledgeGraphRequest{
			DeviceId:        c.conn.opts.deviceID,
			Session:         c.conn.opts.sessionID,
			LanguageCode:    c.opts.Language,
			AudioEncoding:   encoding,
			SaveAudio:       c.opts.SaveAudio,
			FirmwareVersion: c.conn.opts.firmware,
			BootId:          c.conn.opts.bootID,
			SkipDas:         c.opts.NoDas,
			Timezone:        c.opts.Timezone,
		}
	}
	ret.InputAudio = audioData
	return ret
}

func (c *connectionStream) createMessage(audioData []byte, encoding pb.AudioEncoding) interface{} {
	ret := new(pb.StreamingConnectionCheckRequest)
	if !c.hasStreamed {
		*ret = pb.StreamingConnectionCheckRequest{
			DeviceId:        c.conn.opts.deviceID,
			Session:         c.conn.opts.sessionID,
			TotalAudioMs:    c.opts.TotalAudioMs,
			AudioPerRequest: c.opts.AudioPerRequestMs,
			FirmwareVersion: c.conn.opts.firmware,
		}
	}
	ret.InputAudio = audioData
	return ret
}

func (c *intentStream) WaitForResponse() (interface{}, error) {
	for {
		intent := new(IntentResponse)
		err := c.client.RecvMsg(intent)
		if err != nil {
			return nil, err
		} else if !intent.GetIsFinal() {
			// ignore non-final responses
			continue
		}
		return intent.GetIntentResult(), nil
	}
}

func (c *kgStream) WaitForResponse() (interface{}, error) {
	response := new(KnowledgeGraphResponse)
	err := c.client.RecvMsg(response)
	if err != nil {
		return nil, err
	}
	return response, nil
}

func (c *connectionStream) WaitForResponse() (interface{}, error) {
	response := new(ConnectionCheckResponse)
	err := c.client.RecvMsg(response)
	if err != nil {
		return nil, err
	}
	return response, nil
}

func (c *baseStream) Close() error {
	if c.cancel != nil {
		c.cancel()
	}
	return c.client.CloseSend()
}

func (c *baseStream) CloseSend() error {
	return c.client.CloseSend()
}

// SendText performs an intent request with a text string instead of voice data
func (c *Conn) SendText(ctx context.Context, text, session, device string,
	service pb.IntentService) *pb.IntentResult {

	r := &pb.TextRequest{
		TextInput:       text,
		DeviceId:        device,
		Session:         session,
		LanguageCode:    pb.LanguageCode_ENGLISH_US,
		IntentService:   service,
		FirmwareVersion: c.opts.firmware,
	}

	res, err := c.client.TextIntent(ctx, r)
	if err != nil {
		fmt.Println("Text intent error:", err)
		return nil
	}
	fmt.Printf("Text_intent=\"%s\"  query=\"%s\", param=\"%v\"",
		res.IntentResult.Action,
		res.IntentResult.QueryText,
		res.IntentResult.Parameters)
	return res.IntentResult
}

func getContext(ctx context.Context, opts *StreamOpts) (context.Context, func()) {
	if opts.Timeout > 0 {
		return context.WithDeadline(ctx, time.Now().Add(opts.Timeout))
	}
	return ctx, nil
}
