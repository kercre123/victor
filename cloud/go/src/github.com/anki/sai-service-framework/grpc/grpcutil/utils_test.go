package grpcutil

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc/metadata"
)

func TestSplitMethod(t *testing.T) {
	p, s, m := SplitMethod("/package.Service/Method")
	assert.Equal(t, "package", p)
	assert.Equal(t, "Service", s)
	assert.Equal(t, "Method", m)

	fm := JoinMethod(p, s, m)
	assert.Equal(t, "/package.Service/Method", fm)
}

func TestGetStringValueWithFallback(t *testing.T) {
	// Immediate
	ctx := context.WithValue(context.Background(), 1, "value1")
	assert.Equal(t, "value1", GetStringValueWithFallbackToMetadata(ctx, 1, "metadata-key"))

	// Incoming metadata
	ctx = metadata.NewIncomingContext(context.Background(),
		metadata.New(map[string]string{"metadata-key": "value2"}))
	assert.Equal(t, "value2", GetStringValueWithFallbackToMetadata(ctx, 1, "metadata-key"))

	// Get immediate value instead of metadata (values would normally
	// match - for the test case they don't so we can verify which
	// path was taken)
	ctx = metadata.NewIncomingContext(context.WithValue(context.Background(), 1, "immediate"),
		metadata.New(map[string]string{"metadata-key": "meta"}))
	assert.Equal(t, "immediate", GetStringValueWithFallbackToMetadata(ctx, 1, "metadata-key"))

	// Outgoing metadata
	ctx = metadata.NewOutgoingContext(context.Background(),
		metadata.New(map[string]string{"metadata-key": "value3"}))
	assert.Equal(t, "value3", GetStringValueWithFallbackToMetadata(ctx, 1, "metadata-key"))

	// Nonexistent key
	assert.Empty(t, GetStringValueWithFallbackToMetadata(context.Background(), 1, "metadata-key"))
}
