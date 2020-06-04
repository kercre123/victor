package token

import (
	"context"
	"crypto/tls"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/model"
	"github.com/dgrijalva/jwt-go"
	"github.com/jawher/mow.cli"
)

type TokenSigner interface {
	SignToken(token *model.Token) (string, error)
}

// CertificateTokenSigner provides for signing tokens with an x509 key
// pair.
type CertificateTokenSigner struct {
	cfg  *svc.CertConfig
	cert *tls.Certificate
}

func NewCertificateTokenSigner() *CertificateTokenSigner {
	return &CertificateTokenSigner{
		cfg: &svc.CertConfig{Prefix: "token-service-signing"},
	}
}

func (s *CertificateTokenSigner) CommandSpec() string {
	return s.cfg.CommandSpec()
}

func (s *CertificateTokenSigner) CommandInitialize(cmd *cli.Cmd) {
	s.cfg.CommandInitialize(cmd)
}

func (s *CertificateTokenSigner) Start(ctx context.Context) error {
	if c, err := s.cfg.Load(); err != nil {
		return err
	} else {
		s.cert = c
	}
	return nil
}

func (s *CertificateTokenSigner) Stop() error {
	return nil // no-op
}

func (s *CertificateTokenSigner) Kill() error {
	return nil // no-op
}

func (s *CertificateTokenSigner) SignToken(token *model.Token) (string, error) {
	jwt := token.JwtToken(jwt.SigningMethodRS512)
	signingStr, _ := jwt.SigningString()
	if signed, err := jwt.SignedString(s.cert.PrivateKey); err != nil {
		alog.Error{
			"action":             "sign_token",
			"token_id":           token.Id,
			"signing_string":     signingStr,
			"signing_string_len": len(signingStr),
			"status":             alog.StatusError,
			"error":              err,
		}.Log()
		return "", err
	} else {
		return signed, nil
	}
}
