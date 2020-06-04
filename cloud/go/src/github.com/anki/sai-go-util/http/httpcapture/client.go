package httpcapture

import (
	"net/http"
	"os"

	"github.com/anki/sai-go-util/strutil"
	"github.com/gorilla/context"
)

// DefaultRecorder is used to record requests without an associated request id
// (i.e. system requests).
var DefaultRecorder *Recorder

// SetDefaultRecorder sets the capture recorder to use for system http requests
// serviceName should be something like "sai-go-storegate"; it defaults to
// the program name and is used to construct object ids for the blob store.
func SetDefaultRecorder(serviceName string, store Store) {
	if serviceName == "" {
		serviceName = os.Args[0]
	}
	md := Metadata{
		ServiceName: serviceName,
		HTTPURL:     "http://localhost",
	}
	rnd, err := strutil.ShortLowerUUID()
	if err != nil {
		panic("Failed to generate random id")
	}
	id := "srv-" + rnd
	DefaultRecorder = NewRecorder(id, store, md, true)
}

// NewClient wraps an http.Client with one that will record the request and responses
// to a .har archive.
//
// If archiving is disabled, the original http.Client will be returned.
func NewClient(client *http.Client, r *http.Request) *http.Client {
	rec, ok := context.Get(r, ContextKey).(*Recorder)
	if !ok {
		rec = DefaultRecorder
	}
	if rec == nil {
		return client
	}

	newt := &perRequestTransport{
		rt:       client.Transport,
		recorder: rec,
	}

	return &http.Client{
		Transport:     newt,
		CheckRedirect: client.CheckRedirect,
		Jar:           client.Jar,
		Timeout:       client.Timeout,
	}
}

type perRequestTransport struct {
	rt       http.RoundTripper
	recorder *Recorder
}

func (t perRequestTransport) RoundTrip(req *http.Request) (*http.Response, error) {
	tp := t.rt
	if tp == nil {
		tp = http.DefaultTransport
	}

	return t.recorder.Record(req, func() (*http.Response, error) {
		return tp.RoundTrip(req)
	})
}
