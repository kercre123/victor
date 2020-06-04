package profileserver

import (
	"expvar"
	"fmt"
	"net/http"
	_ "net/http/pprof"
	"runtime"
	"time"

	"github.com/aalpern/go-metrics"
	"github.com/aalpern/go-metrics-charts"
	"github.com/aalpern/go-metrics/exp"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
	"github.com/gwatts/manners"
)

var (
	// Metrics reporting configuration
	MetricsRoot        = "metrics"
	MetricsStructured  = true
	MetricsPercentiles = []float64{0.5, 0.75, 0.95, 0.98, 0.99, 0.999}
)

func init() {
	envconfig.String(&MetricsRoot, "METRICS_ROOT", "metrics-root", "Root key for metrics reporting")
	envconfig.Bool(&MetricsStructured, "METRICS_STRUCTURED", "metrics-structured", "Report metrics as maps if true")
}

type RuntimeStats struct {
	NumGoroutine int
	NumCgoCall   int64
}

func (r *RuntimeStats) Read() *RuntimeStats {
	r.NumGoroutine = runtime.NumGoroutine()
	r.NumCgoCall = runtime.NumCgoCall()
	return r
}

// NewServer configures and returns a new profiling server
func NewServer(profilingPort int) *manners.GracefulServer {
	// Register metrics as expvars and bind the /debug/metrics
	// endpoint. TODO: make the metrics expvars parameters
	// configurable.
	exp.Config.RootKey = MetricsRoot
	exp.Config.StructuredMetrics = MetricsStructured
	exp.Config.TimerScale = time.Millisecond
	exp.Config.SetPercentiles(MetricsPercentiles)
	exp.Exp(metrics.DefaultRegistry)
	expvar.Publish("runtime", expvar.Func(func() interface{} {
		return new(RuntimeStats).Read()
	}))
	metricscharts.Register()

	return manners.NewWithServer(&http.Server{
		Addr: fmt.Sprintf(":%d", profilingPort),
		// net/http/pprof automatically registers with DefaultServeMux on import
		Handler:  http.DefaultServeMux,
		ErrorLog: alog.MakeLogger(alog.LogInfo, "profhttperror"),
	})
}
