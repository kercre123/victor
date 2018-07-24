// +build !linux

package token

import (
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc/credentials"
)

func init() {
	defaultESN = func() string {
		return "00000000"
	}
}

var (
	defaultTLSCert = rootcerts.ServerCertPool()
)

func getTLSCert() (credentials.TransportCredentials, error) {
	return credentials.NewClientTLSFromCert(defaultTLSCert, ""), nil
}
