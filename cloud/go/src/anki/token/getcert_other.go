// +build !vicos

package token

import (
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc/credentials"
)

var (
	defaultTLSCert = rootcerts.ServerCertPool()
)

func getTLSCert() (credentials.TransportCredentials, error) {
	return credentials.NewClientTLSFromCert(defaultTLSCert, ""), nil
}
