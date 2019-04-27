package grpcutil

import (
	"context"
	"crypto/x509"

	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/peer"
)

type certificateContextKey int

const (
	certKey certificateContextKey = iota
)

func WithPeerCertificate(ctx context.Context) context.Context {
	if cert := GetPeerCertificate(ctx); cert != nil {
		return BindPeerCertificate(ctx, cert)
	}
	return ctx
}

func BindPeerCertificate(ctx context.Context, cert *x509.Certificate) context.Context {
	return context.WithValue(ctx, certKey, cert)
}

func GetPeerCertificate(ctx context.Context) *x509.Certificate {
	if cert, ok := ctx.Value(certKey).(*x509.Certificate); ok {
		return cert
	} else {
		if peer, ok := peer.FromContext(ctx); ok {
			if tlsInfo, ok := peer.AuthInfo.(credentials.TLSInfo); ok {
				if len(tlsInfo.State.PeerCertificates) > 0 {
					return tlsInfo.State.PeerCertificates[0]
				}
			}
		}
	}
	return nil
}
