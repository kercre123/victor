package apiclient

import (
	"bytes"
	"encoding/json"
	"errors"
	"io"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"
	"time"
)

// ----------------------------------------------------------------------
// API Requests
// ----------------------------------------------------------------------

type Request struct {
	*http.Request

	// AttemptCount tracks the number of times each request was
	// attempted. If there were no retries due to errors or timeouts,
	// AttemptCount will be 1.
	AttemptCount int
}

// RequestConfigs are functions which can configure an API request.
type RequestConfig func(r *Request) error

// WithJsonBody serializes an object to JSON to include as the HTTP
// request body.
func WithJsonBody(data interface{}) RequestConfig {
	return func(r *Request) error {
		j, err := json.Marshal(data)
		if err != nil {
			return err
		}
		reader := bytes.NewReader(j)
		r.Request.Body = ioutil.NopCloser(reader)
		r.Request.ContentLength = int64(reader.Len())
		r.Request.Header.Set("Content-Type", "application/json")
		return nil
	}
}

// WithFormBody serializes a set of URL values to to include as the
// HTTP request body.
func WithFormBody(data url.Values) RequestConfig {
	return func(r *Request) error {
		reader := strings.NewReader(data.Encode())
		r.Request.Body = ioutil.NopCloser(reader)
		r.Request.ContentLength = int64(reader.Len())
		r.Request.Header.Set("Content-Type", "application/x-www-form-urlencoded")
		return nil
	}
}

// WithReaderBody attaches an io.Reader to a Request.
func WithReaderBody(body io.Reader, contentType string) RequestConfig {
	return func(r *Request) error {
		if body == nil {
			return errors.New("WithReaderBody called with nil body")
		}
		r.Request.Header.Set("Content-Type", contentType)
		rc, ok := body.(io.ReadCloser)
		if !ok {
			rc = ioutil.NopCloser(body)
		}
		r.Request.Body = rc
		switch v := body.(type) {
		case *bytes.Buffer:
			r.Request.ContentLength = int64(v.Len())
		case *bytes.Reader:
			r.Request.ContentLength = int64(v.Len())
		case *strings.Reader:
			r.Request.ContentLength = int64(v.Len())
		}
		return nil
	}
}

// WithLocalTime sets the Local-Time header on API requests, for
// tracking clock skew.
func WithLocalTime() RequestConfig {
	return func(r *Request) error {
		r.Request.Header.Set("Local-Time", time.Now().Format(time.RFC1123Z))
		return nil
	}
}

// WithQuery adds a key/value pair to the request query string. The
// operation is additive - it does not remove or modify any key/value
// pairs that are already in the query string.
func WithQuery(key, value string) RequestConfig {
	return func(r *Request) error {
		q := r.Request.URL.Query()
		q.Add(key, value)
		r.Request.URL.RawQuery = q.Encode()
		return nil
	}
}

// WithQueryValues adds a complete set of query values to the URL.
// This replaces any existing query values.
func WithQueryValues(q url.Values) RequestConfig {
	return func(r *Request) error {
		r.Request.URL.RawQuery = q.Encode()
		return nil
	}
}

// WithHeader adds a single header to the request.
func WithHeader(key, val string) RequestConfig {
	return func(r *Request) error {
		r.Request.Header.Set(key, val)
		return nil
	}
}

func WithNoSession() RequestConfig {
	return func(r *Request) error {
		r.Request.Header.Del("Authorization")
		return nil
	}
}
