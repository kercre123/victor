package api

import (
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"strings"
	"time"

	"github.com/anki/sai-blobstore/apierrors"
	"github.com/anki/sai-blobstore/blobstore"
	"github.com/anki/sai-blobstore/db"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/gorilla/context"
	"github.com/gorilla/mux"
)

const (
	// We expect user supplied metadata to begin with this prefix
	userMetadataPrefix = "Usr-"

	mdRequestId   = "Srv-Request-Id"
	mdUploaderIP  = "Srv-Uploader-Ip"
	mdContentSize = "Srv-Blob-Size"
	mdUploadTime  = "Srv-Upload-Time"
	mdBlobId      = "Srv-Blob-Id"
)

var (
	handlers []apirouter.Handler
	Store    *blobstore.Store
)

func defineHandler(handler apirouter.Handler) {
	handlers = append(handlers, handler)
}

// RegisterAPI causes the session API handles to be registered with the http mux.
func RegisterAPI() {
	apirouter.RegisterHandlers(handlers)
}

func requireAnkiPerson(h http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		const action = "validate_anki_person"

		us := validate.ActiveSession(r)
		if !us.User.EmailIsVerified || !strings.HasSuffix(*us.User.Email, "@anki.com") {
			alog.Warn{"action": action, "status": "denied", "user_id": us.User.Id}.Log()
			jsonutil.ToJsonWriter(w).JsonError(r, apierrors.InvalidSessionPerms)
			return
		}
		h.ServeHTTP(w, r)
	})
}

// Register the Upload endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "PUT",
		Path:   "/1/{namespace}/blobs/{blob_id}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.ScopeMask(session.ValidSessionScope),
			Handler:          jsonutil.JsonHandlerFunc(Upload),
		},
	})
}

// Upload uploads a new blob to the store.
//
// PUT /1/:namespace/blobs/:blob_id
func Upload(w jsonutil.JsonWriter, r *http.Request) {
	const action = "upload"
	ns := mux.Vars(r)["namespace"]
	id := mux.Vars(r)["blob_id"]

	metadata, err := reqToMetadata(r, ns, id)
	if err != nil {
		handleError(w, r, action, err)
	}
	size, err := Store.StoreItem(ns, id, r.Body, metadata)
	if err != nil {
		handleError(w, r, action, err)
		return
	}

	alog.Info{"action": action, "status": "ok", "namespace": ns,
		"blob_id": id, "size": size}.LogR(r)

	w.JsonOkData(r, map[string]string{
		"status": "ok",
	})
}

// Register the Download endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/{namespace}/blobs/{blob_id}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ScopeMask(session.ValidSessionScope),
			Handler:          requireAnkiPerson(jsonutil.JsonHandlerFunc(Download)),
		},
	})
}

// Download a blob and its metadata
//
// GET /1/:namespace/blobs/:blob_id
func Download(w jsonutil.JsonWriter, r *http.Request) {
	const action = "download"
	ns := mux.Vars(r)["namespace"]
	id := mux.Vars(r)["blob_id"]

	metadata, err := Store.FetchItemMetadata(ns, id)
	if err != nil {
		handleError(w, r, action, err)
		return
	}

	blob, err := Store.FetchItemData(ns, id)
	if err != nil {
		handleError(w, r, action, err)
		return
	}
	defer blob.Body.Close()

	metadataToHeaders(metadata, w)
	// 2016-09-07 Content-Length is still incorrectly calculated downstream
	// for gzipped data
	// w.Header().Set("Content-Length", strconv.FormatInt(blob.Size, 10))

	// Write explicit status to notify JsonWriter that we're handling the response
	w.WriteHeader(http.StatusOK)

	// Stream result
	n, err := io.Copy(w, blob.Body)
	if err != nil {
		alog.Warn{"action": action, "status": "copy_failed", "namespace": ns,
			"blob_id": id, "size": blob.Size, "error": err}.LogR(r)
	}

	alog.Info{"action": action, "status": "ok", "namespace": ns,
		"blob_id": id, "size": blob.Size, "sent": n}.LogR(r)

}

// Register the Head/info endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "HEAD",
		Path:   "/1/{namespace}/blobs/{blob_id}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ScopeMask(session.ValidSessionScope),
			Handler:          requireAnkiPerson(jsonutil.JsonHandlerFunc(Head)),
		},
	})
}

// Retrieve blob metadata only
//
// HEAD /1/:namespace/blobs/:blob_id
func Head(w jsonutil.JsonWriter, r *http.Request) {
	const action = "head"
	ns := mux.Vars(r)["namespace"]
	id := mux.Vars(r)["blob_id"]

	metadata, err := Store.FetchItemMetadata(ns, id)
	if err != nil {
		handleError(w, r, action, err)
		return
	}

	metadataToHeaders(metadata, w)
	w.Header().Set("Content-Length", metadata[mdContentSize])

	// Write explicit status to notify JsonWriter that we're handling the response
	w.WriteHeader(http.StatusOK)

	alog.Info{"action": action, "status": "ok", "namespace": ns,
		"blob_id": id}.LogR(r)
}

// Register the Search endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/{namespace}/search",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ScopeMask(session.ValidSessionScope),
			Handler:          requireAnkiPerson(jsonutil.JsonHandlerFunc(Search)),
		},
	})
}

type searchInput struct {
	Key        string        `json:"key"`
	Value      string        `json:"value"`
	Value2     string        `json:"value2"`
	SearchMode db.SearchMode `json:"mode"`
	MaxResults int           `json:"limit"`
}

type searchResults struct {
	Results []db.Record `json:"results"`
}

// Search for blobs matching a metadata key
//
// POST /1/search
func Search(w jsonutil.JsonWriter, r *http.Request) {
	const action = "search"
	var req searchInput
	ns := mux.Vars(r)["namespace"]

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		alog.Warn{"action": action, "status": apierrors.InvalidJsonCode, "error": err}.LogR(r)
		w.JsonError(r, apierrors.NewInvalidJSON(err.Error()))
		return
	}

	results, err := Store.FindItems(ns, req.Key, req.Value, req.Value2, req.SearchMode)
	if err != nil {
		handleError(w, r, action, err)
		return
	}

	alog.Info{"action": action, "status": "ok", "namespace": ns,
		"search_key": req.Key, "search_value1": req.Value,
		"search_value2": req.Value2, "search_mode": req.SearchMode,
		"max_results": req.MaxResults, "result_count": len(results)}.LogR(r)

	w.JsonOkData(r, searchResults{Results: results})
}

// extract metadata info from HTTP headers
func reqToMetadata(r *http.Request, ns, blobId string) (md map[string]string, err error) {
	md = make(map[string]string)

	for k, v := range r.Header {
		if !strings.HasPrefix(k, userMetadataPrefix) {
			continue
		}
		md[k] = v[0]
		if len(v[0]) == 0 {
			return nil, apierrors.NewInvalidMetadata(fmt.Sprintf("Metadata key %q is empty", k))
		}
	}

	// Add static metadata
	remoteHost, _, _ := net.SplitHostPort(r.RemoteAddr)
	if remoteHost == "" {
		remoteHost = r.RemoteAddr
	}

	if requestId := context.Get(r, "requestId"); requestId != nil {
		md[mdRequestId] = requestId.(string)
	}
	md[mdUploaderIP] = remoteHost
	md[mdUploadTime] = time.Now().Format(time.RFC3339)
	md[mdBlobId] = blobId
	return md, nil
}

func metadataToHeaders(metadata map[string]string, w jsonutil.JsonWriter) {
	h := w.Header()
	for k, v := range metadata {
		h.Set(k, v)
	}
}

func handleError(w jsonutil.JsonWriter, r *http.Request, action string, err error) {
	switch err := err.(type) {
	case jsonutil.JsonError:
		alog.Info{
			"action": action,
			"status": "client_error",
			"error":  err,
		}.LogR(r)
		w.JsonError(r, err)

	default:
		alog.Error{
			"action": action,
			"status": "unexpected_error",
			"error":  err,
		}.LogR(r)
		w.JsonServerError(r)
	}
}
