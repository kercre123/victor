package token

import "anki/token/jwt"

type options struct {
	server           bool
	identityProvider *jwt.IdentityProvider
	socketNameSuffix string
}

// Option defines an option that can be set on the token server
type Option func(o *options)

// WithServer specifies that an IPC server should be started so other processes
// can request tokens from this process
func WithServer() Option {
	return func(o *options) {
		o.server = true
	}
}

// WithIdentityProvider specifies a non-default identity provider
func WithIdentityProvider(identityProvider *jwt.IdentityProvider) Option {
	return func(o *options) {
		o.identityProvider = identityProvider
	}
}

// WithSocketNameSuffix specifies the (optional) suffix of the socket name
func WithSocketNameSuffix(socketNameSuffix string) Option {
	return func(o *options) {
		o.socketNameSuffix = socketNameSuffix
	}
}
