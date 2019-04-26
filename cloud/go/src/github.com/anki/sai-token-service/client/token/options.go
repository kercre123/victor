package token

import (
	"github.com/anki/sai-token-service/proto/tokenpb"
	"google.golang.org/grpc"
)

// WithClientConn takes a gRPC client connection and configures the
// Token Service client with a gRPC client instantiated to use that
// connection.
//
// WithClientConn and WithClient should not be used together, as they
// will override each other.
func WithClientConn(conn *grpc.ClientConn) ClientOpt {
	return func(c *client) {
		c.TokenClient = tokenpb.NewTokenClient(conn)
	}
}

// WithClient configures the Token Service client with an
// implementation of the gRPC TokenClient interface from the generated
// protobuf code. Primarily provided for testing.
//
// WithClient and WithClientConn should not be used together, as they
// will override each other.
func WithClient(cli tokenpb.TokenClient) ClientOpt {
	return func(c *client) {
		c.TokenClient = cli
	}
}

// WithVerificationKey configures the Token Service client with a key
// for verifying the signature of tokens decoded by TokenFromString().
func WithVerificationKey(key interface{}) ClientOpt {
	return func(c *client) {
		c.key = key
	}
}
