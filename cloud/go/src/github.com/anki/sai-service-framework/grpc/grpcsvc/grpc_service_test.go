package grpcsvc

import (
	"context"
	"testing"
	"time"

	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
)

//
// gRPC service implementation
//

type EchoServerImpl struct{}

func (s *EchoServerImpl) Echo(ctx context.Context, in *EchoData) (*EchoData, error) {
	return in, nil
}

//
// Test suite
//

type GrpcServiceTestSuite struct {
	suite.Suite
}

func TestGrpcServiceTestSuite(t *testing.T) {
	suite.Run(t, new(GrpcServiceTestSuite))
}

func (s *GrpcServiceTestSuite) TestBasicStartupShutdown() {
	t := s.T()

	// Set up a gRPC service
	svc := New(WithAddr("127.0.0.1"),
		WithPort(0),
		WithTLSEnabled(false),
		WithRegisterFn(func(gs *grpc.Server) {
			RegisterEchoServer(gs, &EchoServerImpl{})
		}))

	// Dummy command necessary for all internal parameters to be
	// initialized
	cmd := cli.App("test", "Test stuff")
	svc.CommandInitialize(cmd.Cmd)

	// Start the service and wait for it to be ready
	go func() {
		if err := svc.Start(context.Background()); err != nil {
			t.Fatalf("gRPC service failed to start: %s", err)
		}
	}()
	select {
	case <-svc.Ready():
		break
	case <-time.After(time.Second * 30):
		t.Fatalf("gRPC service ready timed out")
	}

	// Verify that the server exists, has an address, and the gRPC
	// implementation is registered
	require.NotNil(t, svc.Server())
	require.NotNil(t, svc.Addr())
	require.Len(t, svc.Server().GetServiceInfo(), 1)

	// Verify that calling Start() a second time will error
	require.Error(t, svc.Start(context.Background()))

	// Shut it down
	assert.NoError(t, svc.Stop())
}

type testInterceptor struct {
	commandSpecCount       int
	commandInitializeCount int
	unaryInterceptorCount  int
	streamInterceptorCount int
}

func (f *testInterceptor) CommandSpec() string {
	f.commandSpecCount++
	return ""
}

func (f *testInterceptor) CommandInitialize(cmd *cli.Cmd) {
	f.commandInitializeCount++
}

func (f *testInterceptor) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	f.unaryInterceptorCount++
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		return handler(ctx, req)
	}
}

func (f *testInterceptor) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		return handler(srv, stream)
	}
}

func (s *GrpcServiceTestSuite) TestInterceptorCommandInitialize() {
	t := s.T()
	inter := &testInterceptor{}
	svc := New(WithAddr("127.0.0.1"),
		WithPort(0),
		WithTLSEnabled(false),
		WithInterceptor(inter))

	assert.Equal(t, 0, inter.commandSpecCount)
	svc.CommandSpec()
	assert.Equal(t, 1, inter.commandSpecCount)

	assert.Equal(t, 0, inter.commandInitializeCount)
	cmd := cli.App("test", "Test stuff")
	svc.CommandInitialize(cmd.Cmd)
	assert.Equal(t, 1, inter.commandInitializeCount)
}
