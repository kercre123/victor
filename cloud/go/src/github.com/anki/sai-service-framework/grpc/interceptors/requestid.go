package interceptors

import (
	"context"

	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
)

// RequestIDFactory returns interceptors which ensure that every
// request has a unique request ID which is passed through all
// invocation layers and on to other gRPC service invocations.
type RequestIDFactory struct{}

func NewRequestIDFactory() *RequestIDFactory {
	return &RequestIDFactory{}
}

func (f *RequestIDFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		return handler(grpcutil.WithRequestId(ctx), req)
	}
}

func (f *RequestIDFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		return handler(srv, &grpcutil.ServerStreamWithContext{
			ServerStream: stream,
			Ctx:          grpcutil.WithRequestId(stream.Context()),
		})
	}
}

func (f *RequestIDFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		return invoker(grpcutil.WithRequestId(ctx), method, req, reply, cc, opts...)
	}
}

func (f *RequestIDFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		return streamer(grpcutil.WithRequestId(ctx), desc, cc, method, opts...)
	}
}
