package rootcerts

import (
	"crypto/tls"
	"net/http"
	"testing"
)

// Some tests to make sure the generated .go code is sane.

func TestCertsByTrust(t *testing.T) {
	certs := CertsByTrust(EmailTrustedDelegator | ServerTrustedDelegator)
	if len(certs) < 5 {
		t.Fatal("Unexpectedly few matching certificates")
	}
	for _, c := range certs {
		if c.Trust&EmailTrustedDelegator == 0 {
			t.Errorf("Cert %q had incorrect trust level %#v", c.Label, c.Trust)
		}
		if c.Trust&ServerTrustedDelegator == 0 {
			t.Errorf("Cert %q had incorrect trust level %#v", c.Label, c.Trust)
		}
	}
}

func TestServerCertPoolOK(t *testing.T) {
	cp := ServerCertPool()
	sslCerts := CertsByTrust(ServerTrustedDelegator)
	if len(sslCerts) < 100 {
		t.Fatal("Unexpected few server certificates", len(sslCerts))
	}
	if len(sslCerts) != len(cp.Subjects()) {
		t.Fatalf("Incorrect cert count.  expected=%d actual=%d", len(sslCerts), len(cp.Subjects()))
	}
}

func testTransport(t *testing.T, testName string) {
	dt := http.DefaultTransport.(*http.Transport)
	if dt.TLSClientConfig == nil {
		t.Fatal("TLS client config not created")
	}
	if dt.TLSClientConfig.RootCAs == nil {
		t.Fatal("Root CAs not set")
	}
	if len(dt.TLSClientConfig.RootCAs.Subjects()) != len(ServerCertPool().Subjects()) {
		t.Error("Incorrect cert count in ca pool")
	}
}

func TestUpdateDefaultTransportNilConfig(t *testing.T) {
	dt := http.DefaultTransport.(*http.Transport)
	dt.TLSClientConfig = nil
	err := UpdateDefaultTransport()
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	testTransport(t, "nilconfig")
}

func TestUpdateDefaultTransportNewConfig(t *testing.T) {
	dt := http.DefaultTransport.(*http.Transport)
	dt.TLSClientConfig = &tls.Config{ServerName: "set-by-test"} // so we know it wasn't replaced
	err := UpdateDefaultTransport()
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	testTransport(t, "newconfig")
	if dt.TLSClientConfig.ServerName != "set-by-test" {
		t.Fatal("tls config was replaced")
	}
}

type fakeTransport struct{}

func (ft *fakeTransport) RoundTrip(r *http.Request) (*http.Response, error) { return nil, nil }

func TestUpdateDefaultTransportNotTransport(t *testing.T) {
	dt := http.DefaultTransport.(*http.Transport)
	defer func() { http.DefaultTransport = dt }()
	http.DefaultTransport = &fakeTransport{}
	if err := UpdateDefaultTransport(); err == nil {
		t.Fatal("Didn't get expected error")
	}
}

func TestUpdateDefaultTransportNilTransport(t *testing.T) {
	dt := http.DefaultTransport.(*http.Transport)
	defer func() { http.DefaultTransport = dt }()
	http.DefaultTransport = nil
	if err := UpdateDefaultTransport(); err == nil {
		t.Fatal("Didn't get expected error")
	}
}
