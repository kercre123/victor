package interceptors

import (
	"context"
	"fmt"
	"time"

	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
)

// MethodDecoratorFactory returns client & server interceptors which
// decodes method invocation information and add it to the context, to
// be used by downstream interceptors. The method decorator should be
// the first interceptor in the chain, and is configured as such as
// part of the standard interceptor chains. Other standard
// interceptors such as the metrics and access log interceptors will
// panic if the method decorator has not run.
type MethodDecoratorFactory struct{}

func NewMethodDecoratorFactory() *MethodDecoratorFactory {
	return &MethodDecoratorFactory{}
}

func (f *MethodDecoratorFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		descriptor := newDescriptor(ctx, info.FullMethod, info, false)
		return handler(grpcutil.WithMethodCallDescriptor(ctx, descriptor), req)
	}
}

func (f *MethodDecoratorFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		descriptor := newDescriptor(stream.Context(), info.FullMethod, info, true)
		wrapper := &grpcutil.ServerStreamWithContext{stream, grpcutil.WithMethodCallDescriptor(stream.Context(), descriptor)}
		return handler(srv, wrapper)
	}
}

func (f *MethodDecoratorFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		descriptor := newDescriptor(ctx, method, nil, false)
		return invoker(grpcutil.WithMethodCallDescriptor(ctx, descriptor), method, req, reply, cc, opts...)
	}
}

func (f *MethodDecoratorFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		descriptor := newDescriptor(ctx, method, nil, true)
		return streamer(grpcutil.WithMethodCallDescriptor(ctx, descriptor), desc, cc, method, opts...)
	}
}

func newDescriptor(ctx context.Context, fullMethod string, info interface{}, streaming bool) *grpcutil.MethodCallDescriptor {
	pkg, service, method := grpcutil.SplitMethod(fullMethod)
	return &grpcutil.MethodCallDescriptor{
		StartedAt:   time.Now(),
		FullMethod:  fullMethod,
		MetricName:  fmt.Sprintf("%s.%s.%s", pkg, service, method),
		Package:     pkg,
		Service:     service,
		Method:      method,
		ServerInfo:  info,
		IsStreaming: streaming,
	}
}
