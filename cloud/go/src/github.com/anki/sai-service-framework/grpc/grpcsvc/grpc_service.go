package grpcsvc

import (
	"context"
	"errors"
	"fmt"
	"net"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/auth"
	"github.com/anki/sai-service-framework/grpc/interceptors"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
)

const (
	DefaultMaxConnectionAge         = 10 * time.Minute
	DefaultMaxConnectionGracePeriod = 30 * time.Second
	DefaultMaxConnectionIdle        = 2 * time.Minute
	DefaultConnectionTimeout        = 1 * time.Minute
)

// GrpcServiceComponent is a service object component which launches a
// gRPC server listening on a network port. It supports options for
// specifying the listening port & address, x509 certificate & key for
// running with TLS, and a callback function to hook up the gRPC
// service to handle request.
type GrpcServiceComponent struct {
	// port specifies the network port for the gRPC service to listen
	// on
	port *int

	// addr specifies the network address for the gRPC service to
	// listen on
	addr *string

	// tlsConfig encapsulates all the configuration parameters related
	// to TLS, including the server certificate and the pool of CA
	// certificates for validating client certificates for mTLS
	// authentication.
	tlsConfig *svc.ServerTLSConfig

	// registerFn is a callback function that registers the actual
	// gRPC service implementation with the server for it.
	registerFn RegisterServerFn

	// maxConnectionAge defines the maximum amount of time a
	// connection is allowed to be open, in absolute terms, regardless
	// of activity.
	maxConnectionAge *svc.Duration

	// maxConnectionGracePeriod defines the amount of time a
	// connection is given to close gracefully after it has reached
	// maxConnectionAge before being forcibly closed.
	maxConnectionGracePeriod *svc.Duration

	// maxConnectionIdle defines the amount of time that a connection
	// is allowed to be idle before being closed.
	maxConnectionIdle *svc.Duration

	// connectionTimeout defines the maximum amount of time to allow
	// for TLS connection establishment, from initial packet to
	// completion of the handshake.
	connectionTimeout *svc.Duration

	// maxConcurrentStreams defines the maximum number of streaming
	// gRPC calls that are allowed to be active at one time. It does
	// NOT affect the number of RPC streams that can be *initiated*,
	// only those that are allowed to make progress. An unlimited
	// number of stream requests can be issued -- any above the
	// concurrent limit will block until other requests complete.
	maxConcurrentStreams *int

	// serverOpts are the options passed to grpc.NewServer() to create
	// the server handler. Add server options with
	// WithServerOptions(). If TLS options are set it will contain the
	// right ServerOption objects to set TLS.
	serverOpts []grpc.ServerOption

	// interceptors is a list of implementations of the
	// interceptors.ServerInterceptorFactory interface, for creating
	// both streaming and unary interceptors to insert into the
	// interceptor chain.
	interceptors []ServerInterceptorFactory

	// server is a pointer to the running gRPC server, after Start()
	// is called
	server *grpc.Server

	// listener is the underlying TCP listener that responds to
	// incoming connections
	listener net.Listener

	// ready is a channel used to indicate to interested parties (such
	// as test suites) when the server is ready, as the Start() method
	// blocks.
	ready chan bool

	// logGrpc controls whether we redirect the internal gRPC log
	// system to the anki log or not.
	logGrpc *bool
}

type Option func(c *GrpcServiceComponent)

// RegisterServerFn is the signature of functions that will register a
// gRPC service implementation with the grpc.Server object that
// handles the TCP communication.
type RegisterServerFn func(*grpc.Server)

// WithRegisterFn sets the callback function used to associate the
// gRPC service implementation with the gRPC server object that
// handles network communication. The provided function should call
// the generated function pbpackage.RegisterXXXServer(). This
// approach is necessary because limitations in how the generated code
// provides the registration function.
//
// Example:
//   WithRegisterFn(func(gs *grpc.Server) {
//  	 echopb.RegisterEchoServer(gs, <service-implementation>)
//   })
func WithRegisterFn(fn RegisterServerFn) Option {
	return func(m *GrpcServiceComponent) {
		m.registerFn = fn
	}
}

// WithInterceptor adds an interceptor factory to the list of
// interceptors used to construct the unary and streaming interceptor
// chains at gRPC server start time.
func WithInterceptor(f ServerInterceptorFactory) Option {
	return func(c *GrpcServiceComponent) {
		c.interceptors = append(c.interceptors, f)
	}
}

// WithServerOptions adds one or more grpc.ServerOption configurators
// to the list of options to be applied to the gRPC Service when it's
// launched.
//
// Note that callers should NOT add grpc.UnaryServerInterceptor or
// grpc.StreamServerInterceptor options here, as they will override
// interceptors specified by the WithInterceptor() option and the
// default interceptors.
func WithServerOptions(opts ...grpc.ServerOption) Option {
	return func(c *GrpcServiceComponent) {
		for _, opt := range opts {
			c.serverOpts = append(c.serverOpts, opt)
		}
	}
}

var defaultOptions = []Option{
	WithInterceptor(interceptors.NewTraceIDFactory()),
	WithInterceptor(interceptors.NewRequestIDFactory()),
	WithInterceptor(interceptors.NewMethodDecoratorFactory()),
	WithInterceptor(interceptors.NewMetricsFactory()),
	WithInterceptor(&interceptors.PeerCertificateFactory{}),
	WithInterceptor(auth.NewAccountsAuthenticator()),
	WithInterceptor(auth.NewTokenAuthenticator(auth.WithTokenClient())),
	WithInterceptor(interceptors.NewAccessLogFactory()),
	WithInterceptor(&interceptors.RecoveryFactory{}),
}

func New(opts ...Option) *GrpcServiceComponent {
	c := &GrpcServiceComponent{
		tlsConfig: svc.NewServerTLSConfig("grpc"),
		ready:     make(chan bool, 1),
	}
	for _, opt := range defaultOptions {
		opt(c)
	}
	for _, opt := range opts {
		opt(c)
	}
	return c
}

func NewWithoutDefaults(opts ...Option) *GrpcServiceComponent {
	c := &GrpcServiceComponent{
		tlsConfig: svc.NewServerTLSConfig("grpc"),
		ready:     make(chan bool, 1),
	}
	for _, opt := range opts {
		opt(c)
	}
	return c
}

func (c *GrpcServiceComponent) CommandSpec() string {
	spec := "" +
		// Listening address & port
		"[--grpc-port] [--grpc-addr] " +
		"[--grpc-log] " +
		// TLS
		c.tlsConfig.CommandSpec() + " " +
		// Timeouts
		"[--grpc-max-connection-age] [--grpc-max-connection-grace-period] " +
		"[--grpc-max-connection-idle] [--grpc-connection-timeout] " +
		"[--grpc-max-streams]"
	for _, interceptor := range c.interceptors {
		if ci, ok := interceptor.(svc.CommandInitializer); ok {
			spec = spec + " " + ci.CommandSpec()
		}
	}
	return spec
}

func (c *GrpcServiceComponent) CommandInitialize(cmd *cli.Cmd) {
	if c.port == nil {
		c.port = svc.IntOpt(cmd, "grpc-port", 0,
			"Network port number to listen for gRPC requests on")
	}
	if c.addr == nil {
		c.addr = svc.StringOpt(cmd, "grpc-addr", "",
			"Network address to listen for gRPC requests on. Defaults to 0.0.0.0")
	}
	c.tlsConfig.CommandInitialize(cmd)
	if c.maxConnectionAge == nil {
		c.maxConnectionAge = svc.DurationOpt(cmd, "grpc-max-connection-age",
			DefaultMaxConnectionAge,
			"Maximum amount of time any connection is allowed to live, regardless of activity")
	}
	if c.maxConnectionGracePeriod == nil {
		c.maxConnectionGracePeriod = svc.DurationOpt(cmd, "grpc-max-connection-grace-period",
			DefaultMaxConnectionGracePeriod,
			"Maximum amount of time to allow an aged connection to shutdown before force closing")
	}
	if c.maxConnectionIdle == nil {
		c.maxConnectionIdle = svc.DurationOpt(cmd, "grpc-max-connection-idle",
			DefaultMaxConnectionIdle,
			"Maximum amount of time a connection can be idle before being sent a GoAway")
	}
	if c.connectionTimeout == nil {
		c.connectionTimeout = svc.DurationOpt(cmd, "grpc-connection-timeout",
			DefaultConnectionTimeout,
			"Maximum amount of time to allow for TLS connection establishment")
	}
	if c.maxConcurrentStreams == nil {
		c.maxConcurrentStreams = svc.IntOpt(cmd, "grpc-max-streams",
			0,
			"Maximum number of concurrent streams to allow (0 = unlimited)")
	}
	if c.logGrpc == nil {
		c.logGrpc = svc.BoolOpt(cmd, "grpc-log", true,
			"Output grpc internal logs to the Anki log")
	}
	for _, interceptor := range c.interceptors {
		if ci, ok := interceptor.(svc.CommandInitializer); ok {
			ci.CommandInitialize(cmd)
		}
	}
}

func (c *GrpcServiceComponent) Start(ctx context.Context) error {
	if c.server != nil {
		return errors.New("Server is already running")
	}
	if err := c.validate(); err != nil {
		return err
	}

	if c.logGrpc != nil && *c.logGrpc {
		LogGrpcToAnkiLog()
	}

	// Construct the interceptor chains from the configured
	// interceptor factories.
	var unaryInterceptors []grpc.UnaryServerInterceptor
	var streamInterceptors []grpc.StreamServerInterceptor
	for _, f := range c.interceptors {
		unaryInterceptors = append(unaryInterceptors, f.UnaryServerInterceptor())
		streamInterceptors = append(streamInterceptors, f.StreamServerInterceptor())
	}

	// Add the interceptor chains to the list of server options
	opts := []grpc.ServerOption{}
	opts = append(opts,
		WithUnaryServerChain(unaryInterceptors...),
		WithStreamServerChain(streamInterceptors...),
	)

	// Start the interceptors
	for _, interceptor := range c.interceptors {
		if so, ok := interceptor.(svc.ServiceObject); ok {
			if err := so.Start(ctx); err != nil {
				return err
			}
		}
	}

	// Add additional server options handled by the framework
	opts = append(opts,
		grpc.ConnectionTimeout(c.connectionTimeout.Duration()),
		grpc.KeepaliveParams(keepalive.ServerParameters{
			MaxConnectionIdle:     c.maxConnectionIdle.Duration(),
			MaxConnectionAge:      c.maxConnectionAge.Duration(),
			MaxConnectionAgeGrace: c.maxConnectionGracePeriod.Duration(),
		}),
	)

	if *c.maxConcurrentStreams > 0 {
		opts = append(opts, grpc.MaxConcurrentStreams(uint32(*c.maxConcurrentStreams)))
	}

	if len(c.serverOpts) > 0 {
		opts = append(opts, c.serverOpts...)
	}

	addr := fmt.Sprintf("%s:%d", *c.addr, *c.port)
	if listener, err := net.Listen("tcp", addr); err != nil {
		return err
	} else {
		alog.Info{
			"action": "grpc_start",
			"status": "listening",
			"port":   *c.port,
			"addr":   *c.addr,
		}.Log()
		c.listener = listener
		c.server = grpc.NewServer(opts...)
		c.registerFn(c.server)
		c.ready <- true
		// Serve() blocks until the gRPC service is shut down via
		// Stop() or Kill()
		return c.server.Serve(c.listener)
	}
}

// Addr returns the address the gRPC server is listening on. This is
// provided primarily for unit testing, for when the listening port is
// randomly assigned.
func (c *GrpcServiceComponent) Addr() net.Addr {
	return c.listener.Addr()
}

// Server returns a pointer to the underlying gRPC server object. Use
// with caution.
func (c *GrpcServiceComponent) Server() *grpc.Server {
	return c.server
}

// Ready returns a channel indicating when the service is ready for
// incoming connections.
func (c *GrpcServiceComponent) Ready() <-chan bool {
	return c.ready
}

// Stop initiates a graceful stop on the gRPC service, which will wait
// for all client connections to complete before shutting down. It is
// vulnerable to hanging if there are poorly behave clients which
// refuse to disconnect.
func (c *GrpcServiceComponent) Stop() error {
	if c.server != nil {
		// Stop the interceptors
		for _, interceptor := range c.interceptors {
			if so, ok := interceptor.(svc.ServiceObject); ok {
				if err := so.Stop(); err != nil {
					alog.Error{
						"action": "grpc_service_stop",
						"status": "interceptor_stop_error",
						"err":    err,
					}.Log()
				}
			}
		}
		c.server.GracefulStop()
	}
	return nil
}

// Kill will cancel any pending graceful stop and trigger an immediate
// shutdown of the gRPC service, forcibly closing any open client
// connections, which will return to the client with an error.
func (c *GrpcServiceComponent) Kill() error {
	if c.server != nil {
		c.server.Stop()
	}
	return nil
}

// validate ensures that the gRPC configuration is correct, and loads
// the certificate/key pair if it is.
func (c *GrpcServiceComponent) validate() error {
	if c.registerFn == nil {
		return errors.New("gRPC Service register function not provided")
	}

	if *c.tlsConfig.Enabled {
		if cfg, err := c.tlsConfig.Load(); err != nil {
			return err
		} else {
			c.serverOpts = append(c.serverOpts, grpc.Creds(credentials.NewTLS(cfg)))
		}
	} else {
		// Allow running without TLS for local testing, but
		// unconditionally log a warning, so we can alert on TLS
		// misconfigurations in deployed environments.
		alog.Warn{
			"action": "grpc_service_validate",
			"msg":    "TLS is disabled",
		}.Log()
	}

	return nil
}
