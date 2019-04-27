package grpctest

import (
	"context"
	"fmt"
	"testing"

	"github.com/anki/sai-service-framework/grpc/grpcsvc"
	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
)

const repeatCount = 3

//
// gRPC service implementation
//

type EchoServerImpl struct{}

func (s *EchoServerImpl) Echo(ctx context.Context, in *EchoData) (*EchoData, error) {
	return in, nil
}

func (s *EchoServerImpl) Repeat(ctx context.Context, in *EchoData) (*RepeatedData, error) {
	var responseData []*EchoData
	for i := 0; i < repeatCount; i++ {
		responseData = append(responseData, in)
	}
	return &RepeatedData{
		Data: responseData,
	}, nil
}

func (s *EchoServerImpl) StreamingEcho(stream Echo_StreamingEchoServer) error {
	for {
		in, err := stream.Recv()
		if err != nil {
			return err
		}
		if err := stream.Send(in); err != nil {
			return err
		}
	}
}

//
// Test suite
//

var (
	register = grpcsvc.WithRegisterFn(func(gs *grpc.Server) {
		RegisterEchoServer(gs, &EchoServerImpl{})
	})
	ctx = context.Background()
)

type GrpcServiceComponentTestSuite struct {
	Suite
}

func TestGrpcServiceComponentTestSuite(t *testing.T) {
	suite.Run(t, new(GrpcServiceComponentTestSuite))
}

func (s *GrpcServiceComponentTestSuite) Test_BasicStartupAndRequestHandling() {
	s.Setup(register)
	client := NewEchoClient(s.ClientConn)

	data := &EchoData{S: s.T().Name()}
	rsp, err := client.Echo(context.Background(), data)
	require.NoError(s.T(), err)
	assert.True(s.T(), proto.Equal(data, rsp))

	stream, err := client.StreamingEcho(context.Background())
	require.NoError(s.T(), err)
	assert.NotNil(s.T(), stream)

	count := 10
	for i := 0; i < count; i++ {
		data := &EchoData{S: fmt.Sprintf("%s-stream-%d", s.T().Name(), i)}
		require.NoError(s.T(), stream.Send(data))
		echo, err := stream.Recv()
		require.NoError(s.T(), err)
		assert.True(s.T(), proto.Equal(data, echo))
	}
	require.NoError(s.T(), stream.CloseSend())
}

/*
func (s *GrpcServiceComponentTestSuite) Test_MaxConcurrentStreams() {
	s.Setup(register, grpcsvc.WithMaxConcurrentStreams(1))
	client := NewEchoClient(s.ClientConn)

	_, err := client.StreamingEcho(ctx)
	assert.Nil(s.T(), err)
	fmt.Println("Open streaming call 1")

	_, err = client.StreamingEcho(ctx)
	assert.Nil(s.T(), err)
	fmt.Println("Open streaming call 2")
}
*/

func (s *GrpcServiceComponentTestSuite) Test_MaxConnectionAge() {
	// TODO: do some magic w/go routines
}
