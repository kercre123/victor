package grpcclient

import (
	"crypto/tls"
	"fmt"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/grpc/interceptors"
	"github.com/anki/sai-service-framework/svc"
	"github.com/gwatts/rootcerts"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

const (
	DefaultGrpcClientAddr = "0.0.0.0"
	DefaultGrpcClientPort = 8000
)

// ClientConfig encapsulates the parameters necessary to configure a
// client connection to a gRPC service. Like ClientCertConfig (which
// ClientConfig contains an instance of for cert-based credentials),
// ClientConfig generates a set of mow.Cli command-line
// args/environment variables when initialized.
//
// The Prefix is used to ensure each instance of ClientConfig
// generates a unique set of configuration parameters. It is an error
// to leave it empty, or to create 2 ClientConfig instances with the
// same Prefix.
//
// Example parameters generated:
//
//   --prefix-grpc-client-port ($PREFIX_GRPC_CLIENT_PORT)
//   --prefix-grpc-client-addr ($PREFIX_GRPC_CLIENT_ADDR)
//   --prefix-grpc-client-tls-enable ($PREFIX_GRPC_CLIENT_TLS_ENABLE)
//   --prefix-grpc-client-mutual-tls ($PREFIX_GRPC_CLIENT_MUTUAL_TLS)
//   --prefix-grpc-client-cert ($PREFIX_GRPC_CLIENT_CERT)
//   --prefix-grpc-client-certfile ($PREFIX_GRPC_CLIENT_CERTFILE)
//   --prefix-grpc-client-ca-cert ($PREFIX_GRPC_CLIENT_CA_CERT)
//   --prefix-grpc-client-ca-certfile ($PREFIX_GRPC_CLIENT_CA_CERTFILE)
//   --prefix-grpc-client-key ($PREFIX_GRPC_CLIENT_KEY)
//   --prefix-grpc-client-keyfile ($PREFIX_GRPC_CLIENT_KEYFILE)
//   --prefix-grpc-client-appkey ($PREFIX_GRPC_CLIENT_APPKEY)

type ClientConfig struct {
	// Prefix is prepended to the names of the command line
	// arguments/environment variables used to initialize the client
	// config, so that multiple client configs for different services
	// can be loaded. It is an error to leave this empty.
	Prefix string

	// port specifies the network port of the gRPC service to connect
	// to.
	port *int

	// addr specifies the network IP address or hostname of the gRPC
	// service to connect to.
	addr *string

	// tlsEnable enables or disables transport layer security
	tlsEnable *bool

	// mutualTls enables using a peer certificate/key pair for mutual
	// TLS authentication, where the server validates the client's
	// certificate.
	mutualTls *bool

	// tlsSkipVerification disables verification of server
	// certificates. This will set InsecureSkipVerify = true in the
	// TLS config used for client connections.
	tlsSkipVerification *bool

	// peerCertConfig configures an x509 certificate/key pair to use as peer
	// credentials for mutual TLS.
	peerCertConfig *svc.CertConfig

	// caCertConfig configures an x509 public certificate to use as a
	// trusted certificate authority for TLS connection verification.
	caCertConfig *svc.PublicCertConfig

	// appkey configures the Anki application key to pass to the gRPC
	// service, in the anki-app-key metadata field.
	appkey *string
}

func NewClientConfig(prefix string) *ClientConfig {
	return &ClientConfig{
		Prefix: prefix,
		peerCertConfig: &svc.CertConfig{
			Prefix: prefix + "-grpc-client-peer",
		},
		caCertConfig: &svc.PublicCertConfig{
			Prefix: prefix + "-grpc-client-ca",
		},
	}
}

func (cfg *ClientConfig) argname(arg string) string {
	return fmt.Sprintf("%s-grpc-client-%s", cfg.Prefix, arg)
}

func (cfg *ClientConfig) CommandSpec() string {
	return fmt.Sprintf("[--%s] [--%s] [--%s] [--%s] [--%s] [--%s] %s %s",
		cfg.argname("addr"),
		cfg.argname("port"),
		cfg.argname("tls-enable"),
		cfg.argname("tls-skip-verification"),
		cfg.argname("mutual-tls"),
		cfg.argname("appkey"),
		cfg.peerCertConfig.CommandSpec(),
		cfg.caCertConfig.CommandSpec())
}

func (cfg *ClientConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.peerCertConfig.CommandInitialize(cmd)
	cfg.caCertConfig.CommandInitialize(cmd)
	cfg.addr = svc.StringOpt(cmd, cfg.argname("addr"), DefaultGrpcClientAddr,
		fmt.Sprintf("Service address to connect the %s client to", cfg.Prefix))
	cfg.port = svc.IntOpt(cmd, cfg.argname("port"), DefaultGrpcClientPort,
		fmt.Sprintf("Service port to connect the %s client to", cfg.Prefix))
	cfg.tlsEnable = svc.BoolOpt(cmd, cfg.argname("tls-enable"), true,
		fmt.Sprintf("Enables transport layer security for the %s client", cfg.Prefix))
	cfg.tlsSkipVerification = svc.BoolOpt(cmd, cfg.argname("tls-skip-verification"), false,
		fmt.Sprintf("Disables verification of server certificates by the %s client", cfg.Prefix))
	cfg.mutualTls = svc.BoolOpt(cmd, cfg.argname("mutual-tls"), false,
		"Enable mutual TLS. Certificate and key are required if enabled.")
	cfg.appkey = svc.StringOpt(cmd, cfg.argname("appkey"), "",
		fmt.Sprintf("Appkey to pass to the %s service", cfg.Prefix))
}

func (cfg *ClientConfig) Dial(opts ...grpc.DialOption) (*grpc.ClientConn, error) {
	options := []grpc.DialOption{
		grpcutil.WithBuildInfoUserAgent(),
		WithStandardUnaryClientChain(interceptors.AppKeyUnaryClientInterceptor(*cfg.appkey)),
		WithStandardStreamClientChain(interceptors.AppKeyStreamClientInterceptor(*cfg.appkey)),
	}
	var peerCertName string
	if !*cfg.tlsEnable {
		options = append(options, grpc.WithInsecure())
	} else {
		tlsConfig := &tls.Config{
			RootCAs:            rootcerts.ServerCertPool(),
			InsecureSkipVerify: *cfg.tlsSkipVerification,
		}

		if *cfg.mutualTls {
			peerCert, err := cfg.peerCertConfig.Load()
			if err != nil {
				return nil, err
			}
			tlsConfig.Certificates = []tls.Certificate{*peerCert}
			tlsConfig.ServerName = *cfg.peerCertConfig.ServerNameOverride
			peerCertName = peerCert.Leaf.Subject.CommonName
		}

		if caCert, err := cfg.caCertConfig.Load(); err == nil {
			tlsConfig.RootCAs.AddCert(caCert)
			tlsConfig.ClientCAs = tlsConfig.RootCAs
		}

		options = append(options,
			grpc.WithTransportCredentials(credentials.NewTLS(tlsConfig)))
	}

	conn, err := grpc.Dial(cfg.Addr(), options...)
	if err != nil {
		return nil, err
		svc.Fail("%s", err)
	}
	alog.Info{
		"action":           "dial",
		"status":           "connected",
		"addr":             cfg.Addr(),
		"client":           cfg.Prefix,
		"client_cert_name": peerCertName,
		"client_appkey":    grpcutil.SanitizeAppKey(*cfg.appkey),
	}.Log()
	return conn, nil
}

func (cfg *ClientConfig) Addr() string {
	addr := DefaultGrpcClientAddr
	port := DefaultGrpcClientPort
	if cfg.addr != nil && *cfg.addr != "" {
		addr = *cfg.addr
	}
	if cfg.port != nil && *cfg.port != 0 {
		port = *cfg.port
	}
	return fmt.Sprintf("%s:%d", addr, port)
}
