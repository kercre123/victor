package main

import (
	"fmt"
	"net"
	"time"

	"github.com/rcrowley/go-metrics"
	"github.com/wavefronthq/go-metrics-wavefront"
)

func directConfig(server, token string, ht map[string]string) *wavefront.WavefrontConfig {
	return &wavefront.WavefrontConfig{
		DirectReporter: wavefront.NewDirectReporter(server, token),
		Prefix:         "direct.prefix",
		HostTags:       ht,
		Percentiles:    []float64{0.5, 0.75, 0.95, 0.99, 0.999},
	}
}

func main() {

	//Create a counter
	c := metrics.NewCounter()
	//Tags we'll add to the metric
	tags := map[string]string{
		"key2": "val2",
		"key1": "val1",
		"key0": "val0",
		"key4": "val4",
		"key3": "val3",
	}
	// Register it using wavefront.RegisterMetric instead of metrics.Register if there are tags
	wavefront.RegisterMetric("foo", c, tags)
	c.Inc(47)

	// Retreive it using our key and tags.
	// Any unique set of key+tags will be a unique series and thus a unique metric
	m2 := wavefront.GetMetric("foo", tags)
	fmt.Println(m2) // will print &{47}

	//Try retrieving it with the same tags but in a different order
	tags2 := map[string]string{
		"key4": "val4",
		"key2": "val2",
		"key3": "val3",
		"key0": "val0",
		"key1": "val1",
	}
	m3 := wavefront.GetMetric("foo", tags2)
	fmt.Println("Getting with tags in different order:")
	fmt.Println(m3)

	// Retreive it using wavefront.GetOrRegisterMetric instead of metrics.GetOrRegister if there are tags.
	m4 := wavefront.GetOrRegisterMetric("foo", c, tags)
	fmt.Println(m4) // will print &{47}

	//Let's remove the metric now
	wavefront.UnregisterMetric("foo", tags)

	//Try to get it after unregistering
	m5 := wavefront.GetMetric("foo", tags)
	fmt.Println(m5) // will print <nil>

	//Lets add it again and send it to Wavefront
	wavefront.RegisterMetric("foo", c, tags)
	c.Inc(47)

	deltaCounter := metrics.NewCounter()
	wavefront.RegisterMetric(wavefront.DeltaCounterName("∆delta.metric"), deltaCounter, tags)
	deltaCounter.Inc(10)

	// Set the address of the Wavefront Proxy
	addr, _ := net.ResolveTCPAddr("tcp", "localhost:2878")

	// Tags can be passed to the host as well (each tag will get applied to every metric)
	hostTags := map[string]string{
		"source": "go-metrics-test",
	}

	go wavefront.WavefrontProxy(metrics.DefaultRegistry, 1*time.Second, hostTags, "proxy.prefix", addr)

	// Set the server and token for direct ingestion
	server := "https://clusterName.wavefront.com"
	token := "ENTER_TOKEN_HERE"
	directCfg := directConfig(server, token, hostTags)
	wavefront.WavefrontSingleMetric(directCfg, "single.metric", c, nil)

	go wavefront.WavefrontDirect(metrics.DefaultRegistry, 5*time.Second, hostTags, "direct.prefix", server, token)

	fmt.Println("Search wavefront: ts(\"some.prefix.foo.count\")")

	fmt.Println("Entering loop to simulate metrics flushing. Hit ctrl+c to cancel")
	select {}
}
