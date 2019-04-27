package svc

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/base64"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

// CertConfig encapsulates configuring an x509 certificate/private key
// pair from config variables and loading it.
//
// When initialized, a CertConfig will generate 4 mow.Cli options,
// allowing the certificate and private key to be loaded either from
// an immediate block of PEM data, or from a file on disk. The value
// of Prefix is prepended to the options to allow for more than one
// config to be loaded.
//
// E.g.:
//   --prefix-cert ($PREFIX_CERT)
//   --prefix-certfile ($PREFIX_CERTFILE)
//   --prefix-key ($PREFIX_KEY)
//   --prefix-keyfile ($PREFIX_KEYFILE)
//
// If --prefix-cert and --prefix-certfile are provided, the value in
// --prefix-cert takes precedence.
//
// If --prefix-key and --prefix-keyfile are provided, the value in
// --prefix-key takes precedence.
type CertConfig struct {
	// Prefix is prepended to the names of the command line
	// arguments/environment variables used to initialize the cert, so
	// that multiple certs can be loaded. It is an error to leave this
	// empty.
	Prefix string

	// Certificate is the same tls.Certificate as returned by Load,
	// cached for later reference (e.g. for logging). The
	// Certificate.Leaf (an x509.Certificate) member will contain a
	// pointer to the parsed certificate after Load() (the standard
	// library discards this and re-parses the cert as needed; storing
	// the parsed copy in Leaf is both a convenience, and also an
	// optimization for connection establishment).
	Certificate *tls.Certificate

	certData           *string
	certFile           *string
	keyData            *string
	keyFile            *string
	ServerNameOverride *string
}

func (cfg *CertConfig) argname(arg string) string {
	return fmt.Sprintf("%s-%s", cfg.Prefix, arg)
}

func (cfg *CertConfig) CommandSpec() string {
	return fmt.Sprintf("[--%s|--%s] [--%s|--%s] [--%s]",
		cfg.argname("cert"), cfg.argname("certfile"),
		cfg.argname("key"), cfg.argname("keyfile"),
		cfg.argname("server-name-override"))
}

func (cfg *CertConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.certData = StringOpt(cmd, cfg.argname("cert"), "",
		cfg.Prefix+" x509 certificate as PEM or Base64-encoded PEM")
	cfg.keyData = StringOpt(cmd, cfg.argname("key"), "",
		cfg.Prefix+" x509 private key as PEM or Base64-encoded PEM")
	cfg.certFile = StringOpt(cmd, cfg.argname("certfile"), "",
		cfg.Prefix+" x509 certificate filename")
	cfg.keyFile = StringOpt(cmd, cfg.argname("keyfile"), "",
		cfg.Prefix+" x509 private key filename")
	cfg.ServerNameOverride = StringOpt(cmd, cfg.argname("server-name-override"), "",
		cfg.Prefix+" override for server name used in TLS cert validation")
}

func (cfg *CertConfig) Load() (*tls.Certificate, error) {
	if err := cfg.validate(); err != nil {
		return nil, err
	}
	var certBlock []byte
	if *cfg.certData != "" {
		// For some reason Chipper was initially coded with the PEM
		// data Base64 encoded, so try that first. If it's a regular
		// PEM block, this will fail, and we can use it directly.
		if decoded, err := base64.StdEncoding.DecodeString(*cfg.certData); err != nil {
			certBlock = []byte(*cfg.certData)
		} else {
			certBlock = decoded
		}
	} else {
		if c, err := ioutil.ReadFile(*cfg.certFile); err != nil {
			return nil, fmt.Errorf("CertConfig %s: error loading certificate file: %s", cfg.Prefix, err)
		} else {
			certBlock = c
		}
	}

	var keyBlock []byte
	if *cfg.keyData != "" {
		if decoded, err := base64.StdEncoding.DecodeString(*cfg.keyData); err != nil {
			keyBlock = []byte(*cfg.keyData)
		} else {
			keyBlock = decoded
		}
	} else {
		if k, err := ioutil.ReadFile(*cfg.keyFile); err != nil {
			return nil, fmt.Errorf("CertConfig %s: error loading key file: %s", cfg.Prefix, err)
		} else {
			keyBlock = k
		}
	}

	if cert, err := tls.X509KeyPair(certBlock, keyBlock); err != nil {
		return nil, fmt.Errorf("CertConfig %s: error constructing key pair: %s", cfg.Prefix, err)
	} else {
		if cert.Leaf == nil {
			x509Cert, err := x509.ParseCertificate(cert.Certificate[0])
			if err != nil {
				return nil, fmt.Errorf("CertConfig %s: error parsing certificate: %s", cfg.Prefix, err)
			}
			cert.Leaf = x509Cert
		}
		alog.Info{
			"action":        "load_certificate",
			"type":          "pair",
			"name":          cert.Leaf.Subject.CommonName,
			"issuer":        cert.Leaf.Issuer.CommonName,
			"is_ca":         cert.Leaf.IsCA,
			"serial_number": cert.Leaf.SerialNumber.String(),
		}.Log()
		cfg.Certificate = &cert
		return &cert, nil
	}
}

func (cfg *CertConfig) validate() error {
	if cfg.Prefix == "" {
		return fmt.Errorf("CertConfig.Prefix cannot be empty")
	}
	if (cfg.certData == nil || *cfg.certData == "") && (cfg.certFile == nil || *cfg.certFile == "") {
		return fmt.Errorf("Missing cert config for %s. One of %s or %s must be set",
			cfg.Prefix, cfg.argname("cert"), cfg.argname("certfile"))
	}
	if (cfg.keyData == nil || *cfg.keyData == "") && (cfg.keyFile == nil || *cfg.keyFile == "") {
		return fmt.Errorf("Missing key config for %s. One of %s or %s must be set",
			cfg.Prefix, cfg.argname("key"), cfg.argname("keyfile"))
	}
	return nil
}
