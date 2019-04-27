// Package wavefront is a plugin for go-metrics that provides a Wavefront reporter and tag support at the host and metric level.
package wavefront

import (
	"errors"
	"log"
	"net"
	"net/url"
	"sort"
	"strings"
	"time"

	"github.com/rcrowley/go-metrics"
)

var (
	configError = errors.New("error: invalid wavefront configuration")
	directError = errors.New("error: invalid server or token")
)

// RegisterMetric tag support for metrics.Register()
func RegisterMetric(key string, metric interface{}, tags map[string]string) {
	key = EncodeKey(key, tags)
	metrics.Register(key, metric)
}

// GetMetric tag support for metrics.Get()
func GetMetric(key string, tags map[string]string) interface{} {
	key = EncodeKey(key, tags)
	return metrics.Get(key)
}

// GetOrRegisterMetric tag support for metrics.GetOrRegister()
func GetOrRegisterMetric(name string, i interface{}, tags map[string]string) interface{} {
	key := EncodeKey(name, tags)
	return metrics.GetOrRegister(key, i)
}

// UnregisterMetric tag support for metrics.UnregisterMetric()
func UnregisterMetric(name string, tags map[string]string) {
	key := EncodeKey(name, tags)
	metrics.Unregister(key)
}

// EncodeKey encodes the metric name and tags into a unique key.
func EncodeKey(key string, tags map[string]string) string {
	//sort the tags to ensure the key is always the same when getting or setting
	sortedKeys := make([]string, len(tags))
	i := 0
	for k, _ := range tags {
		sortedKeys[i] = k
		i++
	}
	sort.Strings(sortedKeys)
	keyAppend := "["
	for i := range sortedKeys {
		keyAppend += " " + sortedKeys[i] + "=\"" + tags[sortedKeys[i]] + "\""
	}
	keyAppend += "]"
	key += keyAppend
	return key
}

// DecodeKey decodes a metric key into a metric name and tag string
func DecodeKey(key string) (string, string) {
	if strings.Contains(key, "[") == false {
		return key, ""
	}
	parts := strings.Split(key, "[")
	name := parts[0]
	tagStr := parts[1]
	tagStr = tagStr[0 : len(tagStr)-1]
	return name, tagStr
}

func hostTagString(hostTags map[string]string) string {
	htStr := ""
	for k, v := range hostTags {
		htStr += " " + k + "=\"" + v + "\""
	}
	return htStr
}

// WavefrontConfig provides configuration parameters for
// the Wavefront exporter
type WavefrontConfig struct {
	Addr           *net.TCPAddr     // Network address to connect to
	DirectReporter Reporter         // DirectReporter for direct connect (Proxy Addr takes precedence if provided)
	Registry       metrics.Registry // Registry to be exported
	FlushInterval  time.Duration    // Flush interval
	DurationUnit   time.Duration    // Time conversion unit for durations
	Prefix         string           // Prefix to be prepended to metric names. Note that a period is automatically appended to the prefix (if non-empty).
	Percentiles    []float64        // Percentiles to export from timers and histograms
	HostTags       map[string]string
}

// An exporter function which reports metrics to a wavefront proxy located at addr, flushing them every d duration.
func WavefrontProxy(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr) error {
	if addr == nil {
		return configError
	}
	WavefrontWithConfig(WavefrontConfig{
		Addr:          addr,
		Registry:      r,
		FlushInterval: d,
		DurationUnit:  time.Nanosecond,
		Prefix:        prefix,
		HostTags:      ht,
		Percentiles:   []float64{0.5, 0.75, 0.95, 0.99, 0.999},
	})
	return nil
}

// An exporter function which reports metrics directly to a wavefront server every d duration.
func WavefrontDirect(r metrics.Registry, d time.Duration, ht map[string]string, prefix, server, token string) error {
	if server == "" || token == "" {
		return directError
	}
	if _, err := url.ParseRequestURI(server); nil != err {
		return err
	}

	WavefrontWithConfig(WavefrontConfig{
		DirectReporter: NewDirectReporter(server, token),
		Registry:       r,
		FlushInterval:  d,
		DurationUnit:   time.Nanosecond,
		Prefix:         prefix,
		HostTags:       ht,
		Percentiles:    []float64{0.5, 0.75, 0.95, 0.99, 0.999},
	})
	return nil
}

// Deprecated: Use WavefrontProxy() instead.
// Maintained for backwards compatibility, will be removed in the future.
func Wavefront(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr) {
	if err := WavefrontProxy(r, d, ht, prefix, addr); nil != err {
		log.Println(err)
	}
}

// Similar to WavefrontProxy() or WavefrontDirect() but allows caller to pass in a WavefrontConfig struct
func WavefrontWithConfig(c WavefrontConfig) {
	for _ = range time.Tick(c.FlushInterval) {
		if err := writeEntireRegistryAndFlush(&c); nil != err {
			log.Println(err)
		}
	}
}

// WavefrontOnce performs a single submission to Wavefront, returning a
// non-nil error on failed connections. This can be used in a loop
// similar to WavefrontWithConfig for custom error handling.
func WavefrontOnce(c WavefrontConfig) error {
	return writeEntireRegistryAndFlush(&c)
}

// WavefrontSingleMetric submits a single metric to Wavefront. The given metric
// is not registered in the underyling `go-metrics` registry and the registry
// will not be flushed entirely (unlike `WavefrontOnce`). If the connection to
// the proxy or a Wavefront server cannot be made, a non-nil error is returned.
func WavefrontSingleMetric(c *WavefrontConfig, name string, metric interface{}, tags map[string]string) error {
	// Proxy takes precedence if both proxy and direct are provided
	if c.Addr != nil {
		return writeSingleMetricToProxy(c, name, metric, tags)
	}
	if c.DirectReporter != nil {
		return writeSingleMetricToDirect(c, name, metric, tags)
	}
	return configError
}

func writeEntireRegistryAndFlush(c *WavefrontConfig) error {
	// Proxy takes precedence if both proxy and direct are provided
	if c.Addr != nil {
		return writeRegistryAndFlushToProxy(c)
	}
	if c.DirectReporter != nil {
		return writeRegistryAndFlushToDirect(c)
	}
	return configError
}
