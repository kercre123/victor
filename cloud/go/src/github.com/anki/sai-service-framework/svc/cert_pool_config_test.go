package svc

import (
	"io/ioutil"
	"testing"

	"github.com/gwatts/rootcerts"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func boolPtr(value bool) *bool {
	return &value
}

func TestCertPoolConfig_Load(t *testing.T) {
	cfg := CertPoolConfig{
		certPoolFile: ptr("testdata/cert_pool_config_test_combined.pem"),
	}
	pool, err := cfg.Load()
	require.NoError(t, err)
	require.NotNil(t, pool)
	assert.Len(t, pool.Subjects(), 2)

	cfg = CertPoolConfig{
		certPoolFile: ptr("testdata/cert_pool_config_test_1_cert.pem"),
	}
	pool, err = cfg.Load()
	require.NoError(t, err)
	require.NotNil(t, pool)
	assert.Len(t, pool.Subjects(), 1)

	cfg = CertPoolConfig{
		certPoolFile:  ptr("testdata/cert_pool_config_test_combined.pem"),
		includeSystem: boolPtr(true),
	}
	pool, err = cfg.Load()
	require.NoError(t, err)
	require.NotNil(t, pool)
	assert.Len(t, pool.Subjects(), len(rootcerts.Certs())+2)
}

func TestCertPoolConfig_Load_Errors(t *testing.T) {
	cfg := CertPoolConfig{
		certPoolFile: ptr("testdata/key.pem"),
	}
	pool, err := cfg.Load()
	require.Error(t, err)
	assert.Nil(t, pool)

	cfg = CertPoolConfig{
		certPoolFile: ptr("testdata/non-existent-file"),
	}
	pool, err = cfg.Load()
	require.Error(t, err)
	assert.Nil(t, pool)

	cfg = CertPoolConfig{
		certPoolFile: ptr("testdata/empty"),
	}
	pool, err = cfg.Load()
	require.Error(t, err)
	assert.Nil(t, pool)

	_, err = (&CertPoolConfig{}).Load()
	require.Error(t, err)
}

func TestCertPoolConfig_LoadFromVar(t *testing.T) {
	certBlock, err := ioutil.ReadFile("testdata/cert_pool_config_test_combined.pem")
	if err != nil {
		t.Fatalf("%s", err)
	}

	cfg := CertPoolConfig{
		certPoolData: ptr(string(certBlock)),
	}
	pool, err := cfg.Load()
	require.NoError(t, err)
	require.NotNil(t, pool)
	assert.Len(t, pool.Subjects(), 2)
}
