package grpcsvc

import (
	"github.com/anki/sai-service-framework/grpc/grpcclient"
	"github.com/anki/sai-service-framework/grpc/interceptors"
	"github.com/grpc-ecosystem/go-grpc-middleware"
	"github.com/grpc-ecosystem/go-grpc-middleware/recovery"
	"google.golang.org/grpc"
)

// ServerInterceptorFactory defines an interface for types which can
// produce server side interceptor functions.
type ServerInterceptorFactory interface {
	UnaryServerInterceptor() grpc.UnaryServerInterceptor
	StreamServerInterceptor() grpc.StreamServerInterceptor
}

// InterceptorFactory defines the interface for types that can produce
// both server and client side interceptor functions.
type InterceptorFactory interface {
	ServerInterceptorFactory
	grpcclient.ClientInterceptorFactory
}

// These funcs are just pass-throughs for the go-grpc-middleware
// package, just so service implementers have fewer packages to
// import directly.

// ChainUnaryServer takes any number of UnaryServerInterceptor
// functions and returns a composite interceptor which executes them
// in left to right order.
func ChainUnaryServer(interceptors ...grpc.UnaryServerInterceptor) grpc.UnaryServerInterceptor {
	return grpc_middleware.ChainUnaryServer(interceptors...)
}

// ChainStreamServer takes any number of StreamServerInterceptor
// functions and returns a composite interceptor which executes them
// in left to right order.
func ChainStreamServer(interceptors ...grpc.StreamServerInterceptor) grpc.StreamServerInterceptor {
	return grpc_middleware.ChainStreamServer(interceptors...)
}

// WithUnaryServerChain returns a grpc.ServerOption which sets the
// unary server interceptor on a gRPC service to be the left to right
// chain of any number of interceptor functions.
func WithUnaryServerChain(interceptors ...grpc.UnaryServerInterceptor) grpc.ServerOption {
	return grpc_middleware.WithUnaryServerChain(interceptors...)
}

// WithStreamServerChain returns a grpc.ServerOption which sets the
// stream server interceptor on a gRPC service to be the left to right
// chain of any number of interceptor functions.
func WithStreamServerChain(interceptors ...grpc.StreamServerInterceptor) grpc.ServerOption {
	return grpc_middleware.WithStreamServerChain(interceptors...)
}

var standardInterceptors = []InterceptorFactory{
	interceptors.NewTraceIDFactory(),
	interceptors.NewRequestIDFactory(),
	interceptors.NewMethodDecoratorFactory(),
	interceptors.NewMetricsFactory(),
	interceptors.NewAccessLogFactory(),
}

// WithStandardUnaryServerChain returns a grpc.ServerOption to
// configure a gRPC server with the standard unary interceptor chain plus
// optional additional interceptors. Additional interceptors will be
// executed after the standard interceptor set.
func WithStandardUnaryServerChain(interceptors ...grpc.UnaryServerInterceptor) grpc.ServerOption {
	var i []grpc.UnaryServerInterceptor
	for _, f := range standardInterceptors {
		i = append(i, f.UnaryServerInterceptor())
	}
	i = append(i, interceptors...)
	i = append(i, grpc_recovery.UnaryServerInterceptor())
	return WithUnaryServerChain(i...)
}

// WithStandardStreamServerChain returns a grpc.ServerOption to
// configure a gRPC server with the standard stream interceptor chain
// plus optional additional interceptors. Additional interceptors will
// be executed after the standard interceptor set.
func WithStandardStreamServerChain(interceptors ...grpc.StreamServerInterceptor) grpc.ServerOption {
	var i []grpc.StreamServerInterceptor
	for _, f := range standardInterceptors {
		i = append(i, f.StreamServerInterceptor())
	}
	i = append(i, interceptors...)
	i = append(i, grpc_recovery.StreamServerInterceptor())
	return WithStreamServerChain(i...)
}
