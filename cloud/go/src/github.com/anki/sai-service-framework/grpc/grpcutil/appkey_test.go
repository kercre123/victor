package grpcutil

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc/metadata"
)

func TestAppKey(t *testing.T) {
	assert.Empty(t, GetAppKey(context.Background()))
	assert.Equal(t, "appkey", GetAppKey(WithAppKey(context.Background(), "appkey")))

	ctx := metadata.NewIncomingContext(context.Background(),
		metadata.MD{
			"anki-app-key": []string{"appkey2"},
		})
	assert.Equal(t, "appkey2", GetAppKey(ctx))

	ctx = context.WithValue(context.Background(), appKeyKey, "appkey3")
	assert.Equal(t, "appkey3", GetAppKey(ctx))
}

func TestUserSessionKey(t *testing.T) {
	assert.Empty(t, GetUserSessionKey(context.Background()))
	assert.Equal(t, "session", GetUserSessionKey(WithUserSessionKey(context.Background(), "session")))

	ctx := metadata.NewIncomingContext(context.Background(),
		metadata.MD{
			"anki-user-session": []string{"session2"},
		})
	assert.Equal(t, "session2", GetUserSessionKey(ctx))

	ctx = context.WithValue(context.Background(), userSessionKey, "session3")
	assert.Equal(t, "session3", GetUserSessionKey(ctx))
}

func TestSanitizeAppKey(t *testing.T) {
	assert.Equal(t, "33d...", SanitizeAppKey("33dad9d1b44a0368"))
	assert.Equal(t, "1", SanitizeAppKey("1"))
	assert.Equal(t, "12", SanitizeAppKey("12"))
	assert.Equal(t, "123", SanitizeAppKey("123"))
	assert.Equal(t, "123...", SanitizeAppKey("1234"))
}
