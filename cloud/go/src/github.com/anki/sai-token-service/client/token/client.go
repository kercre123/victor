package token

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcclient"
	"github.com/anki/sai-token-service/proto/tokenpb"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

// Client extends the gRPC interface with methods needed by services
// in order to properly use the Token Service for authentication.
//
// Client implements the interceptors.ServerInterceptorFactory
// interface for application services to insert into the interceptor
// chain to provide TMS-based authentication.
//
// It also implements the svc.CommandInitializer and svc.ServiceObject
// interfaces for easily hooking into the service lifecycle.
type Client interface {
	tokenpb.TokenClient
	TokenValidator
}

type client struct {
	tokenpb.TokenClient
	*PublicKeyTokenValidator

	// conn is the gRPC client connection create by cfg.Dial() during
	// Start(). The client object caches it in order to close it in
	// Stop().
	conn *grpc.ClientConn

	// cfg is a standard gRPC client config object, which configures
	// the connection parameters - address, port, mutual TLS, etc...
	cfg *grpcclient.ClientConfig
}

type ClientOpt func(*client)

func NewClient(opts ...ClientOpt) Client {
	c := &client{
		PublicKeyTokenValidator: NewPublicKeyTokenValidator("token-service-signing"),
		cfg: grpcclient.NewClientConfig("token"),
	}
	for _, opt := range opts {
		opt(c)
	}
	return c
}

func (c *client) CommandSpec() string {
	return c.cfg.CommandSpec() + " " +
		c.PublicKeyTokenValidator.CommandSpec()
}

func (c *client) CommandInitialize(cmd *cli.Cmd) {
	c.cfg.CommandInitialize(cmd)
	c.PublicKeyTokenValidator.CommandInitialize(cmd)
}

func (c *client) Start(ctx context.Context) error {
	if err := c.PublicKeyTokenValidator.Start(ctx); err != nil {
		return err
	}

	if c.conn == nil {
		conn, err := c.cfg.Dial()
		if err != nil {
			return err
		}
		c.conn = conn
	}

	if c.TokenClient == nil {
		c.TokenClient = tokenpb.NewTokenClient(c.conn)
	}

	alog.Info{
		"action": "tms_client_start",
		"status": "connected",
	}.Log()

	return nil
}

func (c *client) Stop() error {
	if c.conn != nil {
		c.conn.Close()
		c.conn = nil
	}
	return nil
}

func (c *client) Kill() error {
	return c.Stop()
}
