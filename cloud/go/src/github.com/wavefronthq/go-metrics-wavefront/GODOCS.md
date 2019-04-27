

# wavefront
`import "github.com/wavefronthq/go-metrics-wavefront"`

* [Overview](#pkg-overview)
* [Index](#pkg-index)
* [Subdirectories](#pkg-subdirectories)

## <a name="pkg-overview">Overview</a>
Package wavefront is a plugin for go-metrics that provides a Wavefront reporter and tag support at the host and metric level.




## <a name="pkg-index">Index</a>
* [func DecodeKey(key string) (string, string)](#DecodeKey)
* [func DeltaCounterName(name string) string](#DeltaCounterName)
* [func EncodeKey(key string, tags map[string]string) string](#EncodeKey)
* [func GetMetric(key string, tags map[string]string) interface{}](#GetMetric)
* [func GetOrRegisterMetric(name string, i interface{}, tags map[string]string) interface{}](#GetOrRegisterMetric)
* [func RegisterMetric(key string, metric interface{}, tags map[string]string)](#RegisterMetric)
* [func UnregisterMetric(name string, tags map[string]string)](#UnregisterMetric)
* [func Wavefront(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr)](#Wavefront)
* [func WavefrontDirect(r metrics.Registry, d time.Duration, ht map[string]string, prefix, server, token string) error](#WavefrontDirect)
* [func WavefrontOnce(c WavefrontConfig) error](#WavefrontOnce)
* [func WavefrontProxy(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr) error](#WavefrontProxy)
* [func WavefrontSingleMetric(c *WavefrontConfig, name string, metric interface{}, tags map[string]string) error](#WavefrontSingleMetric)
* [func WavefrontWithConfig(c WavefrontConfig)](#WavefrontWithConfig)
* [func WriteMetricAndFlush(w *bufio.Writer, i interface{}, key string, ts int64, c *WavefrontConfig)](#WriteMetricAndFlush)
* [type Reporter](#Reporter)
  * [func NewDirectReporter(server string, token string) Reporter](#NewDirectReporter)
* [type WavefrontConfig](#WavefrontConfig)


#### <a name="pkg-files">Package files</a>
[api.go](/src/target/api.go) [delta.go](/src/target/delta.go) [direct.go](/src/target/direct.go) [proxy.go](/src/target/proxy.go) [wavefront.go](/src/target/wavefront.go) 





## <a name="DecodeKey">func</a> [DecodeKey](/src/target/wavefront.go?s=1732:1775#L65)
``` go
func DecodeKey(key string) (string, string)
```
DecodeKey decodes a metric key into a metric name and tag string



## <a name="DeltaCounterName">func</a> [DeltaCounterName](/src/target/delta.go?s=435:476#L20)
``` go
func DeltaCounterName(name string) string
```
Gets a delta counter name prefixed with ∆.
Can be used as an input for RegisterMetric() or GetOrRegisterMetric() functions



## <a name="EncodeKey">func</a> [EncodeKey](/src/target/wavefront.go?s=1223:1280#L46)
``` go
func EncodeKey(key string, tags map[string]string) string
```
EncodeKey encodes the metric name and tags into a unique key.



## <a name="GetMetric">func</a> [GetMetric](/src/target/wavefront.go?s=630:692#L28)
``` go
func GetMetric(key string, tags map[string]string) interface{}
```
GetMetric tag support for metrics.Get()



## <a name="GetOrRegisterMetric">func</a> [GetOrRegisterMetric](/src/target/wavefront.go?s=814:902#L34)
``` go
func GetOrRegisterMetric(name string, i interface{}, tags map[string]string) interface{}
```
GetOrRegisterMetric tag support for metrics.GetOrRegister()



## <a name="RegisterMetric">func</a> [RegisterMetric](/src/target/wavefront.go?s=447:522#L22)
``` go
func RegisterMetric(key string, metric interface{}, tags map[string]string)
```
RegisterMetric tag support for metrics.Register()



## <a name="UnregisterMetric">func</a> [UnregisterMetric](/src/target/wavefront.go?s=1039:1097#L40)
``` go
func UnregisterMetric(name string, tags map[string]string)
```
UnregisterMetric tag support for metrics.UnregisterMetric()



## <a name="Wavefront">func</a> [Wavefront](/src/target/wavefront.go?s=4092:4199#L137)
``` go
func Wavefront(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr)
```
Deprecated: Use WavefrontProxy() instead.
Maintained for backwards compatibility, will be removed in the future.



## <a name="WavefrontDirect">func</a> [WavefrontDirect](/src/target/wavefront.go?s=3431:3546#L115)
``` go
func WavefrontDirect(r metrics.Registry, d time.Duration, ht map[string]string, prefix, server, token string) error
```
An exporter function which reports metrics directly to a wavefront server every d duration.



## <a name="WavefrontOnce">func</a> [WavefrontOnce](/src/target/wavefront.go?s=4775:4818#L155)
``` go
func WavefrontOnce(c WavefrontConfig) error
```
WavefrontOnce performs a single submission to Wavefront, returning a
non-nil error on failed connections. This can be used in a loop
similar to WavefrontWithConfig for custom error handling.



## <a name="WavefrontProxy">func</a> [WavefrontProxy](/src/target/wavefront.go?s=2915:3033#L98)
``` go
func WavefrontProxy(r metrics.Registry, d time.Duration, ht map[string]string, prefix string, addr *net.TCPAddr) error
```
An exporter function which reports metrics to a wavefront proxy located at addr, flushing them every d duration.



## <a name="WavefrontSingleMetric">func</a> [WavefrontSingleMetric](/src/target/wavefront.go?s=5181:5290#L163)
``` go
func WavefrontSingleMetric(c *WavefrontConfig, name string, metric interface{}, tags map[string]string) error
```
WavefrontSingleMetric submits a single metric to Wavefront. The given metric
is not registered in the underyling `go-metrics` registry and the registry
will not be flushed entirely (unlike `WavefrontOnce`). If the connection to
the proxy or a Wavefront server cannot be made, a non-nil error is returned.



## <a name="WavefrontWithConfig">func</a> [WavefrontWithConfig](/src/target/wavefront.go?s=4397:4440#L144)
``` go
func WavefrontWithConfig(c WavefrontConfig)
```
Similar to WavefrontProxy() or WavefrontDirect() but allows caller to pass in a WavefrontConfig struct



## <a name="WriteMetricAndFlush">func</a> [WriteMetricAndFlush](/src/target/proxy.go?s=854:952#L43)
``` go
func WriteMetricAndFlush(w *bufio.Writer, i interface{}, key string, ts int64, c *WavefrontConfig)
```



## <a name="Reporter">type</a> [Reporter](/src/target/api.go?s=501:611#L27)
``` go
type Reporter interface {
    Report(format string, pointLines string) (*http.Response, error)
    Server() string
}
```
DirectReporter is an interface representing the ability to report points to a Wavefront service.







### <a name="NewDirectReporter">func</a> [NewDirectReporter](/src/target/api.go?s=775:835#L38)
``` go
func NewDirectReporter(server string, token string) Reporter
```




## <a name="WavefrontConfig">type</a> [WavefrontConfig](/src/target/wavefront.go?s=2212:2797#L86)
``` go
type WavefrontConfig struct {
    Addr           *net.TCPAddr     // Network address to connect to
    DirectReporter Reporter         // DirectReporter for direct connect (Proxy Addr takes precedence if provided)
    Registry       metrics.Registry // Registry to be exported
    FlushInterval  time.Duration    // Flush interval
    DurationUnit   time.Duration    // Time conversion unit for durations
    Prefix         string           // Prefix to be prepended to metric names
    Percentiles    []float64        // Percentiles to export from timers and histograms
    HostTags       map[string]string
}
```
WavefrontConfig provides configuration parameters for
the Wavefront exporter














- - -
Generated by [godoc2md](http://godoc.org/github.com/davecheney/godoc2md)
