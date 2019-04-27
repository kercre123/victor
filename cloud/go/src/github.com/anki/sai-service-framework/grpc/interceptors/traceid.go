package interceptors

import (
	"context"

	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
)

// TraceIDFactory returns interceptors which ensure that every
// request has a unique trace ID which is passed through all
// invocation layers and on to other gRPC service invocations.
type TraceIDFactory struct{}

func NewTraceIDFactory() *TraceIDFactory {
	return &TraceIDFactory{}
}

func (f *TraceIDFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		return handler(grpcutil.WithTraceId(ctx), req)
	}
}

func (f *TraceIDFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		return handler(srv, &grpcutil.ServerStreamWithContext{
			ServerStream: stream,
			Ctx:          grpcutil.WithTraceId(stream.Context()),
		})
	}
}

func (f *TraceIDFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		return invoker(grpcutil.WithTraceId(ctx), method, req, reply, cc, opts...)
	}
}

func (f *TraceIDFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		return streamer(grpcutil.WithTraceId(ctx), desc, cc, method, opts...)
	}
}
