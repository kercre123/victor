// Copyright 2015 Gareth Watts
// Licensed under an MIT license
// See the LICENSE file for details

/*
Package certparse parses root CA certificates from a Mozilla NSS certdata.txt io.Reader.

This package provides a low level scanner, which can read individual values from the file,
as well as objects (certificates or trust declarations) and a high level ReadTrustedCerts
function which will parse objects into x509 certificates and return those that have
been labeled as trusted as delegator in the certdata file (meaning they can be used by a CA
to sign certificates).

The certdata.txt file format changes occasionally, which may cause this parser to break.
*/
package certparse

import (
	"bufio"
	"crypto/x509"
	"errors"
	"fmt"
	"io"
	"strconv"
	"strings"
)

var (
	knownTrustLevels = []string{"CKT_NSS_TRUSTED_DELEGATOR", "CKT_NSS_MUST_VERIFY_TRUST", "CKT_NSS_NOT_TRUSTED"}
	trustedDelegator = "CKT_NSS_TRUSTED_DELEGATOR"
)

// Cert holds the raw data parsed from the certdata.txt file, along with
// the Go x509.Certificate representation.
type Cert struct {
	Label string
	Data  []byte // Raw DER data
	Trust TrustLevel
	Cert  *x509.Certificate
}

type TrustLevel int

const (
	ServerTrustedDelegator TrustLevel = 1 << iota // Trusted for issuing server certificates
	EmailTrustedDelegator                         // Trusted for issuing email certificates
	CodeTrustedDelegator                          // Trusted for issuing code signing certificates
)

// A MozValue is returned from MozScanner.ScanValue.
type MozValue struct {
	Field string
	Type  string
	Value string
}

// MozScanner scans and tokenizes certdata.txt files.
type MozScanner struct {
	s             *bufio.Scanner
	ln            int
	skippedHeader bool
	lastValue     MozValue
	valueError    error
	lastObject    map[string]string
	objectError   error
}

// NewMozScanner initializes a scanner ready for processing.
// It expects a reader supplying a certdata.txt formatted text.
func NewMozScanner(f io.Reader) *MozScanner {
	return &MozScanner{
		s: bufio.NewScanner(f),
	}
}

func (ms *MozScanner) skipHeader() error {
	for ms.s.Scan() {
		line := ms.s.Text()
		ms.ln++
		if line == "BEGINDATA" {
			ms.skippedHeader = true
			return nil
		}
	}
	return errors.New("BEGINDATA line not found in certdata input")
}

// LineNumber returns the last line number scanned.
func (ms *MozScanner) LineNumber() int {
	return ms.ln
}

// ScanValue reads the next field, filed type and value.
// It returns false when no more values can be read, either due to reaching EOF, or encountering
// an error.  Errors can be read by calling ScanValueError
func (ms *MozScanner) ScanValue() bool {
	ms.valueError = nil

	if !ms.skippedHeader {
		if err := ms.skipHeader(); err != nil {
			ms.valueError = err
			return false
		}
	}

	var field, ftype, value string
	var err error

	for ms.s.Scan() {
		line := ms.s.Text()
		ms.ln++
		if len(line) == 0 || line[0] == '#' {
			continue
		}
		parts := strings.SplitN(line, " ", 3)
		if len(parts) < 2 {
			continue
		}
		field, ftype = parts[0], parts[1]

		switch {
		case ftype == "UTF8" && len(parts) > 2:
			value, err = strconv.Unquote(parts[2])
			if err != nil {
				value = parts[2][1 : len(parts[2])-1] // manually strip quotes
			}
		case strings.HasPrefix(ftype, "MULTILINE"):
			switch mltype := ftype[len("MULTILINE_"):]; mltype {
			case "OCTAL":
				if value, err = ms.readMultilineOctal(); err != nil {
					ms.valueError = err
					return false
				}
			default:
				// swallow unhandled multiline encodings
				if _, err = ms.readMultilineGeneric(); err != nil {
					ms.valueError = err
					return false
				}
			}

		default:
			if len(parts) > 2 {
				value = parts[2]
			}
		}
		ms.lastValue = MozValue{Field: field, Type: ftype, Value: value}
		return true
	}
	return false
}

// ScanValueError returns the last non-EOF error encountered by ScanValue
func (ms *MozScanner) ScanValueError() error {
	return ms.valueError
}

// Value returns the most recent value read by ScanValue
func (ms *MozScanner) Value() MozValue {
	return ms.lastValue
}

func (ms *MozScanner) readMultiline(process func(line string) error) error {
	for ms.s.Scan() {
		line := ms.s.Text()
		ms.ln++
		if line == "END" {
			return nil
		}
		if err := process(line); err != nil {
			return err
		}
	}
	return errors.New("unexpected EOF in multiline value")
}

func (ms *MozScanner) readMultilineGeneric() (value string, err error) {
	var v []string
	ms.readMultiline(func(line string) error {
		v = append(v, line)
		return nil
	})
	return strings.Join(v, "\n"), nil
}

func (ms *MozScanner) readMultilineOctal() (value string, err error) {
	var v []byte
	ms.readMultiline(func(line string) error {
		for i := 0; i < len(line); i += 4 {
			n, err := strconv.ParseInt(line[i+1:i+4], 8, 16)
			if err != nil {
				return err
			}
			v = append(v, byte(n))
		}
		return nil
	})
	return string(v), nil
}

// ScanObject repeatedly scans values to accumulate a complete object, which can be read
// by calllng the Object method.
// It returns false when no more objects can be read, either due to reaching EOF, or encountering
// an error.  Errors can be read by calling ScanObjectError
func (ms *MozScanner) ScanObject() bool {
	value := ms.Value()
	ms.objectError = nil
	object := make(map[string]string)
	for value.Field != "CKA_CLASS" && ms.ScanValue() {
		value = ms.Value()
	}
	if err := ms.ScanValueError(); err != nil {
		ms.objectError = ms.valueError
		return false
	}
	if value.Field != "CKA_CLASS" {
		return false
	}

	object[value.Field] = value.Value

	for ms.ScanValue() {
		value = ms.Value()
		if value.Field == "CKA_CLASS" {
			ms.lastObject = object
			return true
		}
		object[value.Field] = value.Value
	}

	if err := ms.ScanValueError(); err != nil {
		ms.objectError = err
		return false
	}
	ms.lastObject = object
	return true
}

// ScanObjectError returns the last non EOF error encountered by ScanObject
func (ms *MozScanner) ScanObjectError() error {
	return ms.objectError
}

// Object returns the last complete object read by ScanObject.  It returns
// a map of field names to their decoded string values.
func (ms *MozScanner) Object() map[string]string {
	return ms.lastObject
}

// ReadObjects parses all objects from the passed in certdata.txt input.
func ReadObjects(f io.Reader) (objects []map[string]string, err error) {
	scanner := NewMozScanner(f)
	for scanner.ScanObject() {
		objects = append(objects, scanner.Object())
	}
	if err := scanner.ScanObjectError(); err != nil {
		return nil, err
	}
	return objects, nil
}

// ReadTrustedCerts parses a certdata.txt formatted input and returns
// the certificates defined within it that are labelled as trusted as a CA.
// Untrusted, or non-CA certificates are not returned.
func ReadTrustedCerts(f io.Reader) (certs []Cert, err error) {
	objects, err := ReadObjects(f)
	if err != nil {
		return nil, err
	}

	// determine trust
	trusted, err := findTrusted(objects)
	if err != nil {
		return nil, err
	}

	for _, obj := range objects {
		if obj["CKA_CLASS"] != "CKO_CERTIFICATE" {
			continue // we're only interested in certificates
		}

		trust, isTrusted := trusted[obj["CKA_LABEL"]]
		if !isTrusted {
			continue
		}

		// make sure Go can load the certificate
		// This will fail for the EC-ACC certificate as it has a negative serial number
		// See https://github.com/golang/go/issues/8265
		// See https://bugzilla.mozilla.org/show_bug.cgi?id=707995
		cert, err := x509.ParseCertificate([]byte(obj["CKA_VALUE"]))
		if err != nil {
			continue
		}

		certs = append(certs, Cert{
			Label: obj["CKA_LABEL"],
			Data:  []byte(obj["CKA_VALUE"]),
			Cert:  cert,
			Trust: trust,
		})
	}
	return certs, nil
}

func findTrusted(objects []map[string]string) (map[string]TrustLevel, error) {
	trusted := make(map[string]TrustLevel)
	for _, obj := range objects {
		if obj["CKA_CLASS"] != "CKO_NSS_TRUST" {
			continue
		}
		// Make sure the entry only references trust levels we know about and that the file
		// format hasn't changed.
		serverTrust := obj["CKA_TRUST_SERVER_AUTH"]
		emailTrust := obj["CKA_TRUST_EMAIL_PROTECTION"]
		codeTrust := obj["CKA_TRUST_CODE_SIGNING"]
		if !contains(serverTrust, knownTrustLevels) {
			return nil, fmt.Errorf("unknown trust level %q referenced", serverTrust)
		}
		if !contains(emailTrust, knownTrustLevels) {
			return nil, fmt.Errorf("unknown trust level %q referenced", serverTrust)
		}
		if !contains(codeTrust, knownTrustLevels) {
			return nil, fmt.Errorf("unknown trust level %q referenced", serverTrust)
		}
		if serverTrust == "CKT_NSS_NOT_TRUSTED" || emailTrust == "CKT_NSS_NOT_TRUSTED" || codeTrust == "CKT_NSS_NOT_TRUSTED" {
			// not trusted for one means not trusted for any, according to my interpretation of
			// https://groups.google.com/forum/#!msg/mozilla.dev.tech.crypto/ZP3Kn84VBfA/_ozb5TvRLkcJ
			continue
		}
		trust := TrustLevel(0)
		if serverTrust == "CKT_NSS_TRUSTED_DELEGATOR" {
			trust |= ServerTrustedDelegator
		}
		if emailTrust == "CKT_NSS_TRUSTED_DELEGATOR" {
			trust |= EmailTrustedDelegator
		}
		if codeTrust == "CKT_NSS_TRUSTED_DELEGATOR" {
			trust |= CodeTrustedDelegator
		}
		if trust != 0 {
			trusted[obj["CKA_LABEL"]] = trust
		}
	}
	return trusted, nil
}

func contains(val string, set []string) bool {
	for _, v := range set {
		if v == val {
			return true
		}
	}
	return false
}
