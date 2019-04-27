package grpcutil

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/metadata"
)

func TestTraceId(t *testing.T) {
	assert.Empty(t, GetTraceId(context.Background()))
	assert.NotEmpty(t, GetTraceId(WithTraceId(context.Background())))

	ctx := metadata.NewIncomingContext(context.Background(),
		metadata.MD{
			"anki-trace-id": []string{"trace1"},
		})
	assert.Equal(t, "trace1", GetTraceId(WithTraceId(ctx)))

	ctx = WithTraceId(ctx)
	md, ok := metadata.FromOutgoingContext(ctx)
	require.True(t, ok)
	tid, ok := md["anki-trace-id"]
	require.True(t, ok)
	assert.Equal(t, "trace1", tid[0])
}
