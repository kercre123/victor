package apiclient

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"mime"
	"net/http"
)

const (
	requestIDHeader = "Anki-Request-ID"
)

// ----------------------------------------------------------------------
// API Responses
// ----------------------------------------------------------------------

type Response struct {
	*http.Response
	err *APIError
}

// RequestID returns the request id header returned in the response.
func (r *Response) RequestID() string {
	return r.Header.Get(requestIDHeader)
}

// Data reads the HTTP response body and returns it as a raw byte
// array.
func (r *Response) Data() ([]byte, error) {
	defer r.Response.Body.Close()
	body, err := ioutil.ReadAll(r.Response.Body)
	if err != nil {
		return nil, err
	}
	return body, nil
}

func (r *Response) IsJson() bool {
	ct, _, err := mime.ParseMediaType(r.Header.Get("Content-Type"))
	if err != nil {
		return false
	}
	return ct == "text/json" || ct == "application/json"
}

// Json reads the HTTP response body and parses it into a generic Json
// structure.
func (r *Response) Json() (Json, error) {
	raw, err := r.Data()
	if err != nil {
		return nil, err
	}

	var data Json
	err = json.Unmarshal(raw, &data)
	return data, err
}

// UnmarshalJson reqads the HTTP response body and parses it into a
// caller-supplied struct.
func (r *Response) UnmarshalJson(v interface{}) error {
	raw, err := r.Data()
	if err != nil {
		return err
	}

	err = json.Unmarshal(raw, v)
	if err != nil {
		return err
	}
	return nil
}

// DecodeError attempts to decode a standard JSON error response as generated
// by the jsonutil package.
func (r *Response) DecodeError() (e *APIError, err error) {
	if r.err != nil {
		return r.err, nil
	}
	raw, err := r.Data()
	if err != nil {
		return nil, err
	}

	if jerr := json.Unmarshal(raw, &e); jerr != nil {
		return nil, fmt.Errorf("JSON decode failed: %v - original body: %q", jerr, string(raw))
	}
	r.err = e
	return e, nil
}

// IsErrorCode returns true and the corresponding error if the response
// is a service-standard JSON error response and that response has
// its code set to the supplied value.
func (r *Response) IsErrorCode(code string) (bool, *APIError) {
	if r == nil || (r.StatusCode >= 200 && r.StatusCode <= 299) {
		// not an error
		return false, nil
	}
	jerr, err := r.DecodeError()
	if err != nil {
		// some other type of error
		return false, nil
	}
	if jerr.Code == code {
		return true, jerr
	}
	return false, nil
}

type APIError struct {
	Status   string `json:"status"`
	Code     string `json:"code"`
	Message  string `json:"message"`
	ErrorID  string `json:"error_id"`
	Metadata map[string]interface{}
}

func (e APIError) String() string {
	return fmt.Sprintf("err_id=%s code=%s", e.ErrorID, e.Code)
}

func (e APIError) Error() string {
	return e.String()
}

// Loggable interface
func (e APIError) ToLogValues() map[string]interface{} {
	return map[string]interface{}{
		"status":   e.Status,
		"code":     e.Code,
		"message":  e.Message,
		"error_id": e.ErrorID,
		"metadata": e.Metadata,
	}
}
