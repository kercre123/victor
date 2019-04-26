package apiclient

import (
	"errors"
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
	"time"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/strutil"
	"github.com/cenkalti/backoff"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestNew_WithServerURL(t *testing.T) {
	c, err := New("test", WithServerURL("http://server/"))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	assert.Equal(t, "http://server", c.serverUrl)

	c, err = New("test", WithServerURL("http://server"))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	assert.Equal(t, "http://server", c.serverUrl)
}

func TestNew_WithAuthKeys(t *testing.T) {
	c, err := New("test",
		WithAppKey("key"),
		WithSessionToken("session"))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	assert.Equal(t, "key", c.appKey)
	assert.Equal(t, "session", c.sessionToken)
}

func TestNew_WithTimeout(t *testing.T) {
	timeout := 5 * time.Second
	c, err := New("test", WithTimeout(timeout))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	assert.Equal(t, timeout, c.Client.Timeout)
}

func TestTransportInsecure(t *testing.T) {
	os.Setenv(InsecureClientEnvKey, "true")
	c, err := New("test", WithDefaultTransport())
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	require.NotNil(t, c.transport)
	tr, ok := c.transport.(*http.Transport)
	require.True(t, ok)
	require.NotNil(t, tr.TLSClientConfig)
	assert.True(t, tr.TLSClientConfig.InsecureSkipVerify)
}

func TestTransportNotInsecure(t *testing.T) {
	os.Unsetenv(InsecureClientEnvKey)
	c, err := New("test", WithDefaultTransport())
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}
	require.NotNil(t, c.transport)
	tr, ok := c.transport.(*http.Transport)
	require.True(t, ok)
	require.Nil(t, tr.TLSClientConfig)
}

func TestAuthentication(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.Equal(t, "Anki token", r.Header.Get(headerAuthorization))
		assert.Equal(t, "app", r.Header.Get(headerAppKey))
	}))
	defer ts.Close()

	c, err := New("test", WithServerURL(ts.URL), WithAppKey("app"), WithSessionToken("token"))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}

	req, err := c.NewRequest("GET", "/")
	if err != nil {
		t.Fatalf("Error creating request. %s", err)
	}
	_, err = c.Do(req)
	if err != nil {
		t.Errorf("Error executing request. %s", err)
	}
}

func TestClientID(t *testing.T) {
	id, err := strutil.Str(strutil.AllSafe, 19)
	if err != nil {
		t.Fatalf("Unable to create id. %s", err)
	}

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.Equal(t, id, r.Header.Get(headerClientId))
	}))
	defer ts.Close()

	c, err := New("test", WithServerURL(ts.URL), WithClientId(id))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}

	req, err := c.NewRequest("GET", "/")
	if err != nil {
		t.Fatalf("Error creating request. %s", err)
	}
	_, err = c.Do(req)
	if err != nil {
		t.Errorf("Error executing request. %s", err)
	}
	assert.Equal(t, 1, req.AttemptCount)
}

func TestUserAgent(t *testing.T) {
	userAgent, err := strutil.Str(strutil.AllSafe, 19)
	if err != nil {
		t.Fatalf("Unable to create user agent. %s", err)
	}

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.Equal(t, userAgent, r.Header.Get(headerUserAgent))
	}))
	defer ts.Close()

	c, err := New("test", WithServerURL(ts.URL), WithUserAgent(userAgent))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}

	req, err := c.NewRequest("GET", "/")
	if err != nil {
		t.Fatalf("Error creating request. %s", err)
	}
	_, err = c.Do(req)
	if err != nil {
		t.Errorf("Error executing request. %s", err)
	}
	assert.Equal(t, 1, req.AttemptCount)
}

func TestParseAgent(t *testing.T) {
	pairs := ParseAgent("Go-http-client/1.1 sai-api-client/1.0 sai-go-ankival/125a50c6899e")
	assert.Equal(t, 3, len(pairs))
	assert.Equal(t, "1.1", pairs["Go-http-client"])
	assert.Equal(t, "1.0", pairs["sai-api-client"])
	assert.Equal(t, "125a50c6899e", pairs["sai-go-ankival"])
}

func TestBuildInfoUserAgent(t *testing.T) {
	agent := BuildInfoUserAgent()
	pairs := ParseAgent(agent)
	assert.Equal(t, 3, len(pairs))
	assert.Equal(t, "1.1", pairs["Go-http-client"])
	assert.Equal(t, "1.0", pairs["sai-api-client"])

	sha, ok := pairs[buildinfo.Info().Product]
	assert.True(t, ok, "Product key not found in user-agent")
	assert.Equal(t, buildinfo.Info().Commit, sha)
}

func TestGetStatus(t *testing.T) {
	s := map[string]string{
		"branch":          "develop",
		"build_host":      "gobot",
		"build_time":      "2016-02-25T17:34:54Z",
		"build_time_unix": "1456421694",
		"build_user":      "teamcity",
		"commit":          "6d15cebdfa785b95fa37bd122c035e3ebbd49a46",
		"hostname":        "2567f0b0675a",
		"last_check":      "2016-03-03T23:06:10Z",
		"product":         "accounts",
		"status":          "ok",
	}
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		jw := &jsonutil.JsonWriterAdapter{ResponseWriter: w}
		if r.URL.Path != "/status" {
			t.Errorf("Wrong URL path from client. Expected /status, received %s", r.URL.Path)
			jw.JsonErrorData(r, http.StatusNotFound, nil)
		} else {
			jw.JsonOkData(r, s)
		}
	}))
	defer ts.Close()

	c, err := New("test", WithServerURL(ts.URL))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}

	status, err := c.GetStatus()
	if err != nil {
		t.Fatalf("Error fetching status. %s", err)
	}
	assert.Equal(t, s["branch"], status.Branch)
	assert.Equal(t, s["build_host"], status.BuildHost)
	assert.Equal(t, s["build_time"], status.BuildTime.Format(time.RFC3339))
	assert.Equal(t, s["build_time_unix"], status.BuildTimeUnix)
	assert.Equal(t, s["build_user"], status.BuildUser)
	assert.Equal(t, s["commit"], status.Commit)
	assert.Equal(t, s["hostname"], status.Hostname)
	assert.Equal(t, s["last_check"], status.LastCheck.Format(time.RFC3339))
	assert.Equal(t, s["product"], status.Product)
	assert.Equal(t, s["status"], status.Status)
}

func TestGetStatus404(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusNotFound)
	}))
	defer ts.Close()

	c, err := New("test", WithServerURL(ts.URL))
	if err != nil {
		t.Fatalf("Error creating client. %s", err)
	}

	_, err = c.GetStatus()
	if err == nil {
		t.Fatalf("No error fetching status.")
	}
}

type errorUntilCountExecutor struct {
	Target int
}

func (e *errorUntilCountExecutor) Do(r *Request) (*Response, error) {
	r.AttemptCount = r.AttemptCount + 1
	if r.AttemptCount == e.Target {
		return &Response{}, nil
	}
	return nil, errors.New("errorExecutor")
}

func WithErrorExecutor(target int) Config {
	return func(c *Client) error {
		c.executor = &errorUntilCountExecutor{
			Target: target,
		}
		return nil
	}
}

func TestRetry(t *testing.T) {
	const count = 5
	c, err := New("test",
		WithErrorExecutor(count),
		WithRetry(&backoff.ZeroBackOff{}))
	require.Nil(t, err)
	require.NotNil(t, c)

	req, err := c.NewRequest("GET", "/")
	require.Nil(t, err)
	require.NotNil(t, req)

	_, err = c.Do(req)
	assert.Nil(t, err)
	assert.Equal(t, count, req.AttemptCount)
}
