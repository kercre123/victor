package wavefront

import (
	"bytes"
	"errors"
	"io"
	"io/ioutil"
	"net/http"
	"time"
)

var (
	client      = &http.Client{Timeout: time.Second * 10}
	reportError = errors.New("error: invalid format or points")
)

const (
	contentType    = "Content-Type"
	authzHeader    = "Authorization"
	bearer         = "Bearer "
	textPlain      = "text/plain"
	reportEndpoint = "/report"
	formatKey      = "f"
)

// DirectReporter is an interface representing the ability to report points to a Wavefront service.
type Reporter interface {
	Report(format string, pointLines string) (*http.Response, error)
	Server() string
}

// The implementation of a DirectReporter that reports points directly to a Wavefront server.
type directReporter struct {
	serverURL string
	token     string
}

func NewDirectReporter(server string, token string) Reporter {
	return &directReporter{serverURL: server, token: token}
}

func (reporter directReporter) Report(format string, pointLines string) (*http.Response, error) {
	if format == "" || pointLines == "" {
		return nil, reportError
	}

	apiURL := reporter.serverURL + reportEndpoint
	req, err := http.NewRequest("POST", apiURL, bytes.NewBufferString(pointLines))
	req.Header.Set(contentType, textPlain)
	req.Header.Set(authzHeader, bearer+reporter.token)
	if err != nil {
		return &http.Response{}, err
	}

	q := req.URL.Query()
	q.Add(formatKey, format)
	req.URL.RawQuery = q.Encode()

	resp, err := client.Do(req)
	if err != nil {
		return resp, err
	}
	io.Copy(ioutil.Discard, resp.Body)
	defer resp.Body.Close()
	return resp, nil
}

func (reporter directReporter) Server() string {
	return reporter.serverURL
}
