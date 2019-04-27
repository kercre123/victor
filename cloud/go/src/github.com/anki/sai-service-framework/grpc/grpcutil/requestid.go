package grpcutil

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
)

type requestIdContextKey int

const (
	requestIdKey requestIdContextKey = iota
)

// GetRequestId extracts the request ID from a context object, if one
// has been set. Returns the empty string if no request ID is present
// on the context.
func GetRequestId(ctx context.Context) string {
	if rid, ok := ctx.Value(requestIdKey).(string); ok {
		return rid
	}
	return ""
}

// WithRequestId returns a new context with a request ID bound as an
// immediate context value.
//
// If the input context already has a request ID bound as an immediate
// value, that will be used. If no request ID exists, one will be
// generated.
func WithRequestId(ctx context.Context) context.Context {
	id := GetRequestId(ctx)

	if id != "" {
		alog.Debug{
			"action": "with_request_id",
			"status": "found_request_id",
			"id":     id,
		}.Log()
		return ctx
	} else {
		id, _ = strutil.ShortLowerUUID()
		alog.Debug{
			"action": "with_request_id",
			"status": "generating_request_id",
			"id":     id,
		}.Log()
	}

	ctx = alog.WithLogMap(ctx, map[string]interface{}{
		"request_id": id,
	})
	return context.WithValue(ctx, requestIdKey, id)
}
