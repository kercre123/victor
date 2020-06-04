// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build nolibxmlsec1

package saml

/*
#cgo nolibxmlsec1 CFLAGS: -DNOBUILDSIG=1
*/
import "C"

func Initialize() {}

func Shutdown() {}

func (s *SAMLIDP) ValidateSignature(xmldata string) *XmlSecError {
	return NotAvailable
}
