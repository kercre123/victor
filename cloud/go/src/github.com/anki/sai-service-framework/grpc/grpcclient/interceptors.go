package grpcclient

import (
	"github.com/anki/sai-service-framework/grpc/interceptors"
	"github.com/grpc-ecosystem/go-grpc-middleware"
	"google.golang.org/grpc"
)

// ClientInterceptorFactory defines an interface for types which can
// produce client side interceptor functions.
type ClientInterceptorFactory interface {
	UnaryClientInterceptor() grpc.UnaryClientInterceptor
	StreamClientInterceptor() grpc.StreamClientInterceptor
}

// ChainUnaryClient takes any number of UnaryClientInterceptor
// functions and returns a composite interceptor which executes them
// in left to right order.
func ChainUnaryClient(interceptors ...grpc.UnaryClientInterceptor) grpc.UnaryClientInterceptor {
	return grpc_middleware.ChainUnaryClient(interceptors...)
}

// ChainStreamClient takes any number of StreamClientInterceptor
// functions and returns a composite interceptor which executes them
// in left to right order.
func ChainStreamClient(interceptors ...grpc.StreamClientInterceptor) grpc.StreamClientInterceptor {
	return grpc_middleware.ChainStreamClient(interceptors...)
}

// WithUnaryClientChain returns a grpc.ServerOption which sets the
// unary client interceptor on a gRPC service to be the left to right
// chain of any number of interceptor functions.
func WithUnaryClientChain(interceptors ...grpc.UnaryClientInterceptor) grpc.DialOption {
	return grpc.WithUnaryInterceptor(grpc_middleware.ChainUnaryClient(interceptors...))
}

// WithStreamClientChain returns a grpc.ServerOption which sets the
// stream client interceptor on a gRPC service to be the left to right
// chain of any number of interceptor functions.
func WithStreamClientChain(interceptors ...grpc.StreamClientInterceptor) grpc.DialOption {
	return grpc.WithStreamInterceptor(grpc_middleware.ChainStreamClient(interceptors...))
}

var standardInterceptors = []ClientInterceptorFactory{
	interceptors.NewTraceIDFactory(),
	interceptors.NewRequestIDFactory(),
	interceptors.NewMethodDecoratorFactory(),
	interceptors.NewMetricsFactory(),
	interceptors.NewAccessLogFactory(),
}

// WithStandardUnaryClientChain returns a grpc.DialOption to configure
// a gRPC client connection with the standard unary interceptor chain
// plus optional additional interceptors. Additional interceptors will
// be executed after the standard interceptor set.
func WithStandardUnaryClientChain(interceptors ...grpc.UnaryClientInterceptor) grpc.DialOption {
	var i []grpc.UnaryClientInterceptor
	for _, f := range standardInterceptors {
		i = append(i, f.UnaryClientInterceptor())
	}
	i = append(i, interceptors...)
	return WithUnaryClientChain(i...)
}

// WithStandardStreamClientChain returns a grpc.DialOption to
// configure a gRPC client connection with the standard stream
// interceptor chain plus optional additional interceptors. Additional
// interceptors will be executed after the standard interceptor set.
func WithStandardStreamClientChain(interceptors ...grpc.StreamClientInterceptor) grpc.DialOption {
	var i []grpc.StreamClientInterceptor
	for _, f := range standardInterceptors {
		i = append(i, f.StreamClientInterceptor())
	}
	i = append(i, interceptors...)
	return WithStreamClientChain(i...)
}
