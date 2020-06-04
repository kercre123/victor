package apiclient

import "net/http"
import "github.com/anki/sai-go-util/log"

// LogIfAnyError generates an error level log message
// if err != nil or the response body contains an error.
func LogIfAnyError(action string, resp *Response, err error) (e *alog.Error) {
	if err != nil {
		return &alog.Error{"action": action, "status": "api_error", "error": err}
	} else if resp.StatusCode < 200 || resp.StatusCode >= 400 {
		jerr, err := resp.DecodeError()
		if err != nil {
			return &alog.Error{"action": action, "status": "api_response_decode_failed",
				"remote_request_id": resp.RequestID(), "error": err}
		}
		return &alog.Error{"action": action, "status": "api_response_error",
			"remote_request_id": resp.RequestID(), "err_": jerr}
	}
	return nil
}

// SetDefaultTransport adds a WithTransport configuration to
// DefaultClientConfig and returns a function that will revert
// DefaultClientConfig back to the original configuration.
//
// Intended for use by unit tests.
func SetDefaultTransport(t http.RoundTripper) func() {
	var orgConfig []Config
	for _, c := range DefaultClientConfig {
		orgConfig = append(orgConfig, c)
	}
	DefaultClientConfig = append(DefaultClientConfig, WithTransport(t))

	return func() {
		DefaultClientConfig = orgConfig
	}
}
