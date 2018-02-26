package chipper

import (
	"context"
	"fmt"

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

type StreamOpts struct {
	SessionId string
}

type Stream struct {
	conn    *Conn
	stream  pb.ChipperGrpc_StreamingIntentClient
	session string
	opts    *StreamOpts
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

	return &Stream{
		conn:   c,
		stream: stream,
		opts:   &opts}, nil
}

func (c *Conn) Close() error {
	return c.conn.Close()
}

func (c *Stream) SendAudio(audioData []byte) error {
	return c.stream.Send(
		&pb.StreamingIntentRequest{
			DeviceId:        c.conn.device,
			Session:         c.opts.SessionId,
			LanguageCode:    pb.LanguageCode_ENGLISH_US,
			SingleUtterance: true,
			IntentService:   pb.IntentService_DIALOGFLOW,
			AppKey:          c.conn.appKey,
			InputAudio:      audioData})
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
