package metricshandler

import (
	"net/http"
	"time"

	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/metricsutil"
)

// ----------------------------------------------------------------------
// Metrics Handler HTTP middleware
// ----------------------------------------------------------------------

// A MetricsHandler wraps a standard http.Handler and provides metrics
// on all operations. Tracked metrics include counts for HTTP response
// code ranges (1xx, 2xx, etc...) and latency for all
// requests. Latency metrics are tracked in agreggate across all
// request paths by HTTP method, and also by method for each unique
// path requested.
type MetricsHandler struct {

	// A registry for the metrics to be recorded in. A prefixed or
	// child registry can be used to group different sets of
	// endspoints as necessary.
	Registry metrics.Registry

	// Handler is the wrapped HTTP handler.
	Handler http.Handler

	// The path (with mux variables) this handler handles, for metrics
	// reporting.
	Path string

	counters map[int]metrics.Counter
}

func (h *MetricsHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	// Set up metrics
	all := metricsutil.GetTimer("all."+r.Method, h.Registry)
	forThisPath := metricsutil.GetTimer("path."+h.Path+"."+r.Method, h.Registry)

	start := time.Now()
	defer all.UpdateSince(start)
	defer forThisPath.UpdateSince(start)

	// This does not force the response to JSON, it just wraps the
	// writer so we can capture the response code.
	jw := jsonutil.ToJsonWriter(w)
	h.Handler.ServeHTTP(jw, r)

	index := int(jw.StatusCode() / 100)
	if counter, ok := h.counters[index]; ok {
		counter.Inc(1)
	}
}

var (
	registry = metricsutil.NewRegistry("http.server")
)

// Create and initialize a new MetricsHandler which registers its
// metrics in the default http server metrics registry.
func Handler(path string, h http.Handler) *MetricsHandler {
	return HandlerWithRegistry(path, h, registry)
}

// Create and initalize a new MetricsHandler, specifying the metrics
// registry to use for registering its metrics. Multiple handlers can
// be instantiated with distinct metrics by using different prefixed
// registries.
func HandlerWithRegistry(path string, h http.Handler, r metrics.Registry) *MetricsHandler {
	return &MetricsHandler{
		Handler:  h,
		Registry: r,
		Path:     path,
		counters: map[int]metrics.Counter{
			1: metricsutil.GetCounter("response.1xx", r),
			2: metricsutil.GetCounter("response.2xx", r),
			3: metricsutil.GetCounter("response.3xx", r),
			4: metricsutil.GetCounter("response.4xx", r),
			5: metricsutil.GetCounter("response.5xx", r),
		},
	}
}
