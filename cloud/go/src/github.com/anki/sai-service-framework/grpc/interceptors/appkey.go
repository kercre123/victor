package interceptors

import (
	"context"
	"google.golang.org/grpc"

	"github.com/anki/sai-service-framework/grpc/grpcutil"
)

func AppKeyUnaryClientInterceptor(appKey string) grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		return invoker(grpcutil.WithAppKey(ctx, appKey), method, req, reply, cc, opts...)
	}
}

func AppKeyStreamClientInterceptor(appKey string) grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		return streamer(grpcutil.WithAppKey(ctx, appKey), desc, cc, method, opts...)
	}
}
