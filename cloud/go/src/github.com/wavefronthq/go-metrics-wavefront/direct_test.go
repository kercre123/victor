package wavefront

import (
	"strings"
	"testing"
	"time"

	"github.com/rcrowley/go-metrics"
)

var (
	tags = map[string]string{
		"key1": "val1",
	}
	tagStr = "key1=\"val1\" key2=\"val2\""
)

func testConfig() *WavefrontConfig {
	return &WavefrontConfig{
		Prefix:       "test.prefix.",
		Percentiles:  []float64{0.5, 0.75, 0.95, 0.99, 0.999},
		DurationUnit: 5 * time.Second,
	}
}

func testConfigWithNoPrefix() *WavefrontConfig {
	return &WavefrontConfig{
		Prefix:       "",
		Percentiles:  []float64{0.5, 0.75, 0.95, 0.99, 0.999},
		DurationUnit: 5 * time.Second,
	}
}

func TestAppendMetric(t *testing.T) {
	counter := metrics.NewCounter()
	counter.Inc(10)
	key := EncodeKey("foo", tags)
	var points []string
	points = appendMetric(counter, key, testConfig(), points)

	if len(points) != 1 {
		t.Error("Expected len=1", "Actual len=", len(points))
	}
}

func TestCounterPoint(t *testing.T) {
	counter := metrics.NewCounter()
	counter.Inc(10)
	point := counterPoint(counter, "foo", tagStr, testConfig())
	expected := "test.prefix.foo.count 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("counters don't match", expected, point)
	}
}

func TestCounterPointWithNoPrefix(t *testing.T) {
	counter := metrics.NewCounter()
	counter.Inc(10)
	point := counterPoint(counter, "foo", tagStr, testConfigWithNoPrefix())
	expected := "foo.count 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("counters don't match", expected, point)
	}
}

func TestDeltaPoint(t *testing.T) {
	counter := metrics.NewCounter()
	counter.Inc(10)
	name := DeltaCounterName("foo")
	point := deltaPoint(counter, name, tagStr, 0, testConfig())

	if !strings.HasPrefix(point, "∆") {
		t.Error("invalid delta prefix", point)
	}
	expected := "∆test.prefix.foo.count 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("counters don't match", expected, point)
	}
}

func TestDeltaPointWithNoPrefix(t *testing.T) {
	counter := metrics.NewCounter()
	counter.Inc(10)
	name := DeltaCounterName("foo")
	point := deltaPoint(counter, name, tagStr, 0, testConfigWithNoPrefix())

	if !strings.HasPrefix(point, "∆") {
		t.Error("invalid delta prefix", point)
	}
	expected := "∆foo.count 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("delta counters don't match", expected, point)
	}
}

func TestGaugePoint(t *testing.T) {
	gauge := metrics.NewGauge()
	gauge.Update(10)
	point := gaugePoint(gauge, "foo", tagStr, testConfig())
	expected := "test.prefix.foo.value 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("gauges don't match", expected, point)
	}
}

func TestGaugePointWithNoPrefix(t *testing.T) {
	gauge := metrics.NewGauge()
	gauge.Update(10)
	point := gaugePoint(gauge, "foo", tagStr, testConfigWithNoPrefix())
	expected := "foo.value 10 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("gauges don't match", expected, point)
	}
}

func TestGaugeFloat64Point(t *testing.T) {
	gauge := metrics.NewGaugeFloat64()
	gauge.Update(10)
	point := gaugeFloat64Point(gauge, "foo", tagStr, testConfig())
	expected := "test.prefix.foo.value 10.000000 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("gaugeFloat64's don't match", expected, point)
	}
}

func TestGaugeFloat64PointWithNoPrefix(t *testing.T) {
	gauge := metrics.NewGaugeFloat64()
	gauge.Update(10)
	point := gaugeFloat64Point(gauge, "foo", tagStr, testConfigWithNoPrefix())
	expected := "foo.value 10.000000 key1=\"val1\" key2=\"val2\""
	if strings.TrimRight(point, "\n") != expected {
		t.Error("gaugeFloat64's don't match", expected, point)
	}
}

func TestHistoPoints(t *testing.T) {
	s := metrics.NewExpDecaySample(1028, 0.015)
	histo := metrics.NewHistogram(s)
	histo.Update(10)
	points := histoPoints(histo, "foo", tagStr, testConfig())
	if len(points) != 10 {
		t.Error("invalid histogram result")
	}
}

func TestMeterPoints(t *testing.T) {
	meter := metrics.NewMeter()
	meter.Mark(10)
	points := meterPoints(meter, "foo", tagStr, testConfig())
	if len(points) != 5 {
		t.Error("invalid meter result")
	}
}

func TestTimerPoints(t *testing.T) {
	timer := metrics.NewTimer()
	timer.Time(func() {})
	timer.Update(10)
	points := timerPoints(timer, "foo", tagStr, testConfig())
	if len(points) != 14 {
		t.Error("invalid timer result")
	}
}
