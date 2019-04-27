package interceptors

import (
	"context"

	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
)

// PeerCertificateFactory returns server interceptors which will
// extract the client certificate used in mutual TLS, if present, and
// attach its parsed form as an x509.Certificate to the context.
type PeerCertificateFactory struct{}

func (f *PeerCertificateFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		return handler(grpcutil.WithPeerCertificate(ctx), req)
	}
}

func (f *PeerCertificateFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		return handler(srv, &grpcutil.ServerStreamWithContext{
			ServerStream: stream,
			Ctx:          grpcutil.WithPeerCertificate(stream.Context()),
		})
	}
}
