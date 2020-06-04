package httpcapture

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
	"github.com/google/martian/har"
)

const timeFormat = "20060102-150405"

var serverIP = getIP()

// Recorder captures and records HTTP requests and responses.
type Recorder struct {
	id        string
	logger    *har.Logger
	store     Store
	reqNum    int64
	flushNum  int64
	autoflush bool
	metadata  Metadata
	fm        sync.RWMutex
	wg        sync.WaitGroup
}

// NewRecorder creates a Recorder instance.
// id represents the id that will be assigned to the blob that's created after
// a flush event.
// if autoflush is true, then a flush event will be triggered after every call to
// Record.  This may still cause multiple requests/responses to be included in
// in a single object, however, if they happen in close proximity.
func NewRecorder(id string, store Store, md Metadata, autoflush bool) *Recorder {
	logger := har.NewLogger()
	logger.SetOption(har.BodyLogging(true))
	logger.SetOption(har.PostDataLogging(true))

	if id == "" {
		rnd, err := strutil.ShortLowerUUID()
		if err != nil {
			panic("Failed to generate random id")
		}
		id = "srv-" + rnd
	}

	return &Recorder{
		id:        id,
		store:     store,
		logger:    logger,
		autoflush: autoflush,
		metadata:  md,
	}
}

// Wait waits for any in-progress flush operations to complete.
func (r *Recorder) Wait() {
	if r != nil {
		r.wg.Wait()
	}
}

// Flush triggers a write of currently buffered capture data to the
// backing store.
// The write itself is performed in a background goroutine.
func (r *Recorder) Flush(id string) {
	r.fm.Lock()
	defer r.fm.Unlock()
	r.flushNum++
	if id == "" {
		id = r.id + "-" + strconv.FormatInt(r.flushNum, 10)
	}

	data := r.logger.Export()
	if len(data.Log.Entries) == 0 {
		return
	}
	r.logger.Reset()
	md := r.metadata
	md.RequestId = id
	r.wg.Add(1)

	// Perform the actual upload in the background
	go func() {
		defer r.wg.Done()
		first := data.Log.Entries[0]

		// fill out remaining metadata
		md.ServerIP = serverIP
		md.RequestTime = first.StartedDateTime
		md.TotalTime = first.Time
		md.RequestCount = len(data.Log.Entries)

		var rids []string
		for _, entry := range data.Log.Entries {
			if entry.Response == nil {
				continue
			}
			for _, hd := range entry.Response.Headers {
				if hd.Name == "Anki-Request-Id" {
					rids = append(rids, hd.Value)
				}
			}
		}
		md.ReferencedRequestIds = rids

		jd, err := json.Marshal(data)
		if err != nil {
			alog.Error{"action": "httpcapture_encode", "status": "error", "error": err}.Log()
			return
		}

		start := time.Now()
		if err := r.store.StoreArchive(id, jd, md); err != nil {
			alog.Error{"action": "httpcapture_store", "status": "error", "error": err}.Log()
		} else {
			inf := alog.Info{"action": "httpcapture_store", "status": "ok",
				"id": id, "time_ms": int(time.Now().Sub(start) / time.Millisecond)}
			for i, entry := range data.Log.Entries {
				inf[fmt.Sprintf("url-%d", i)] = entry.Request.URL
			}
			inf.Log()
		}
	}()
}

func (r *Recorder) Record(req *http.Request, f func() (*http.Response, error)) (*http.Response, error) {
	resp, err := r.record(req, f)
	if r.autoflush {
		r.Flush("")
	}
	return resp, err
}

func (r *Recorder) record(req *http.Request, f func() (*http.Response, error)) (*http.Response, error) {
	r.fm.RLock()
	defer r.fm.RUnlock()

	rn := atomic.AddInt64(&r.reqNum, 1)
	id := r.id + "-" + strconv.FormatInt(rn, 10)
	r.logger.RecordRequest(id, req)
	resp, err := f()
	if err == nil {
		r.logger.RecordResponse(id, resp)
	}
	return resp, err
}
