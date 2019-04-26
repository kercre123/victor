package token

import (
	"context"
	"crypto/x509"
	"time"

	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/model"
	"github.com/dgrijalva/jwt-go"
	"github.com/jawher/mow.cli"
)

type TokenValidator interface {
	svc.ServiceObject

	// IsValidToken validates a JWT token.
	//
	// A token is invalid if any of the following conditions are true:
	//   * the client has been configured with a signature verification
	//     key and the signature fails verification
	//   * it is missing any of the claim fields
	//   * it has expired.
	IsValidToken(token string) bool

	// TokenFromString parses the signed token string and returns a
	// structured form of the token with the claims readily
	// accessible.
	//
	// If the client has been configured with a signature verification
	// key via WithVerificationKey, the token signature will be
	// validated, and an error will be returned if it is invalid.
	//
	// If no key has been configured, the token will be decoded and
	// parsed without verification.
	TokenFromString(token string) (*model.Token, error)
}

type PublicKeyTokenValidator struct {
	tmsCertConfig *svc.PublicCertConfig
	cert          *x509.Certificate
	key           interface{}
}

type PublicKeyTokenValidatorOpt func(v *PublicKeyTokenValidator)

func WithKey(key interface{}) PublicKeyTokenValidatorOpt {
	return func(v *PublicKeyTokenValidator) {
		v.key = key
	}
}

func NewPublicKeyTokenValidator(prefix string, opts ...PublicKeyTokenValidatorOpt) *PublicKeyTokenValidator {
	v := &PublicKeyTokenValidator{
		tmsCertConfig: &svc.PublicCertConfig{
			Prefix: prefix,
		},
	}
	for _, opt := range opts {
		opt(v)
	}
	return v
}

func NewValidator(opts ...PublicKeyTokenValidatorOpt) TokenValidator {
	return NewPublicKeyTokenValidator("token-service-signing", opts...)
}

func (v *PublicKeyTokenValidator) CommandSpec() string {
	return v.tmsCertConfig.CommandSpec()
}

func (v *PublicKeyTokenValidator) CommandInitialize(cmd *cli.Cmd) {
	v.tmsCertConfig.CommandInitialize(cmd)
}

func (v *PublicKeyTokenValidator) Start(ctx context.Context) error {
	if v.key == nil {
		cert, err := v.tmsCertConfig.Load()
		if err != nil {
			return err
		}
		v.cert = cert
		v.key = cert.PublicKey
	}
	return nil
}

func (v *PublicKeyTokenValidator) Stop() error {
	return nil
}

func (v *PublicKeyTokenValidator) Kill() error {
	return nil
}

func (v *PublicKeyTokenValidator) IsValidToken(token string) bool {
	if tok, err := v.TokenFromString(token); err != nil {
		return false
	} else {
		return tok.IssuedAt.Before(time.Now().UTC())
	}
}

func (v *PublicKeyTokenValidator) TokenFromString(token string) (*model.Token, error) {
	if v.key != nil {
		tok, err := jwt.Parse(token, func(t *jwt.Token) (interface{}, error) {
			// TODO: validate signing method (alg)
			return v.key, nil
		})
		if err != nil {
			return nil, err
		}
		return model.FromJwtToken(tok)
	} else {
		if tok, _, err := new(jwt.Parser).ParseUnverified(token, jwt.MapClaims{}); err != nil {
			return nil, err
		} else {
			return model.FromJwtToken(tok)
		}
	}
}
