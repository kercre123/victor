package interceptors

import (
	"context"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
)

// AuthHandler is a function type providing the logic for
// authenticating gRPC requests via an auth interceptor. The handler
// function is responsible for returning the correct errors codes in
// the event of unauthenticated or unauthorized access.
//
// See google.golang.org/grpc/status for status.Errorf() and
// google.golang.org/grpc/codes for status codes. Auth handlers should
// return nil for success, and either codes.Unauthenticated or
// codes.PermissionDenied for failure.
type ServerAuthenticator func(context.Context, *grpcutil.MethodCallDescriptor) error

// ClientAuthenticator is a function type that decorates a client side
// context with credentials to be passed to the server.
type ClientAuthenticator func(context.Context) context.Context

type AuthFactory struct {
	serverAuth ServerAuthenticator
	clientAuth ClientAuthenticator
}

type AuthFactoryOption func(f *AuthFactory)

func WithServerAuthenticator(handler ServerAuthenticator) AuthFactoryOption {
	return func(f *AuthFactory) {
		f.serverAuth = handler
	}
}

func WithClientAuthenticator(handler ClientAuthenticator) AuthFactoryOption {
	return func(f *AuthFactory) {
		f.clientAuth = handler
	}
}

func NewAuthFactory(opts ...AuthFactoryOption) *AuthFactory {
	f := &AuthFactory{}
	for _, opt := range opts {
		opt(f)
	}
	return f
}

func (f *AuthFactory) clientContext(ctx context.Context) context.Context {
	if f.clientAuth != nil {
		return f.clientAuth(ctx)
	}
	return ctx
}

func (f *AuthFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		if err := f.serverAuth(ctx, descriptor); err != nil {
			return nil, err
		}
		return handler(ctx, req)
	}
}

func (f *AuthFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		descriptor := grpcutil.MustGetMethodCallDescriptor(stream.Context())
		if err := f.serverAuth(stream.Context(), descriptor); err != nil {
			return err
		}
		return handler(srv, stream)
	}
}

func (f *AuthFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		return invoker(f.clientContext(ctx), method, req, reply, cc, opts...)
	}
}

func (f *AuthFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		return streamer(f.clientContext(ctx), desc, cc, method, opts...)
	}
}
