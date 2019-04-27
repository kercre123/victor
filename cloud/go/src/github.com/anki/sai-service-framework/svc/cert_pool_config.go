package svc

import (
	"crypto/x509"
	"encoding/pem"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-go-util/log"
	"github.com/gwatts/rootcerts"
	"github.com/jawher/mow.cli"
)

// CertPoolConfig loads an x509 certificate pool from PEM data,
// supplied either in an environment variable or a file on disk,
// optionally including the system roots.
//
// When initialized it will generate three mow.Cli options:
//   --prefix-cert-pool ($PREFIX_CERT_POOL)
//   --prefix-cert-pool-file ($PREFIX_CERT_POOL_FILE)
//   --prefix-include-system ($PREFIX_INCLUDE_SYSTEM)
//
// If both --prefix-cert-pool and --prefix-cert-pool-file are
// supplied, --prefix-cert-pool will be used.
type CertPoolConfig struct {
	Prefix string
	Pool   *x509.CertPool

	includeSystem *bool
	certPoolData  *string
	certPoolFile  *string
}

func (cfg *CertPoolConfig) argname(arg string) string {
	return fmt.Sprintf("%s-%s", cfg.Prefix, arg)
}

func (cfg *CertPoolConfig) CommandSpec() string {
	return fmt.Sprintf("[--%s|--%s] [--%s]",
		cfg.argname("cert-pool-file"),
		cfg.argname("cert-pool"),
		cfg.argname("include-system"))
}

func (cfg *CertPoolConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.certPoolData = StringOpt(cmd, cfg.argname("cert-pool"), "",
		cfg.Prefix+" x509 certificate or certificates as PEM data. Overrides cert-pool-file if both are supplied.")
	cfg.certPoolFile = StringOpt(cmd, cfg.argname("cert-pool-file"), "",
		cfg.Prefix+" filename to load x509 certificate or certificates")
	cfg.includeSystem = BoolOpt(cmd, cfg.argname("include-system"), false,
		cfg.Prefix+" include system root certificates")
}

func (cfg *CertPoolConfig) Load() (*x509.CertPool, error) {
	// Set up PEM block data
	var pemData []byte
	if cfg.certPoolData != nil && *cfg.certPoolData != "" {
		pemData = []byte(*cfg.certPoolData)
	} else {
		if cfg.certPoolFile == nil {
			return nil, fmt.Errorf("CertPoolConfig %s: must set one of cert-pool or cert-pool-file", cfg.Prefix)
		}
		if data, err := ioutil.ReadFile(*cfg.certPoolFile); err != nil {
			return nil, fmt.Errorf("CertPoolConfig %s: error loading cert pool file: %s", cfg.Prefix, err)
		} else {
			pemData = data
		}
	}

	// Set up initial pool
	pool := x509.NewCertPool()
	if cfg.includeSystem != nil && *cfg.includeSystem {
		for _, cert := range rootcerts.Certs() {
			pool.AddCert(cert.X509Cert())
		}
	}

	// Parse the certs out of the PEM block and load them to the pool
	for len(pemData) > 0 {
		// Get the next block. If pem.Decode() returns nil we've
		// reached the end.
		var block *pem.Block
		block, pemData = pem.Decode(pemData)
		if block == nil {
			break
		}
		if block.Type != "CERTIFICATE" || len(block.Headers) != 0 {
			return nil, fmt.Errorf("CertPoolConfig %s: Non-certificate data found in PEM block", cfg.Prefix)
		}

		if cert, err := x509.ParseCertificate(block.Bytes); err != nil {
			return nil, fmt.Errorf("CertPoolConfig %s: error parsing certificate: %s", cfg.Prefix, err)
		} else {
			alog.Info{
				"action":        "load_certificate_pool",
				"type":          "cert",
				"name":          cert.Subject.CommonName,
				"issuer":        cert.Issuer.CommonName,
				"is_ca":         cert.IsCA,
				"serial_number": cert.SerialNumber.String(),
			}.Log()
			pool.AddCert(cert)
		}
	}

	if len(pool.Subjects()) == 0 {
		return nil, fmt.Errorf("CertPoolConfig %s: No certificates found in cert pool", cfg.Prefix)
	}

	cfg.Pool = pool
	return pool, nil
}
