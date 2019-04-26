package apiclient

import (
	"crypto/tls"
	"fmt"
	"io"
	"net/http"
	"net/http/httputil"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/http/httpcapture"
	"github.com/anki/sai-go-util/http/metrics/metricsclient"
	"github.com/cenkalti/backoff"
)

// ----------------------------------------------------------------------
// Base API Client
// ----------------------------------------------------------------------

const (
	headerAppKey        = "Anki-App-Key"
	headerClientId      = "Anki-Client-Id"
	headerAuthorization = "Authorization"
	headerUserAgent     = "User-Agent"
	authorizationPrefix = "Anki "

	ApiUserAgent         = "sai-api-client/1.0"
	InsecureClientEnvKey = "HTTP_INSECURE_CLIENT"
)

var (
	// DefaultClientConfig supplies the default configuration used by
	// New.  Additional Config options supplied to New may override these.
	DefaultClientConfig = []Config{
		WithSimpleExecutor(),
		WithDefaultTransport(),
	}
)

type Executor interface {
	Do(r *Request) (*Response, error)
}

// Client is the base type for Anki API clients. It handles common
// tasks such as constructed requests correctly with Anki
// authentication tokens in the right headers.
type Client struct {
	// client is the underlying HTTP client with metrics
	Client *http.Client

	// executor what is invoked to execute an HTTP request.
	executor Executor

	// transport is passed to metricclient
	transport    http.RoundTripper
	timeout      time.Duration
	serverUrl    string
	appKey       string
	sessionToken string
	clientId     string
	noCapture    bool
	baseRequest  *http.Request
	debugFlags   int
	debugWriter  io.Writer
	userAgent    string
}

// Status represents the information returned from the /status
// endpoint exposed by our standard API framework.
type Status struct {
	Product       string    `json:"product"`
	Hostname      string    `json:"hostname"`
	Status        string    `json:"status"`
	LastCheck     time.Time `json:"last_check"`
	Branch        string    `json:"branch"`
	Commit        string    `json:"commit"`
	BuildTimeUnix string    `json:"build_time_unix"`
	BuildTime     time.Time `json:"build_time"`
	BuildHost     string    `json:"build_host"`
	BuildUser     string    `json:"build_user"`
}

// Configs are functions which can configure the client object.
type Config func(c *Client) error

// WithServerURL sets the base URL to use for all API calls.
func WithServerURL(url string) Config {
	return func(c *Client) error {
		c.serverUrl = strings.TrimSuffix(url, "/")
		return nil
	}
}

// WithAppKey sets the Anki app key to authenticate all API calls
// with.
func WithAppKey(key string) Config {
	return func(c *Client) error {
		c.appKey = key
		return nil
	}
}

// WithSessionToken sets the session token to authenticate all API
// calls with.
func WithSessionToken(token string) Config {
	return func(c *Client) error {
		c.sessionToken = token
		return nil
	}
}

// WithClientId sets the optional unique client ID used to identify
// this client in the Anki-Client-Id request header.
func WithClientId(id string) Config {
	return func(c *Client) error {
		c.clientId = id
		return nil
	}
}

// WithoutCapture disables sending request/response data
// to the capture store.
func WithoutCapture() Config {
	return func(c *Client) error {
		c.noCapture = true
		return nil
	}
}

// WithBaseRequest associates the client with an originating HTTP request.
// This is used by the http capture library to associate outbound HTTP
// requests with the server received request.
func WithBaseRequest(r *http.Request) Config {
	return func(c *Client) error {
		c.baseRequest = r
		return nil
	}
}

// WithTimeout configures the timeout duration used by the underlying
// HTTP client.
func WithTimeout(t time.Duration) Config {
	return func(c *Client) error {
		c.timeout = t
		return nil
	}
}

// WithUserAgent configures the full string to use in the User-Agent
// header for all requests.
func WithUserAgent(id string) Config {
	return func(c *Client) error {
		c.userAgent = id
		return nil
	}
}

// WithBuildInfoUserAgent configures the client to use the user agent
// string built by BuildInfoUserAgent().
func WithBuildInfoUserAgent() Config {
	return WithUserAgent(BuildInfoUserAgent())
}

// BuildInfoUserAgent constructs a user agent string identifying the
// Anki Go service making the call.
func BuildInfoUserAgent() string {
	// Unfortunately, there is no way to obtain this from the Go std
	// library without actually *sending* (not just constructing) an
	// HTTP request. See https://golang.org/src/net/http/request.go#L411
	const defaultUserAgent = "Go-http-client/1.1"

	info := buildinfo.Info()
	return fmt.Sprintf("%s %s %s/%s",
		defaultUserAgent,
		ApiUserAgent,
		info.Product,
		info.Commit)
}

// ParseAgent splits standard User-Agent strings composed of
// /-delimited tuples separated by spaces into a map.
func ParseAgent(userAgent string) map[string]string {
	tuples := strings.Split(userAgent, " ")
	values := map[string]string{}
	for _, tuple := range tuples {
		pair := strings.Split(tuple, "/")
		if len(pair) < 2 {
			values[pair[0]] = ""
		} else {
			values[pair[0]] = pair[1]
		}
	}
	return values
}

// Bit flags OR'd together to determine what parts of a client
// request or response should be printed when passed to WithDebug.
const (
	RequestHeaders = 1 << iota
	RequestBody
	ResponseHeaders
	ResponseBody
)

// WithDebug configures the printing of request and/or response data for
// client requests.
//
// flags consists of any of the RequestHeaders, RequestBody, ResponseHeaders
// and ResponseBody flags OR'd together.
func WithDebug(w io.Writer, flags int) Config {
	return func(c *Client) error {
		c.debugWriter = w
		c.debugFlags = flags
		return nil
	}
}

// WithSimpleExecutor configures the API client to call the go HTTP
// client directly, with no automatic retry (this is the default).
func WithSimpleExecutor() Config {
	return func(c *Client) error {
		c.executor = &simpleExecutor{
			client: c,
		}
		return nil
	}
}

// WithRetry configures the API client to automatically retry failed
// HTTP requests, with retry policies configured by the
// backoff.BackOff object.
func WithRetry(backoff backoff.BackOff) Config {
	return func(c *Client) error {
		c.executor = &retryExecutor{
			executor: c.executor,
			backoff:  backoff,
		}
		return nil
	}
}

func WithDefaultRetry() Config {
	return WithRetry(&backoff.ExponentialBackOff{
		InitialInterval:     100 * time.Millisecond,
		RandomizationFactor: backoff.DefaultRandomizationFactor,
		Multiplier:          1.25,
		MaxInterval:         5 * time.Second,
		MaxElapsedTime:      60 * time.Second,
		Clock:               backoff.SystemClock,
	})
}

// WithTransport uses the supplied transport as the base transport
// supplied to the metrics client.
func WithTransport(rt http.RoundTripper) Config {
	return func(c *Client) error {
		c.transport = rt
		return nil
	}
}

// WithInsecureTransport generates an HTTP transport with TLS certificate
// verification disabled.  Should only be used in trusted test environments.
func WithInsecureTransport() Config {
	return func(c *Client) error {
		c.transport = &http.Transport{
			TLSClientConfig: &tls.Config{
				InsecureSkipVerify: true,
			},
			Proxy: http.ProxyFromEnvironment,
		}
		return nil
	}
}

// WithDefaultTransport calls either WithTransport(http.DefaultTransport) or
// WithInsecureTransport if the HTTP_INSECURE_TRANSPORT environment variable
// is set to true.
// This is the default transport behavior if no other transport configuration
// is supplied.
func WithDefaultTransport() Config {
	return func(c *Client) error {
		insecure, _ := strconv.ParseBool(os.Getenv(InsecureClientEnvKey))
		if insecure {
			WithInsecureTransport()(c)
		} else {
			WithTransport(http.DefaultTransport)(c)
		}
		return nil
	}
}

// New creates and configures a new base client object. Unless
// overriden, the client will be configured with the simple executor
// and build info user-agent.
func New(prefix string, config ...Config) (*Client, error) {
	c := new(Client)

	// apply the default configuration, if any
	if err := c.applyConfig(DefaultClientConfig); err != nil {
		return nil, err
	}

	// override with passed-in configuration.
	if err := c.applyConfig(config); err != nil {
		return nil, err
	}

	mc, err := metricsclient.New(prefix, metricsclient.WithTransport(c.transport))
	if err != nil {
		return nil, err
	}

	c.Client = mc
	if c.timeout > 0 {
		c.Client.Timeout = c.timeout
	}

	if !c.noCapture {
		c.Client = httpcapture.NewClient(c.Client, c.baseRequest)
	}

	return c, nil
}

func (c *Client) applyConfig(cfgs []Config) error {
	for _, cfg := range cfgs {
		if err := cfg(c); err != nil {
			return err
		}
	}
	return nil
}

// NewRequest creates and configures a new API request.
func (c *Client) NewRequest(method, path string, config ...RequestConfig) (*Request, error) {
	if len(path) == 0 {
		path = "/"
	}
	if path[0] != '/' {
		path = "/" + path
	}
	p := c.serverUrl + path
	r, err := http.NewRequest(method, p, nil)
	if err != nil {
		return nil, err
	}

	req := &Request{Request: r}

	if c.appKey != "" {
		r.Header.Set(headerAppKey, c.appKey)
	}

	if c.sessionToken != "" {
		r.Header.Set(headerAuthorization, authorizationPrefix+c.sessionToken)
	}

	if c.clientId != "" {
		r.Header.Set(headerClientId, c.clientId)
	}

	if c.userAgent != "" {
		r.Header.Set(headerUserAgent, c.userAgent)
	}

	for _, cfg := range config {
		if err := cfg(req); err != nil {
			return nil, err
		}
	}

	return req, nil
}

// Do executes an API request synchronously and returns the response.
func (c *Client) Do(r *Request) (*Response, error) {
	c.debugRequest(r)

	resp, err := c.executor.Do(r)

	if resp != nil {
		c.debugResponse(resp)
	}

	return resp, err
}

// GetStatus fetches the API status information from the common
// /status endpoint exposed by all Anki APIs.
func (c *Client) GetStatus() (*Status, error) {
	req, err := c.NewRequest("GET", "/status")
	if err != nil {
		return nil, err
	}

	rsp, err := c.Do(req)
	if err != nil {
		return nil, err
	}

	var s Status
	err = rsp.UnmarshalJson(&s)
	if err != nil {
		return nil, err
	}
	return &s, nil
}

func (c *Client) debugRequest(r *Request) {
	if (c.debugFlags & (RequestHeaders | RequestBody)) != 0 {
		fmt.Fprintln(c.debugWriter, "--- REQUEST START ---")
		if body, err := httputil.DumpRequestOut(r.Request, c.debugFlags&RequestBody != 0); err == nil {
			c.debugWriter.Write(body)
		}
		fmt.Fprintln(c.debugWriter, "--- REQUEST END ---")
	}
}

func (c *Client) debugResponse(r *Response) {
	if (c.debugFlags & (ResponseHeaders | ResponseBody)) != 0 {
		fmt.Fprintln(c.debugWriter, "--- RESPONSE START ---")
		if body, err := httputil.DumpResponse(r.Response, c.debugFlags&ResponseBody != 0); err == nil {
			c.debugWriter.Write(body)
		}
		fmt.Fprintln(c.debugWriter, "--- RESPONSE END ---")
	}
}

// ----------------------------------------------------------------------
// Synchronous Executor
// ----------------------------------------------------------------------

type simpleExecutor struct {
	client *Client
}

// Do executes an API request synchronously and returns the response.
func (e *simpleExecutor) Do(r *Request) (*Response, error) {
	r.AttemptCount = r.AttemptCount + 1
	rsp, err := e.client.Client.Do(r.Request)
	if err != nil {
		return nil, err
	}
	return &Response{Response: rsp}, nil
}

// ----------------------------------------------------------------------
// Retry
// ----------------------------------------------------------------------

type retryExecutor struct {
	executor Executor
	backoff  backoff.BackOff
}

func (e *retryExecutor) Do(r *Request) (rsp *Response, err error) {
	retryErr := backoff.Retry(func() error {
		rsp, err = e.executor.Do(r)
		return err
	}, e.backoff)
	if retryErr != nil {
		return nil, retryErr
	}
	return rsp, err
}
