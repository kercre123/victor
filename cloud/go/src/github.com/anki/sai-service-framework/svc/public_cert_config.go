package svc

import (
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

// PublicCertConfig encapsulates configuring an x509 public
// certificate from config variables and loading it.
//
// When initialized, a PublicCertConfig will generate 2 mow.Cli
// options, allowing the certificate to be loaded either from an
// immediate block of PEM data, or from a file on disk. The value of
// Prefix is prepended to the options to allow for more than one
// config to be loaded.
//
// E.g.:
//   --prefix-public-cert ($PREFIX_PUBLIC_CERT)
//   --prefix-public-certfile ($PREFIX_PUBLIC_CERTFILE)
//
// If --prefix-public-cert and --prefix-public-certfile are provided,
// the value in --prefix-public-cert takes precedence.
type PublicCertConfig struct {
	// Prefix is prepended to the names of the command line
	// arguments/environment variables used to initialize the cert, so
	// that multiple certs can be loaded. It is an error to leave this
	// empty.
	Prefix string

	// Certificate is the same x509.Certificate as returned by Load,
	// cached for later reference (e.g. for logging).
	Certificate *x509.Certificate

	certData *string
	certFile *string
}

func (cfg *PublicCertConfig) argname(arg string) string {
	return fmt.Sprintf("%s-public-%s", cfg.Prefix, arg)
}

func (cfg *PublicCertConfig) CommandSpec() string {
	return fmt.Sprintf("[--%s|--%s]",
		cfg.argname("cert"), cfg.argname("certfile"))
}

func (cfg *PublicCertConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.certData = StringOpt(cmd, cfg.argname("cert"), "",
		cfg.Prefix+" x509 certificate as PEM or Base64-encoded PEM")
	cfg.certFile = StringOpt(cmd, cfg.argname("certfile"), "",
		cfg.Prefix+" x509 certificate filename")
}

func (cfg *PublicCertConfig) Load() (*x509.Certificate, error) {
	if err := cfg.validate(); err != nil {
		return nil, err
	}
	var certPEM []byte
	if *cfg.certData != "" {
		// For some reason Chipper was initially coded with the PEM
		// data Base64 encoded, so try that first. If it's a regular
		// PEM block, this will fail, and we can use it directly.
		if decoded, err := base64.StdEncoding.DecodeString(*cfg.certData); err != nil {
			certPEM = []byte(*cfg.certData)
		} else {
			certPEM = decoded
		}
	} else {
		if c, err := ioutil.ReadFile(*cfg.certFile); err != nil {
			return nil, fmt.Errorf("PublicCertConfig %s: error reading cert file: %s", cfg.Prefix, err)
		} else {
			certPEM = c
		}
	}

	certBlock, _ := pem.Decode(certPEM)
	if certBlock == nil {
		return nil, fmt.Errorf("Failed to decode PEM data for %s public certificate", cfg.Prefix)
	}
	if cert, err := x509.ParseCertificate(certBlock.Bytes); err != nil {
		return nil, fmt.Errorf("PublicCertConfig %s: error parsing certificate: %s", cfg.Prefix, err)
	} else {
		alog.Info{
			"action":        "load_certificate",
			"type":          "public",
			"name":          cert.Subject.CommonName,
			"issuer":        cert.Issuer.CommonName,
			"is_ca":         cert.IsCA,
			"serial_number": cert.SerialNumber.String(),
		}.Log()
		cfg.Certificate = cert
		return cert, nil
	}
}

func (cfg *PublicCertConfig) validate() error {
	if cfg.Prefix == "" {
		return fmt.Errorf("PublicCertConfig.Prefix cannot be empty")
	}
	if (cfg.certData == nil || *cfg.certData == "") && (cfg.certFile == nil || *cfg.certFile == "") {
		return fmt.Errorf("Missing cert config for %s. One of %s or %s must be set",
			cfg.Prefix, cfg.argname("cert"), cfg.argname("certfile"))
	}
	return nil
}
