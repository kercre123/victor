package jsonutil

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"strings"

	"github.com/gorilla/context"
)

var (
	BadJsonErrorResponse = errors.New("Invalid JSON error response")
)

// The JsonErrorResponse type represents an error message to be encoded
// as an HTTP text/json response
type JsonErrorResponse struct {
	HttpStatus int
	Code       string
	Message    string
	Metadata   map[string]interface{}
}

func (e JsonErrorResponse) Error() string {
	return fmt.Sprintf("%s: %s", e.Code, e.Message)
}

func (e JsonErrorResponse) JsonError() JsonErrorResponse {
	return e
}

func (e *JsonErrorResponse) UnmarshalJSON(data []byte) error {
	var ok bool
	metadata := make(map[string]interface{})
	if err := json.Unmarshal(data, &metadata); err != nil {
		return err
	}
	if e.Code, ok = metadata["code"].(string); !ok {
		return BadJsonErrorResponse
	}
	if e.Message, ok = metadata["message"].(string); !ok {
		return BadJsonErrorResponse
	}
	delete(metadata, "status")
	delete(metadata, "code")
	delete(metadata, "message")
	e.Metadata = metadata
	return nil
}

// Errors implementng the JsonError interface can be automatically encoded by
// a JsonWriter used by an HTTP handler.
type JsonError interface {
	error
	JsonError() JsonErrorResponse
}

// JsonWriter simplies writing JSON HTTP handlers by providing some methods
// for encoding JSON errors and responses.
type JsonWriter interface {
	// ResponseWriter interface
	Header() http.Header
	Write([]byte) (int, error)
	WriteHeader(int)

	StatusSet() bool
	StatusCode() int

	JsonOkData(r *http.Request, data interface{}) error
	JsonOk(r *http.Request, message string) error
	JsonOkEtagData(r *http.Request, etag string, data interface{}) error
	JsonError(r *http.Request, err JsonError) error
	JsonErrorData(r *http.Request, httpStatus int, data interface{}) error
	JsonErrorMessage(r *http.Request, httpStatus int, code, message string, metadata map[string]interface{}) error
	JsonServerError(r *http.Request) error
}

func getErrorId(r *http.Request) string {
	if r == nil {
		return ""
	}
	if requestId, ok := context.GetOk(r, "requestId"); ok {
		return requestId.(string)
	}
	return ""
}

type JsonWriterAdapter struct {
	http.ResponseWriter
	statusCode int
}

func (w *JsonWriterAdapter) WriteHeader(status int) {
	w.ResponseWriter.WriteHeader(status)
	w.statusCode = status
}

func (w *JsonWriterAdapter) StatusSet() bool {
	return w.statusCode != 0
}

func (w *JsonWriterAdapter) StatusCode() int {
	return w.statusCode
}

// JsonOKData takes any arbitary data structure and streams it to the client
// as text/json
func (w *JsonWriterAdapter) JsonOkData(r *http.Request, data interface{}) error {
	w.Header().Set("Content-Type", "text/json")
	w.WriteHeader(http.StatusOK)
	enc := json.NewEncoder(w)
	return enc.Encode(data)
}

func (w *JsonWriterAdapter) JsonOkEtagData(r *http.Request, etag string, data interface{}) error {
	w.Header().Set("Etag", etag)

	// Return a not modified response if the method is a GET or HEAD
	// and If-None-Match is set and is different to the etag, or set to "*"
	if inm := r.Header.Get("If-None-Match"); inm != "" && etag != "" {
		if (r.Method == "GET" || r.Method == "HEAD") && (inm == "*" || inm == etag) {
			// borrowed from net/http/fs.go
			h := w.Header()
			delete(h, "Content-Type")
			delete(h, "Content-Length")
			w.WriteHeader(http.StatusNotModified)
			return nil
		}
	}

	return w.JsonOkData(r, data)
}

type okResponse struct {
	Status  string `json:"status"`
	Message string `json:"message"`
}

// JsonOk sends a JSON encoded standard OK response with message string.
func (w *JsonWriterAdapter) JsonOk(r *http.Request, message string) error {
	enc := json.NewEncoder(w)
	w.Header().Set("Content-Type", "text/json")
	w.WriteHeader(http.StatusOK)
	return enc.Encode(okResponse{"OK", message})
}

// JsonError accepts an error and turns it into a JSON response.
//
// If err conforms to the JsonError interface then code and message
// will be set approropately
// TODO: handle the Metadata field
func (w *JsonWriterAdapter) JsonError(r *http.Request, err JsonError) error {
	var edata map[string]interface{}
	w.Header().Set("Content-Type", "text/json")
	e := err.JsonError()
	if e.HttpStatus > 0 {
		w.WriteHeader(e.HttpStatus)
	} else {
		w.WriteHeader(http.StatusInternalServerError)
	}
	edata = map[string]interface{}{
		"status":  "error",
		"code":    e.Code,
		"message": e.Message,
	}
	if errorId := getErrorId(r); errorId != "" {
		edata["error_id"] = errorId
	}
	for k, v := range e.Metadata {
		edata[k] = v
	}
	enc := json.NewEncoder(w)
	return enc.Encode(edata)
}

/*
func (w *JsonWriterAdapter) JsonError(r *http.Request, err error) error {
	var edata map[string]interface{}
	if jerr, ok := err.(JsonError); ok {
		e := jerr.JsonError()
		if e.HttpStatus > 0 {
			w.WriteHeader(e.HttpStatus)
		} else {
			w.WriteHeader(500)
		}
		edata = map[string]interface{}{
			"status":  "error",
			"code":    e.Code,
			"message": e.Message,
		}
		if errorId := getErrorId(r); errorId != "" {
			edata["error_id"] = errorId
		}
		for k, v := range e.Metadata {
			edata[k] = v
		}
	} else {
		w.WriteHeader(500)
		edata = map[string]interface{}{
			"status":  "error",
			"code":    "uncoded_error",
			"message": err.Error(),
		}
	}
	enc := json.NewEncoder(w)
	return enc.Encode(edata)
}
*/

// JsonErrorData accepts an HTTP status code and any arbitrary data structure
// and streams it to the client as text/json.
func (w *JsonWriterAdapter) JsonErrorData(r *http.Request, httpStatus int, data interface{}) error {
	w.Header().Set("Content-Type", "text/json")
	w.WriteHeader(httpStatus)
	enc := json.NewEncoder(w)
	return enc.Encode(data)
}

// JsonErrorMessage accepts an HTTP status code, text code and message and returns it
// to the client as a json object.
func (w *JsonWriterAdapter) JsonErrorMessage(r *http.Request, httpStatus int, code, message string, metadata map[string]interface{}) error {
	w.JsonError(r, JsonErrorResponse{
		HttpStatus: httpStatus,
		Code:       code,
		Message:    message,
		Metadata:   metadata,
	})
	return nil
}

func (w *JsonWriterAdapter) JsonServerError(r *http.Request) error {
	w.JsonError(r, JsonErrorResponse{
		HttpStatus: http.StatusInternalServerError,
		Code:       "server_failure",
		Message:    "An unexpected server error occurred",
	})
	return nil
}

func validateJsonRequest(r *http.Request) (jerr JsonError) {
	ct := strings.ToLower(r.Header.Get("Content-Type"))
	if !strings.HasPrefix(ct, "text/json") && !strings.HasPrefix(ct, "application/json") {
		return JsonErrorResponse{HttpStatus: http.StatusBadRequest, Code: "bad_content_type", Message: "Expected text/json content-type"}
	}
	return nil
}

// ReadRequestJsonLimit attempts to read a json request body (but does not decode it).
//
// limit specifies the maximum number of bytes to read from the request.
// Expects the Content-Type header to be text/json or application/json
//
// Returns an error implementing the JsonError interface in the event of error
func ReadRequestJsonLimit(r *http.Request, limit int) (data []byte, jerr JsonError) {
	var err error
	if jerr := validateJsonRequest(r); jerr != nil {
		return nil, jerr
	}
	if data, err = ioutil.ReadAll(io.LimitReader(r.Body, int64(limit+1))); err != nil {
		return nil, JsonErrorResponse{HttpStatus: http.StatusBadRequest, Code: "bad_input", Message: err.Error()}
	}
	if len(data) > limit {
		return nil, JsonErrorResponse{HttpStatus: http.StatusBadRequest, Code: "bad_input", Message: "Data exceeds size limit"}
	}
	return data, nil
}

func ParseRequestJsonLimit(r *http.Request, limit int, v interface{}) (jerr JsonError) {
	if jerr := validateJsonRequest(r); jerr != nil {
		return jerr
	}
	if err := json.NewDecoder(io.LimitReader(r.Body, int64(limit))).Decode(v); err != nil {
		return JsonErrorResponse{HttpStatus: http.StatusBadRequest, Code: "bad_input", Message: err.Error()}
	}
	return nil
}

type JsonHandlerFunc func(w JsonWriter, r *http.Request)

func (f JsonHandlerFunc) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	jw, _ := w.(JsonWriter)
	if jw == nil {
		jw = &JsonWriterAdapter{ResponseWriter: w}
	}
	f(jw, r)
	if !jw.StatusSet() {
		jw.JsonOk(r, "ok")
	}
}

func ToJsonWriter(w http.ResponseWriter) JsonWriter {
	jw, _ := w.(JsonWriter)
	if jw == nil {
		jw = &JsonWriterAdapter{ResponseWriter: w}
	}
	return jw
}

// JsonWriterRecorder provides an httptest.ResponseRecorder that
// complies with the JsonWriter interface.
type JsonWriterRecorder struct {
	*httptest.ResponseRecorder
	ErrorResponse interface{}
	OkResponse    interface{}
	ServerFailure bool
	statusCode    int
}

// Enforce at compile time that JsonWriterRecorder implements
// JsonWriter
var _ JsonWriter = &JsonWriterRecorder{}

func NewWriterRecorder() *JsonWriterRecorder {
	return &JsonWriterRecorder{
		ResponseRecorder: httptest.NewRecorder(),
	}
}

func (w *JsonWriterRecorder) WriteHeader(status int) {
	w.ResponseRecorder.WriteHeader(status)
	w.statusCode = status
}

func (w *JsonWriterRecorder) StatusSet() bool {
	return w.statusCode != 0
}

func (w *JsonWriterRecorder) StatusCode() int {
	return w.statusCode
}

func (w *JsonWriterRecorder) JsonOk(r *http.Request, message string) error {
	w.OkResponse = message
	w.WriteHeader(http.StatusOK)
	return nil
}

// Convert resposne to JSON and send to the writer
func (w *JsonWriterRecorder) JsonOkData(r *http.Request, data interface{}) error {
	w.OkResponse = data
	w.WriteHeader(http.StatusOK)
	return nil
}

func (w *JsonWriterRecorder) JsonOkEtagData(r *http.Request, etag string, data interface{}) error {
	w.Header().Set("Etag", etag)
	return w.JsonOkData(r, data)
}

func (w *JsonWriterRecorder) JsonError(r *http.Request, err JsonError) error {
	e := err.JsonError()
	w.ErrorResponse = err
	w.WriteHeader(e.HttpStatus)
	return nil
}

func (w *JsonWriterRecorder) JsonErrorData(r *http.Request, httpStatus int, data interface{}) error {
	w.WriteHeader(httpStatus)
	w.ErrorResponse = data
	return nil
}

func (w *JsonWriterRecorder) JsonErrorMessage(r *http.Request, httpStatus int, code, message string, metadata map[string]interface{}) error {
	w.WriteHeader(httpStatus)
	w.ErrorResponse = &JsonErrorResponse{HttpStatus: httpStatus, Code: code, Message: message, Metadata: metadata}
	return nil
}

func (w *JsonWriterRecorder) JsonServerError(r *http.Request) error {
	w.WriteHeader(http.StatusInternalServerError)
	w.ServerFailure = true
	return nil
}
