package auth

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// GrpcAuthorizer enforces authorization specification to allow or
// disallow gRPC calls to proceed based on supplied user credentials
// bound to the current context.
//
// User credentials are extracted from the the request and bound to
// the current context by one of the authentication components,
// allowing the authorizer to operate consistently regardless of
// whether the call was authenticated using a JWT token or a session
// token.
type GrpcAuthorizer struct {
	specs       map[string]GrpcAuthorizationSpec
	defaultSpec *GrpcAuthorizationSpec
	enabled     *bool
}

type GrpcAuthorizerOption func(a *GrpcAuthorizer)

// WithAuthorizationSpecs sets the per-method authorization specs to
// enforce.
func WithAuthorizationSpecs(specs []GrpcAuthorizationSpec) GrpcAuthorizerOption {
	return func(a *GrpcAuthorizer) {
		for _, spec := range specs {
			if _, ok := a.specs[spec.Method]; ok {
				panic("Programming error - multiple authorization specs provided for method " + spec.Method)
			}
			a.specs[spec.Method] = spec
		}
	}
}

// WithDefaultAuthorizationSpec sets an authorization spec to use as a
// fallback if a per-method spec has not been specified.
func WithDefaultAuthorizationSpec(spec GrpcAuthorizationSpec) GrpcAuthorizerOption {
	return func(a *GrpcAuthorizer) {
		a.defaultSpec = &spec
	}
}

// NewGrpcAuthorizer instantiates and initialized a new GrpcAuthorizer
// with the provided options.
func NewGrpcAuthorizer(opts ...GrpcAuthorizerOption) *GrpcAuthorizer {
	auth := &GrpcAuthorizer{
		specs: make(map[string]GrpcAuthorizationSpec),
	}
	for _, opt := range opts {
		opt(auth)
	}
	return auth
}

func (a *GrpcAuthorizer) CommandSpec() string {
	return "[--grpc-authorization-enabled]"
}

func (a *GrpcAuthorizer) CommandInitialize(cmd *cli.Cmd) {
	// TODO: change this default to true when we're ready to enable
	// authorization everywhere
	a.enabled = svc.BoolOpt(cmd, "grpc-authorization-enabled", false,
		"Enable access controls on gRPC calls")
}

// UnaryServerInterceptor returns an interceptor function for gRPC to
// use for unary RPC invocations.
func (a *GrpcAuthorizer) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if err := a.authorize(ctx); err != nil {
			return nil, err
		}
		return handler(ctx, req)
	}
}

// StreamServerInterceptor returns an interceptor function for gRPC to
// use for streaming RPC invocations.
func (a *GrpcAuthorizer) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if err := a.authorize(stream.Context()); err != nil {
			return err
		}
		return handler(srv, stream)
	}
}

func (a *GrpcAuthorizer) specForMethod(method string) *GrpcAuthorizationSpec {
	if spec, ok := a.specs[method]; ok {
		return &spec
	}
	return a.defaultSpec
}

func (a *GrpcAuthorizer) authorize(ctx context.Context) error {
	if !*a.enabled {
		return nil
	}

	// Get the method identification from the context. This is set by
	// the MethodDecorator interceptor.
	method := grpcutil.MustGetMethodCallDescriptor(ctx)

	// Find the authorization spec for the method. For now, every
	// method must have either an explicit spec or a default.
	spec := a.specForMethod(method.FullMethod)
	if spec == nil {
		alog.Error{
			"action": "grpc_authorize",
			"status": "missing_authorization_spec",
			"method": method.FullMethod,
		}.Log()
		return status.Errorf(codes.PermissionDenied, "Missing authorization specification")
	}

	return spec.Authorize(ctx)
}
