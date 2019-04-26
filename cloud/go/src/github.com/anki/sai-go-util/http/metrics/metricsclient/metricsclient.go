package metricsclient

import (
	"errors"
	"net/http"
	"time"

	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/metricsutil"
)

var (
	registry          = metricsutil.NewRegistry("http.client")
	NilTransportError = errors.New("HTTP transport can not be nil")
)

// MetricsTransport implements the standard library's HTTP transport
// interface, http.RoundTripper, by wrapping an existing
// implementation of it. MetricsTransport automatically records timer
// metrics for all HTTP operations, and counters for the classes of
// HTTP return codes.
type metricsTransport struct {
	transport http.RoundTripper
	registry  metrics.Registry

	timers   map[string]metrics.Timer
	counters map[int]metrics.Counter
	errors   metrics.Counter
}

type metricsTransportConfig func(t *metricsTransport) error

func WithTransport(rt http.RoundTripper) metricsTransportConfig {
	return func(t *metricsTransport) error {
		if rt == nil {
			return NilTransportError
		}
		t.transport = rt
		return nil
	}
}

// Initialize a new HTTP client, wrapping the transporting in a
// metricsTransport to provide performance and response code
// instrumentation.
//
// Metrics instances are initialized here rather than dynamically, to
// avoid locking on the metrics registry while making client
// calls. Because each client contains its metrics state, client
// instances should be long-lived.
//
// Examples
//
//    // Create a simple instrumented HTTP client
//    c := metricsclient.New("myapi")
//
//    // Create an instrumented HTTP client with a non-default
//    // transport
//    var t http.RoundTriper = ...
//    c := metricsclient.New("myapi", metricsclient.WithTransport(t))
//
func New(prefix string, configs ...metricsTransportConfig) (*http.Client, error) {
	reg := metricsutil.NewChildRegistry(registry, prefix)
	c := &http.Client{}
	t := &metricsTransport{
		transport: http.DefaultTransport,
		registry:  reg,
		timers: map[string]metrics.Timer{
			"GET":     metricsutil.GetTimer("method.GET", reg),
			"PUT":     metricsutil.GetTimer("method.PUT", reg),
			"POST":    metricsutil.GetTimer("method.POST", reg),
			"OPTIONS": metricsutil.GetTimer("method.OPTIONS", reg),
			"HEAD":    metricsutil.GetTimer("method.HEAD", reg),
			"DELETE":  metricsutil.GetTimer("method.DELETE", reg),
			"TRACE":   metricsutil.GetTimer("method.TRACE", reg),
			"CONNECT": metricsutil.GetTimer("method.CONNECT", reg),
			"ALL":     metricsutil.GetTimer("method.ALL", reg),
		},
		counters: map[int]metrics.Counter{
			1: metricsutil.GetCounter("response.1xx", reg),
			2: metricsutil.GetCounter("response.2xx", reg),
			3: metricsutil.GetCounter("response.3xx", reg),
			4: metricsutil.GetCounter("response.4xx", reg),
			5: metricsutil.GetCounter("response.5xx", reg),
		},
		errors: metricsutil.GetCounter("errors", reg),
	}
	for _, cfg := range configs {
		if err := cfg(t); err != nil {
			return nil, err
		}
	}
	c.Transport = t
	return c, nil
}

func (t metricsTransport) processResponse(resp *http.Response, err error) (*http.Response, error) {
	if resp != nil {
		index := int(resp.StatusCode / 100)
		if counter := t.counters[index]; counter != nil {
			counter.Inc(1)
		}
	}
	if err != nil {
		t.errors.Inc(1)
	}
	return resp, err
}

func (t metricsTransport) updateTimer(method string, start time.Time) {
	if timer, ok := t.timers[method]; ok {
		timer.UpdateSince(start)
	}
}

func (t metricsTransport) RoundTrip(req *http.Request) (*http.Response, error) {
	start := time.Now()
	defer t.updateTimer(req.Method, start)
	defer t.updateTimer("ALL", start)

	return t.processResponse(t.transport.RoundTrip(req))
}
