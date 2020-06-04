package metricsutil

import (
	"github.com/aalpern/go-metrics"
)

var DefaultRegistry = metrics.DefaultRegistry

func NewRegistry(prefix string) metrics.Registry {
	return metrics.NewPrefixedChildRegistry(DefaultRegistry, prefix+".")
}

func NewChildRegistry(parent metrics.Registry, prefix string) metrics.Registry {
	return metrics.NewPrefixedChildRegistry(parent, prefix+".")
}

func GetTimer(name string, reg metrics.Registry) metrics.Timer {
	return metrics.GetOrRegisterTimer(name, reg)
}

func GetCounter(name string, reg metrics.Registry) metrics.Counter {
	return metrics.GetOrRegisterCounter(name, reg)
}

func GetGauge(name string, reg metrics.Registry) metrics.Gauge {
	return metrics.GetOrRegisterGauge(name, reg)
}

func GetGaugeFloat64(name string, reg metrics.Registry) metrics.GaugeFloat64 {
	return metrics.GetOrRegisterGaugeFloat64(name, reg)
}

func GetHistogram(name string, reg metrics.Registry, sample metrics.Sample) metrics.Histogram {
	return metrics.GetOrRegisterHistogram(name, reg, sample)
}

func GetMeter(name string, reg metrics.Registry) metrics.Meter {
	return metrics.GetOrRegisterMeter(name, reg)
}
