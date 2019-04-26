// +build nolibxmlsec1

package session

import "errors"

func SetActiveSAMLIDP(name string) error {
	return errors.New("No SAML support compiled in; requires saml build flag")
}

func IDPNames() (names []string) {
	return names
}
