package svc

import (
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestPublicCertConfig_Prefix(t *testing.T) {
	cfg := PublicCertConfig{Prefix: "test"}
	assert.Equal(t, "test-public-cert", cfg.argname("cert"))
	assert.Equal(t, "[--test-public-cert|--test-public-certfile]", cfg.CommandSpec())
}

func TestPublicCertConfig_Load_Errors(t *testing.T) {
	cfg := PublicCertConfig{
		certData: ptr(""),
		certFile: ptr("testdata/nonexistent-cert"),
	}
	_, err := cfg.Load()
	require.Error(t, err)

	cfg = PublicCertConfig{
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = PublicCertConfig{
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
	}
	_, err = cfg.Load()
	require.Error(t, err)

}

func TestPublicCertConfig_Validation(t *testing.T) {
	cfg := PublicCertConfig{}
	_, err := cfg.Load()
	require.Error(t, err)

	cfg = PublicCertConfig{
		certData: ptr(""),
		certFile: ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = PublicCertConfig{
		certData: ptr("blah"),
		certFile: ptr("blah"),
	}
	_, err = cfg.Load()
	require.Error(t, err)

	cfg = PublicCertConfig{
		certData: ptr(""),
		certFile: ptr(""),
	}
	_, err = cfg.Load()
	require.Error(t, err)
}

func TestPublicCertConfig_Load_FromFiles(t *testing.T) {
	cfg := PublicCertConfig{Prefix: "test",
		certData: ptr(""),
		certFile: ptr("testdata/certificate.pem"),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}

func TestPublicCertConfig_Load_FromVars(t *testing.T) {
	certBlock, err := ioutil.ReadFile("testdata/certificate.pem")
	if err != nil {
		t.Fatalf("%s", err)
	}

	cfg := PublicCertConfig{Prefix: "test",
		certData: ptr(string(certBlock)),
		certFile: ptr(""),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}

func TestPublicCertConfig_Load_FromBase64Vars(t *testing.T) {
	certBlock, err := ioutil.ReadFile("testdata/certificate.b64")
	if err != nil {
		t.Fatalf("%s", err)
	}

	cfg := PublicCertConfig{Prefix: "test",
		certData: ptr(string(certBlock)),
		certFile: ptr(""),
	}
	cert, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, cert)
}
