package model

import (
	"errors"
	"fmt"
	"time"

	"github.com/dgrijalva/jwt-go"
)

const (
	JwtClaimTokenId     = "token_id"
	JwtClaimTokenType   = "token_type"
	JwtClaimRequestorId = "requestor_id"
	JwtClaimUserId      = "user_id"
	JwtClaimIssuedAt    = "iat"
	JwtClaimExpiresAt   = "expires"
)

var (
	errorNilToken = errors.New("Missing token")
	errorNoClaims = errors.New("Missing claims")
	locUTC, _     = time.LoadLocation("UTC")
)

// Token is a structured representation of an access token.
type Token struct {
	// Id is the unique ID of the token.
	Id string

	// Type - Only 'user+robot' is supported right now.
	Type string

	// RequestorId is an identifier for the entity which requested the
	// token. Likely to be the common name of the robot cert
	// (i.e. 'vic:<ESN>', or later an Anki Principal URN)
	RequestorId string

	// UserId is the accounts system ID of the user associated with
	// the requesting entity.
	UserId string

	// IssuedAt is the UTC time when the token was issued.
	IssuedAt time.Time

	// ExpiresAt is the UTC time at which the token is no longer
	// valid. Generally equal to IssuedAt + 24 hours.
	ExpiresAt time.Time

	// PurgeAt is the UTC time at which Dynamo will automatically
	// delete the token. Only used within the Token Service.
	PurgeAt time.Time

	// RevokedAt is the UTC time at which the token was revoked, if it
	// has been revoked. Tokens can be revoked due to account system
	// password changes or account deletion. Only used within the
	// Token Service.
	RevokedAt time.Time

	// Revoked is true if this token has been revoked due to account
	// system changes. Only used within the Token Service.
	Revoked bool

	// Raw is the raw string form of the JWT token, if this Token
	// object was parsed from a JWT token. Only used within the Token
	// Service.
	Raw string
}

// IsExpired is a simple predicate indicating whether the token's
// expiration time has passed or not.
func (t Token) IsExpired() bool {
	return time.Now().UTC().After(t.ExpiresAt)
}

// JwtToken converts this Token object to a jwt.Token object suitable
// for hashing and conversion to a signed string.
func (t Token) JwtToken(method jwt.SigningMethod) *jwt.Token {
	return jwt.NewWithClaims(method, jwt.MapClaims{
		JwtClaimTokenId:     t.Id,
		JwtClaimTokenType:   t.Type,
		JwtClaimUserId:      t.UserId,
		JwtClaimRequestorId: t.RequestorId,
		JwtClaimIssuedAt:    t.IssuedAt.Format(time.RFC3339Nano),
		JwtClaimExpiresAt:   t.ExpiresAt.Format(time.RFC3339Nano),
	})
}

// FromJwtToken converts a generic jwt.Token object, parsed from a
// signed token string, into a Token structure, validating that all
// the required Anki token claims are present.
func FromJwtToken(t *jwt.Token) (*Token, error) {
	if t == nil {
		return nil, errorNilToken
	}
	if claims, ok := t.Claims.(jwt.MapClaims); ok {
		tok := &Token{
			Raw: t.Raw,
		}

		if value, ok := claims[JwtClaimTokenId].(string); ok {
			tok.Id = value
		} else {
			return nil, errorMissingClaim(JwtClaimTokenId)
		}

		if value, ok := claims[JwtClaimTokenType].(string); ok {
			tok.Type = value
		} else {
			return nil, errorMissingClaim(JwtClaimTokenType)
		}

		if value, ok := claims[JwtClaimUserId].(string); ok {
			tok.UserId = value
		} else {
			return nil, errorMissingClaim(JwtClaimUserId)
		}

		if value, ok := claims[JwtClaimRequestorId].(string); ok {
			tok.RequestorId = value
		} else {
			return nil, errorMissingClaim(JwtClaimRequestorId)
		}

		if value, ok := claims[JwtClaimIssuedAt].(string); ok {
			if iat, err := time.ParseInLocation(time.RFC3339, value, locUTC); err != nil {
				return nil, err
			} else {
				tok.IssuedAt = iat
			}
		} else {
			return nil, errorMissingClaim(JwtClaimIssuedAt)
		}

		if value, ok := claims[JwtClaimExpiresAt].(string); ok {
			if expires, err := time.ParseInLocation(time.RFC3339, value, locUTC); err != nil {
				return nil, err
			} else {
				tok.ExpiresAt = expires
			}
		} else {
			return nil, errorMissingClaim(JwtClaimExpiresAt)
		}

		return tok, nil
	} else {
		return nil, errorNoClaims
	}
}

//
// helpers
//

func errorMissingClaim(claim string) error {
	return fmt.Errorf("Missing claim %s", claim)
}
