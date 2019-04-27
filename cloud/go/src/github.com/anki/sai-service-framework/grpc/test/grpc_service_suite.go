package grpctest

import (
	"context"
	"net"
	"time"

	"github.com/anki/sai-service-framework/grpc/grpcsvc"
	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
)

type Suite struct {
	suite.Suite

	RegisterFn grpcsvc.RegisterServerFn
	Service    *grpcsvc.GrpcServiceComponent
	ClientConn *grpc.ClientConn
	Addr       net.Addr
}

func (s *Suite) SetupTest() {
	if s.RegisterFn != nil {
		s.Setup()
	}
}

func (s *Suite) Setup(opts ...grpcsvc.Option) {
	var options []grpcsvc.Option
	options = append(options, opts...)
	if s.RegisterFn != nil {
		options = append(options, grpcsvc.WithRegisterFn(s.RegisterFn))
	}
	options = append(options, []grpcsvc.Option{
		// Listen only on the loopback, not the external IP address
		grpcsvc.WithAddr("127.0.0.1"),
		// Get assigned a random port
		grpcsvc.WithPort(0),
		// No TLS
		grpcsvc.WithTLSEnabled(false),
	}...)
	s.Service = grpcsvc.NewWithoutDefaults(options...)

	// This will not actually be used, but ensures that the service
	// object's internal parameter state is non-nil.
	cmd := cli.App("test", "Test stuff")
	s.Service.CommandInitialize(cmd.Cmd)

	go func() {
		if err := s.Service.Start(context.Background()); err != nil {
			s.T().Fatalf("gRPC service failed to start: %s", err)
		}
	}()
	// Wait until the gRPC service has been created.
	select {
	case <-s.Service.Ready():
		break
	}

	if len(s.Service.Server().GetServiceInfo()) == 0 {
		s.T().Fatal("No gRPC services registered")
	}

	if conn, err := grpc.Dial(s.Service.Addr().String(), grpc.WithInsecure()); err != nil {
		s.T().Fatalf("gRPC client dial failed: %s", err)
	} else {
		s.ClientConn = conn
	}
}

func (s *Suite) TearDownTest() {
	if s.ClientConn != nil {
		if err := s.ClientConn.Close(); err != nil {
			s.T().Fatalf("gRPC client connection failed to close: %s", err)
		}
		s.ClientConn = nil
	}

	if s.Service != nil {
		stop := make(chan interface{})
		go func() {
			if err := s.Service.Stop(); err != nil {
				s.T().Fatalf("gRPC Service error on stop: %s", err)
			}
			close(stop)
		}()

		select {
		case <-stop:
			return
		case <-time.After(30 * time.Second):
			s.T().Log("gRPC Service failed to stop in time; killing...")
			if err := s.Service.Kill(); err != nil {
				s.T().Fatalf("gRPC Service error on kill: %s", err)
			}
		}
		s.Service = nil
	}
}
