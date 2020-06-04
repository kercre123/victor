// Copyright 2015 Gareth Watts
// Licensed under an MIT license
// See the LICENSE file for details

package certparse

import (
	"bytes"
	"reflect"
	"sort"
	"testing"

	"github.com/kr/pretty"
)

func testScanner(input string) *MozScanner {
	return NewMozScanner(bytes.NewReader([]byte(input)))
}

var valScanTestExpected = []MozValue{
	{Field: "FIELD_ONE", Type: "TYPE1", Value: ""},
	{Field: "FIELD_TWO", Type: "TYPE2", Value: "VALUE1"},
	{Field: "FIELD_THREE", Type: "MULTILINE_OCTAL", Value: "0N1\v0\t\x06\x03U\x04\x06\x13\x02US1\x100\x0e\x06\x03U\x04\n\x13\aEquifa"},
	{Field: "FIELD_FOUR", Type: "MULTILINE_UNKNOWN", Value: ""},
	{Field: "FIELD_FIVE", Type: "TYPE3", Value: "VALUE1 VALUE2"},
	{Field: "FIELD_SIX", Type: "UTF8", Value: "Yen ¥ sign"},
}

func TestValueScanOk(t *testing.T) {
	s := testScanner(testValueInput)
	var values []MozValue
	for s.ScanValue() {
		values = append(values, s.Value())
	}
	if err := s.ScanValueError(); err != nil {
		t.Fatal("Unexpected error", err)
	}
	if !reflect.DeepEqual(valScanTestExpected, values) {
		pretty.Println("expected", valScanTestExpected)
		pretty.Println("actual  ", values)
		t.Error("Did not receive expected values")
	}
}

func TestValueScanErr(t *testing.T) {
	s := testScanner("no begin\nline")
	var values []MozValue
	for s.ScanValue() {
		values = append(values, s.Value())
	}
	if err := s.ScanValueError(); err == nil {
		t.Error("Did not receive an error")
	}
	if len(values) != 0 {
		t.Error("Unexpected values parsed", values)
	}
}

var objScanExpectedFields = [][]string{
	{"CKA_CERTIFICATE_TYPE", "CKA_CLASS", "CKA_ID", "CKA_ISSUER", "CKA_LABEL",
		"CKA_MODIFIABLE", "CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_SUBJECT", "CKA_TOKEN", "CKA_VALUE"},
	{"CKA_CERT_MD5_HASH", "CKA_CERT_SHA1_HASH", "CKA_CLASS", "CKA_ISSUER", "CKA_LABEL",
		"CKA_MODIFIABLE", "CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_TOKEN", "CKA_TRUST_CODE_SIGNING",
		"CKA_TRUST_EMAIL_PROTECTION", "CKA_TRUST_SERVER_AUTH", "CKA_TRUST_STEP_UP_APPROVED"},
	{"CKA_CERTIFICATE_TYPE", "CKA_CLASS", "CKA_ID", "CKA_ISSUER", "CKA_LABEL", "CKA_MODIFIABLE",
		"CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_SUBJECT", "CKA_TOKEN", "CKA_VALUE"},
	{"CKA_CERT_MD5_HASH", "CKA_CERT_SHA1_HASH", "CKA_CLASS", "CKA_ISSUER", "CKA_LABEL",
		"CKA_MODIFIABLE", "CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_TOKEN", "CKA_TRUST_CODE_SIGNING",
		"CKA_TRUST_EMAIL_PROTECTION", "CKA_TRUST_SERVER_AUTH", "CKA_TRUST_STEP_UP_APPROVED"},
	{"CKA_CERTIFICATE_TYPE", "CKA_CLASS", "CKA_ID", "CKA_ISSUER", "CKA_LABEL", "CKA_MODIFIABLE",
		"CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_SUBJECT", "CKA_TOKEN", "CKA_VALUE"},
	{"CKA_CERT_MD5_HASH", "CKA_CERT_SHA1_HASH", "CKA_CLASS", "CKA_ISSUER", "CKA_LABEL",
		"CKA_MODIFIABLE", "CKA_PRIVATE", "CKA_SERIAL_NUMBER", "CKA_TOKEN", "CKA_TRUST_CODE_SIGNING",
		"CKA_TRUST_EMAIL_PROTECTION", "CKA_TRUST_SERVER_AUTH", "CKA_TRUST_STEP_UP_APPROVED"},
}

func keys(m map[string]string) (keys []string) {
	for k := range m {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	return keys
}

func TestObjectScanOk(t *testing.T) {
	s := testScanner(testCertInput)
	var objects []map[string]string
	for s.ScanObject() {
		objects = append(objects, s.Object())
	}
	if err := s.ScanObjectError(); err != nil {
		t.Fatal("Unepxected error", err)
	}
	for i, obj := range objects {
		keys := keys(obj)
		expected := objScanExpectedFields[i]
		if !reflect.DeepEqual(keys, expected) {
			t.Errorf("Keys mismatch for object %d", i)
		}
		for k, v := range obj {
			if len(v) == 0 {
				t.Errorf("object %d field %s has zero length value", i, k)
			}
		}
	}
	if len(objects) != 6 { // 2 objects for each of the 3 certs
		t.Fatalf("Incorrect number of objects.  Expected 6, got %d", len(objects))
	}
}

var readTrustedExpected = []struct {
	Label        string
	Organization string
	Trust        TrustLevel
}{
	{"Equifax Secure CA", "Equifax", ServerTrustedDelegator | EmailTrustedDelegator | CodeTrustedDelegator},
	{"Certinomis - Root CA", "Certinomis", ServerTrustedDelegator},
}

func TestReadTrustedCertsOk(t *testing.T) {
	f := bytes.NewReader([]byte(testCertInput))
	certs, err := ReadTrustedCerts(f)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	// should have certs for Equifax Secure CA and Certinomis only.
	if len(certs) != 2 {
		t.Fatal("Incorrect cert count", len(certs))
	}

	for i, cert := range certs {
		//pretty.Println(cert.Cert)
		expected := readTrustedExpected[i]
		if cert.Label != expected.Label {
			t.Errorf("cert %d label mismatch expected=%q actual=%q", i, expected.Label, cert.Label)
		}
		if cert.Cert.Issuer.Organization[0] != expected.Organization {
			t.Errorf("cert %d organization mismatch expected=%q actual=%q", i, expected.Organization, cert.Cert.Issuer.Organization[0])
		}
		if cert.Trust != expected.Trust {
			t.Errorf("cert%d trust mismatch expected=%#v actual=%#v", i, expected.Trust, cert.Trust)
		}
		if !reflect.DeepEqual(cert.Data, cert.Cert.Raw) {
			t.Errorf("cert %d raw data mismatch", i)
		}
	}

}
