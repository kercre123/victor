// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build !nolibxmlsec1

// Package samlsig implements XML signature validation & verification for SAML
// documents.
//
// It requires the libxmlsec1-dev package to build and will cause the resulting program
// to be dynamically linked against libxmlsec1 and related libraries
package saml

/*
#cgo nolibxmlsec1 CFLAGS: -DNOBUILDSIG=1
#cgo pkg-config: xmlsec1
#include "sig.h"
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"runtime"

	"unsafe"
)

var (
	initialized bool
)

func init() {
	Initialize()
}

// Initialize initializes the xmlsec subsystem.
// It is automatically called by init() so it isn't necessary to call
// this explicitly unless Shutdown() has been called first.
func Initialize() {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	if C.init_xmlsec() != 0 {
		err := C.last_xmlsec_error()
		panic(fmt.Sprintf("Failed to initialize xmlsec: %d: %v", err.code, err.msg))
	}
	initialized = true
}

// Shutdown causes xmlsec to shutdown and release allocated resources.
// Subsequent calls to Validate will fail unless Initialize() is called again.
func Shutdown() {
	C.shutdown_xmlsec()
	initialized = false
}

// ValidateSignature takes an XLM SAML response and verifies the signature embdedded in the document
// against the certificate supplied by the IDP metadata.
func (s *SAMLIDP) ValidateSignature(xmldata string) *XmlSecError {
	if !initialized {
		return NotInitialized
	}
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	ccert := C.CString(s.Metadata.SignPEM())
	cxml := C.CString(xmldata)
	defer C.free(unsafe.Pointer(ccert))
	defer C.free(unsafe.Pointer(cxml))

	if r := C.verify_signed_xml(cxml, ccert); r == 1 {
		return nil
	}
	err := C.last_xmlsec_error()
	return &XmlSecError{
		Code: int(err.code),
		Msg:  C.GoString((*C.char)(unsafe.Pointer(&err.msg))),
	}
}
