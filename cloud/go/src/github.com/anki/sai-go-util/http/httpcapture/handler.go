package httpcapture

import (
	"net"
	"net/http"
	"net/http/httptest"
	"time"

	"github.com/gorilla/context"
)

// ContextKey is the key name used with a Gorilla request context
// to store a Recorder instance associated with the active request.
const ContextKey = "httpCapture"

// Handler is an HTTP handler for recording the request/response
// to .har archives.
type Handler struct {
	Handler     http.Handler
	Store       Store
	ServiceName string
}

// NewHandler returns a new capture Handler.
func NewHandler(h http.Handler, store Store, serviceName string) *Handler {
	return &Handler{Handler: h, Store: store, ServiceName: serviceName}
}

func (h *Handler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	// Ensure the hostname is in the URL as HAR requires it
	orgURL := r.URL
	r.URL.Scheme = "http"
	r.URL.Host = r.Host

	rid, ok := context.Get(r, "requestId").(string)
	if !ok {
		// passthru
		h.Handler.ServeHTTP(w, r)
		return
	}

	pt := &passthruRecorder{
		ResponseRecorder: httptest.NewRecorder(),
		rw:               w,
	}

	// Strip the port from the remote (client) address
	clientAddr, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		clientAddr = r.RemoteAddr
	}

	url := r.Host + r.URL.Path
	md := Metadata{
		ServiceName: h.ServiceName,
		ClientIP:    clientAddr,
		UserAgent:   r.Header.Get("User-Agent"),
		HTTPURL:     url,
	}

	recorder := NewRecorder(rid, h.Store, md, false)
	context.Set(r, ContextKey, recorder)

	recorder.Record(r, func() (*http.Response, error) {
		r.URL = orgURL
		h.Handler.ServeHTTP(pt, r)
		for k, v := range pt.ResponseRecorder.HeaderMap {
			w.Header()[k] = v
		}
		return pt.Result(), nil
	})
	recorder.Flush(rid)
}

// passthruRecorder wraps an httptest.ResponseRecorder
// so that data sent to the recorder is also relayed to an underlying
// http.ResponseWriter.
type passthruRecorder struct {
	*httptest.ResponseRecorder
	rw        http.ResponseWriter
	writeDone bool
}

func (pt *passthruRecorder) writeHeaders() {
	if pt.writeDone {
		return
	}
	pt.writeDone = true
	for k, v := range pt.ResponseRecorder.HeaderMap {
		pt.rw.Header()[k] = v
	}
}

func (pt *passthruRecorder) Write(buf []byte) (int, error) {
	pt.writeHeaders()
	n, err := pt.rw.Write(buf)
	if err != nil {
		return n, err
	}
	return pt.ResponseRecorder.Write(buf)
}

func (pt *passthruRecorder) WriteString(str string) (int, error) {
	pt.writeHeaders()
	n, err := pt.rw.Write([]byte(str))
	if err != nil {
		return n, err
	}
	return pt.ResponseRecorder.WriteString(str)
}

func (pt *passthruRecorder) WriteHeader(code int) {
	pt.writeHeaders()
	pt.rw.WriteHeader(code)
	pt.ResponseRecorder.WriteHeader(code)
}

type Metadata struct {
	RequestId            string    `json:"request_id,omitempty"`
	ServiceName          string    `json:"service_name,omitempty"`
	ReferencedRequestIds []string  `json:"ref_request_ids,omitempty"`
	RequestTime          time.Time `json:"request_time"`
	TotalTime            int64     `json:"total_time_ms"`
	RequestCount         int       `json:"request_count"`
	ClientIP             string    `json:"client_ip,omitempty"`
	UserAgent            string    `json:"user_agent,omitempty"`
	HTTPURL              string    `json:"http_url"`
	ServerIP             string    `json:"server_ip"`
}
