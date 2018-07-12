package token

import (
	"anki/robot"
	"crypto/tls"

	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc/credentials"
)

func init() {
	if opt := robot.OSUserAgent(); opt != nil {
		platformOpts = append(platformOpts, opt)
	}
}

var robotCreds credentials.TransportCredentials

func getTLSCert() (credentials.TransportCredentials, error) {
	if robotCreds == nil {
		cert, err := robot.TLSKeyPair("/factory/cloud")
		if err != nil {
			return nil, err
		}
		robotCreds = credentials.NewTLS(&tls.Config{
			Certificates: []tls.Certificate{cert},
			RootCAs:      rootcerts.ServerCertPool(),
		})
	}
	return robotCreds, nil
}
