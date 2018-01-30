package chipper

import (
	"context"
	"fmt"

	"anki/util"

	pb "github.com/anki/sai-chipper-voice/grpc/pb2"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

var (
	defaultTLSCert = credentials.NewClientTLSFromCert(nil, "")
)

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

type Client struct {
	client  pb.ChipperGrpcClient
	stream  pb.ChipperGrpc_StreamingIntentClient
	conn    *grpc.ClientConn
	appKey  string
	session string
	device  string
}

func NewClient(serverUrl string, appKey string, deviceId string, sessionId string) (*Client, error) {
	dialOpts := []grpc.DialOption{}
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(defaultTLSCert))
	if appKey != "" {
		dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(&rpcAppKey{appKey}))
	}
	conn, err := grpc.Dial(serverUrl, dialOpts...)
	if err != nil {
		return nil, err
	}

	rpcClient := pb.NewChipperGrpcClient(conn)
	stream, err := rpcClient.StreamingIntent(context.Background())
	if err != nil {
		fmt.Println("GRPC Stream creation error:", err)
		return nil, err
	}

	client := &Client{
		client:  rpcClient,
		stream:  stream,
		conn:    conn,
		appKey:  appKey,
		session: sessionId,
		device:  deviceId}
	return client, nil
}

func (c *Client) SendAudio(audioData []byte) error {
	return c.stream.Send(
		&pb.StreamingIntentRequest{
			DeviceId:        c.device,
			Session:         c.session,
			LanguageCode:    pb.LanguageCode_ENGLISH_US,
			SingleUtterance: true,
			IntentService:   pb.IntentService_DIALOGFLOW,
			AppKey:          c.appKey,
			InputAudio:      audioData})
}

func (c *Client) WaitForIntent() (*pb.IntentResult, error) {
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

func (c *Client) Close() error {
	err := util.Errors{}
	err.Append(c.stream.CloseSend())
	err.Append(c.conn.Close())
	return err.Error()
}
