// +build !vicos

package token

import (
	"anki/robot"
	"crypto/tls"

	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc/credentials"
)

var (
	// UseClientCert can be set to true to force the use of client certs
	UseClientCert = false

	defaultTLSCert = rootcerts.ServerCertPool()
	robotCreds     credentials.TransportCredentials
)

func getTLSCert() (credentials.TransportCredentials, error) {
	if !UseClientCert {
		return credentials.NewClientTLSFromCert(defaultTLSCert, ""), nil
	}

	if robotCreds == nil {
		cert, err := robot.TLSKeyPair(robot.DefaultCloudDir)
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
