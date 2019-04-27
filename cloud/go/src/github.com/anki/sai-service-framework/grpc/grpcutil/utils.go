package grpcutil

import (
	"context"
	"fmt"
	"path"
	"strings"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"
)

// StreamWithContext wraps an existing gRPC stream in order to
// override the context.
type StreamWithContext struct {
	grpc.Stream
	ctx context.Context
}

func (s *StreamWithContext) Context() context.Context {
	return s.ctx
}

// ServerStreamWithContext wraps an existing RPC server stream in
// order to override the context.
type ServerStreamWithContext struct {
	grpc.ServerStream
	Ctx context.Context
}

func (s *ServerStreamWithContext) Context() context.Context {
	return s.Ctx
}

// SplitMethod splits the full method name of a gRPC method into its
// component package, service, and method names. e.g. an input of
// "/package.Service/Method" returns "package", "Service", "Method".
func SplitMethod(fullMethod string) (string, string, string) {
	fullService := strings.Split(path.Dir(fullMethod)[1:], ".")
	method := path.Base(fullMethod)
	return fullService[0], fullService[1], method
}

// JoinMethod takes a package name, service name, and method name as
// input and produces a gRPC "full method name" as output, in the form
// "/package.service/method".
func JoinMethod(pkg, service, method string) string {
	return fmt.Sprintf("/%s.%s/%s", pkg, service, method)
}

// GetStringValueWithFallbackToMetadata retrieves a string value from
// the context, first checking for an immediate bound value, falling
// back to the incoming metadata first, then the outgoing metadata.
func GetStringValueWithFallbackToMetadata(ctx context.Context, ctxKey interface{}, metadataKey string) string {

	// If the value is bound as an immediate value on the context,
	// return that
	if value, ok := ctx.Value(ctxKey).(string); ok {
		return value
	} else {

		// Otherwise, check the incoming metadata
		if md, ok := metadata.FromIncomingContext(ctx); ok {
			if incomingId, ok := md[metadataKey]; ok {
				if len(incomingId) > 0 {
					id := incomingId[0]
					alog.Debug{
						"action": "get_context_value",
						"status": "found_incoming_metadata",
						"key":    metadataKey,
						"value":  id,
					}.Log()
					return id
				}
			}
		}

		// For client logs, check the outgoing metadata if we haven't
		// found one in the incoming metadata
		if md, ok := metadata.FromOutgoingContext(ctx); ok {
			if outgoingId, ok := md[metadataKey]; ok {
				if len(outgoingId) > 0 {
					id := outgoingId[0]
					alog.Debug{
						"action": "get_context_value",
						"status": "found_outgoing_metadata",
						"key":    metadataKey,
						"value":  id,
					}.Log()
					return id
				}
			}
		}
	}

	return ""
}

// WithBuildInfoUserAgent returns a grpc DialOption to pass to
// grpc.Dial() or ClientConfig.Dial() which sets the User-Agent based
// on github.com/anki/sai-go-util/buildinfo
func WithBuildInfoUserAgent() grpc.DialOption {
	info := buildinfo.Info()
	hash := info.Commit
	if len(info.Commit) > 7 {
		hash = info.Commit[:7]
	}
	ua := fmt.Sprintf("%s/%s sai-service-framework/1.0", info.Product, hash)
	return grpc.WithUserAgent(ua)
}
