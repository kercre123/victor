package svc

import (
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestCertConfig_Prefix(t *testing.T) {
	cfg := CertConfig{Prefix: "test"}
	assert.Equal(t, "test-cert", cfg.argname("cert"))
	assert.Equal(t, "[--test-cert|--test-certfile] [--test-key|--test-keyfile] [--test-server-name-override]", cfg.CommandSpec())
}

func ptr(val string) *string {
	return &val
}

func TestCertConfig_Load_Errors(t *testing.T) {
	cfg := CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		certFile: ptr("testdata/nonexistent-cert"),
		keyData:  ptr(""),
		keyFile:  ptr("testdata/key.pem"),
	}
	_, err := cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
		keyData:  ptr(""),
		keyFile:  ptr("testdata/nonexistent-key"),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
		keyData:  ptr("nah, this ain't a key"),
		keyFile:  ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)

}

func TestCertConfig_Validation(t *testing.T) {
	cfg := CertConfig{}
	_, err := cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "",
		certData: ptr(""),
		certFile: ptr(""),
		keyData:  ptr(""),
		keyFile:  ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "",
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
		keyData:  ptr(""),
		keyFile:  ptr("testdata/key.pem"),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		certFile: ptr(""),
		keyData:  ptr(""),
		keyFile:  ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "test",
		certData: ptr("blah"),
		certFile: ptr("blah"),
		keyData:  ptr(""),
		keyFile:  ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		certFile: ptr(""),
		keyData:  ptr("blah"),
		keyFile:  ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)
}

func TestCertConfig_Load_FromFiles(t *testing.T) {
	cfg := CertConfig{
		Prefix:   "test",
		certData: ptr(""),
		keyData:  ptr(""),
		certFile: ptr("testdata/certificate.pem"),
		keyFile:  ptr("testdata/key.pem"),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}

func TestCertConfig_Load_FromVars(t *testing.T) {
	certBlock, err := ioutil.ReadFile("testdata/certificate.pem")
	if err != nil {
		t.Fatalf("%s", err)
	}
	keyBlock, err := ioutil.ReadFile("testdata/key.pem")
	if err != nil {
		t.Fatalf("%s", err)
	}

	cfg := CertConfig{
		Prefix:   "test",
		certData: ptr(string(certBlock)),
		keyData:  ptr(string(keyBlock)),
		certFile: ptr(""),
		keyFile:  ptr(""),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}

func TestCertConfig_Load_FromBase64Vars(t *testing.T) {
	certBlock, err := ioutil.ReadFile("testdata/certificate.b64")
	if err != nil {
		t.Fatalf("%s", err)
	}
	keyBlock, err := ioutil.ReadFile("testdata/key.b64")
	if err != nil {
		t.Fatalf("%s", err)
	}

	cfg := CertConfig{
		Prefix:   "test",
		certData: ptr(string(certBlock)),
		keyData:  ptr(string(keyBlock)),
		certFile: ptr(""),
		keyFile:  ptr(""),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}
