package model

import (
	"testing"
	"time"

	"github.com/dgrijalva/jwt-go"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestToken_IsExpiredLogic(t *testing.T) {
	assert.False(t, Token{
		IssuedAt:  time.Now().UTC(),
		ExpiresAt: time.Now().UTC().Add(time.Hour * 24),
	}.IsExpired())
	assert.True(t, Token{
		IssuedAt:  time.Now().UTC().Add(time.Hour * 24),
		ExpiresAt: time.Now().UTC(),
	}.IsExpired())
}

func TestToken_ToJwtToken(t *testing.T) {
	issued := time.Now().UTC()
	expires := issued.Add(time.Hour * 24)
	tok := &Token{
		Id:          "foo",
		Type:        "user+robot",
		RequestorId: "Unit Test",
		UserId:      "12345",
		IssuedAt:    issued,
		ExpiresAt:   expires,
	}
	jt := tok.JwtToken(jwt.SigningMethodHS256)
	require.NotNil(t, jt)
	require.NotNil(t, jt.Claims)
	assert.Equal(t, tok.JwtToken(jwt.SigningMethodHS256), tok.JwtToken(jwt.SigningMethodHS256))
	claims, ok := jt.Claims.(jwt.MapClaims)
	require.True(t, ok)
	assert.Equal(t, "foo", claims[JwtClaimTokenId])
	assert.Equal(t, "user+robot", claims[JwtClaimTokenType])
	assert.Equal(t, "Unit Test", claims[JwtClaimRequestorId])
	assert.Equal(t, issued.Format(time.RFC3339Nano), claims[JwtClaimIssuedAt])
	assert.Equal(t, expires.Format(time.RFC3339Nano), claims[JwtClaimExpiresAt])
}

func TestToken_RoundTrip(t *testing.T) {
	issued := time.Now().UTC().Truncate(time.Millisecond)
	expires := issued.Add(time.Hour * 24)
	userRobotToken := &Token{
		Id:          "e76067aa",
		Type:        "user+robot",
		RequestorId: "21cb8cd2",
		UserId:      "233702b2",
		IssuedAt:    issued,
		ExpiresAt:   expires,
	}
	tok2, err := FromJwtToken(userRobotToken.JwtToken(jwt.SigningMethodHS256))
	require.NoError(t, err)
	assert.Equal(t, userRobotToken, tok2)

	userOnlyToken := &Token{
		Id:   "e76067aa",
		Type: "user",
		// RequestorId: "21cb8cd2",
		UserId:    "21cb8cd2",
		IssuedAt:  issued,
		ExpiresAt: expires,
	}
	tok3, err := FromJwtToken(userOnlyToken.JwtToken(jwt.SigningMethodHS256))
	require.NoError(t, err)
	assert.Equal(t, userOnlyToken, tok3)
}

func TestToken_FromTokenNil(t *testing.T) {
	tok, err := FromJwtToken(nil)
	require.Error(t, err)
	assert.Nil(t, tok)
}

func TestToken_Validation(t *testing.T) {
	issued := time.Now().UTC()
	expires := issued.Add(time.Hour * 24)

	tok, err := FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		// JwtClaimTokenId:     "id",
		JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt:   expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId: "id",
		// JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt:   expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:   "id",
		JwtClaimTokenType: "user+robot",
		// JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt:   expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:   "id",
		JwtClaimTokenType: "user+robot",
		JwtClaimUserId:    "user",
		// JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:  issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt: expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:     "id",
		JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		// JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt: expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:     "id",
		JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    "invalid",
		JwtClaimExpiresAt:   expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:     "id",
		JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		// JwtClaimExpiresAt:   expires.Format(time.RFC3339Nano),
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
		JwtClaimTokenId:     "id",
		JwtClaimTokenType:   "user+robot",
		JwtClaimUserId:      "user",
		JwtClaimRequestorId: "robot",
		JwtClaimIssuedAt:    issued.Format(time.RFC3339Nano),
		JwtClaimExpiresAt:   "invalid",
	}))
	require.Error(t, err)
	require.Nil(t, tok)

	tok, err = FromJwtToken(jwt.New(jwt.SigningMethodHS256))
	require.Error(t, err)
	require.Nil(t, tok)
}
