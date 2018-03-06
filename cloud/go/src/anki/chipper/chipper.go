package chipper

import (
	"context"
	"fmt"

	"anki/opus"

	pb "github.com/anki/sai-chipper-voice/grpc/pb2"

	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

var (
	defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

type IntentResult = pb.IntentResult

type rpcAppKey struct {
	Value string
}

func (k *rpcAppKey) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return map[string]string{
		"trusted_app_key": k.Value,
	}, nil
}

func (k *rpcAppKey) RequireTransportSecurity() bool {
	return true
}

type Conn struct {
	conn   *grpc.ClientConn
	client pb.ChipperGrpcClient
	appKey string
	device string
}

type Stream struct {
	conn    *Conn
	stream  pb.ChipperGrpc_StreamingIntentClient
	session string
	opts    *StreamOpts
	encoder *opus.OggStream
}

type StreamOpts struct {
	CompressOpts
	SessionId string
	Handler   pb.IntentService
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

func NewConn(serverUrl string, appKey string, deviceId string) (*Conn, error) {
	dialOpts := []grpc.DialOption{}
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(defaultTLSCert))
	if appKey != "" {
		dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(&rpcAppKey{appKey}))
	}
	rpcConn, err := grpc.Dial(serverUrl, dialOpts...)
	if err != nil {
		return nil, err
	}

	rpcClient := pb.NewChipperGrpcClient(rpcConn)

	conn := &Conn{
		conn:   rpcConn,
		client: rpcClient,
		appKey: appKey,
		device: deviceId}
	return conn, nil
}

func (c *Conn) NewStream(opts StreamOpts) (*Stream, error) {
	stream, err := c.client.StreamingIntent(context.Background())
	if err != nil {
		fmt.Println("GRPC Stream creation error:", err)
		return nil, err
	}

	var encoder *opus.OggStream
	if opts.Compress {
		encoder = &opus.OggStream{
			SampleRate: 16000,
			Channels:   1,
			FrameSize:  opts.FrameSize,
			Bitrate:    opts.Bitrate,
			Complexity: opts.Complexity}
	}

	return &Stream{
		conn:    c,
		stream:  stream,
		opts:    &opts,
		encoder: encoder}, nil
}

func (c *Conn) Close() error {
	return c.conn.Close()
}

func (c *Stream) SendAudio(audioData []byte) error {
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

	return c.stream.Send(
		&pb.StreamingIntentRequest{
			DeviceId:        c.conn.device,
			Session:         c.opts.SessionId,
			LanguageCode:    pb.LanguageCode_ENGLISH_US,
			SingleUtterance: true,
			IntentService:   c.opts.Handler,
			AppKey:          c.conn.appKey,
			InputAudio:      audioData,
			AudioEncoding:   encoding})
}

func (c *Stream) WaitForIntent() (*IntentResult, error) {
	for {
		intent, err := c.stream.Recv()
		if err != nil {
			return nil, err
		} else if !intent.GetIsFinal() {
			// ignore non-final responses
			continue
		}
		return intent.GetIntentResult(), nil
	}
}

func (c *Stream) Close() error {
	return c.stream.CloseSend()
}
