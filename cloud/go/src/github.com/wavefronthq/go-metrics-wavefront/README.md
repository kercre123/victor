# go-metrics-wavefront [![GoDoc](https://godoc.org/github.com/wavefrontHQ/go-metrics-wavefront?status.svg)](https://godoc.org/github.com/wavefrontHQ/go-metrics-wavefront) [![travis build status](https://travis-ci.com/wavefrontHQ/go-metrics-wavefront.svg?branch=master)](https://travis-ci.com/wavefrontHQ/go-metrics-wavefront)

This is a plugin for [go-metrics](https://github.com/rcrowley/go-metrics) which adds a Wavefront reporter and a simple abstraction that supports tagging at the host and metric level.

## Usage

### Wavefront Reporter

The Wavefront Reporter supports tagging at the host level. Any tags passed to the reporter here will be applied to every metric before being sent to Wavefront.

```go
import (
  "github.com/rcrowley/go-metrics"
  "github.com/wavefronthq/go-metrics-wavefront"
)

func main() {
  hostTags := map[string]string{
    "source": "go-metrics-test",
  }
  // report to a Wavefront proxy
  go wavefront.WavefrontProxy(metrics.DefaultRegistry, 1*time.Second, hostTags, "some.prefix", addr)

  // report to a Wavefront server
  go wavefront.WavefrontDirect(metrics.DefaultRegistry, 5*time.Second, hostTags, "direct.prefix", server, token)
}
```

### Tagging Metrics

In addition to tagging at the host level, you can add tags to individual metrics.

```go
import (
  "github.com/rcrowley/go-metrics"
  "github.com/wavefronthq/go-metrics-wavefront"
)

func main() {

  c := metrics.NewCounter()
  wavefront.RegisterMetric(
    "foo", c, map[string]string{
      "key1": "val1",
      "key2": "val2",
    })
  c.Inc(47)
}
```
`wavefront.RegisterMetric()` has the same affect as go-metrics' `metrics.Register()` except that it accepts tags in the form of a string map. The tags are then used by the Wavefront reporter at flush time. The tags become part of the key for a metric within go-metrics' Registry. Every unique combination of metric name+tags is a unique series. You can pass your tags in any order to the Register and Get functions documented below. The Wavefront plugin ensures the tags are always encoded in the same order within the Registry to ensure no duplication of metric series.

[Go Docs](https://github.com/wavefrontHQ/go-metrics-wavefront/blob/master/GODOCS.md)

### Extended Code Example

```go
package main

import (
  "fmt"
  "net"
  "time"

  "github.com/rcrowley/go-metrics"
  "github.com/wavefronthq/go-metrics-wavefront"
)

func main() {

  //Create a counter
  c := metrics.NewCounter()
  //Tags we'll add to the metric
  tags := map[string]string{
    "key2": "val1",
    "key1": "val2",
    "key0": "val0",
    "key4": "val4",
    "key3": "val3",
  }
  // Register it using wavefront.RegisterMetric instead of metrics.Register if there are tags
  wavefront.RegisterMetric("foo", c, tags)
  c.Inc(47)

  // Retrieve it using metric name and tags.
  // Any unique set of name+tags will be a unique series and thus a unique metric
  m2 := wavefront.GetMetric("foo", tags)
  fmt.Println(m2) // will print &{47}

  // Retrieve it using wavefront.GetOrRegisterMetric instead of metrics.GetOrRegister if there are tags.
  m3 := wavefront.GetOrRegisterMetric("foo", c, tags)
  fmt.Println(m3) // will print &{47}

  //Let's remove the metric now
  wavefront.UnregisterMetric("foo", tags)

  //Try to get it after unregistering
  m4 := wavefront.GetMetric("foo", tags)
  fmt.Println(m4) // will print <nil>

  //Lets add it again and send it to Wavefront
  wavefront.RegisterMetric("foo", c, tags)
  c.Inc(47)

  // Set the address of the Wavefront Proxy
  addr, _ := net.ResolveTCPAddr("tcp", "192.168.99.100:2878")

  // Tags can be passed to the host as well (each tag will get applied to every metric)
  hostTags := map[string]string{
    "source": "go-metrics-test",
  }

  go wavefront.WavefrontProxy(metrics.DefaultRegistry, 1*time.Second, hostTags, "some.prefix", addr)

  // Send metrics directly to a wavefront server
  server := "https://clusterName.wavefront.com"
  token := "ENTER_TOKEN_HERE"
  go wavefront.WavefrontDirect(metrics.DefaultRegistry, 5*time.Second, hostTags, "direct.prefix", server, token)

  fmt.Println("Search wavefront: ts(\"some.prefix.foo.count\")")
  fmt.Println("Search wavefront: ts(\"direct.prefix.foo.count\")")

  fmt.Println("Entering loop to simulate metrics flushing. Hit ctrl+c to cancel")
  
  select{}
}
```
