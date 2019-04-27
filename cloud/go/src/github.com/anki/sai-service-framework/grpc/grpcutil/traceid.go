package grpcutil

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
	"google.golang.org/grpc/metadata"
)

type traceIdContextKey int

const (
	traceIdKey         traceIdContextKey = iota
	TraceIdMetadataKey                   = "anki-trace-id"
)

// GetTraceId extracts the trace ID from a context object, if one
// has been set. Returns the empty string if no trace ID is present
// on the context.
func GetTraceId(ctx context.Context) string {
	if rid, ok := ctx.Value(traceIdKey).(string); ok {
		return rid
	}
	return ""
}

// WithTraceId returns a new context with a trace ID bound as an
// immediate context value, and on the outgoing metadata for
// forwarding in calls to other gRPC services.
//
// If the input context already has a trace ID bound as an immediate
// value, that will be used first. If there is no immediate trace ID
// and the context contains a trace ID in the incoming metadata,
// that will be used. If no trace ID exists, one will be generated.
func WithTraceId(ctx context.Context) context.Context {
	id := GetTraceId(ctx)

	if id == "" {
		if md, ok := metadata.FromIncomingContext(ctx); ok {
			if incomingId, ok := md[TraceIdMetadataKey]; ok {
				id = incomingId[0]
				alog.Debug{
					"action": "with_trace_id",
					"status": "found_metadata_trace_id",
					"id":     id,
				}.Log()
			}
		}
	} else {
		alog.Debug{
			"action": "with_trace_id",
			"status": "found_trace_id",
			"id":     id,
		}.Log()
	}

	if id == "" {
		id, _ = strutil.ShortLowerUUID()
		alog.Debug{
			"action": "with_trace_id",
			"status": "generating_trace_id",
			"id":     id,
		}.Log()
	}

	return context.WithValue(
		metadata.AppendToOutgoingContext(ctx, TraceIdMetadataKey, id),
		traceIdKey, id)
}
