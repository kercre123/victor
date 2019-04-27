package wavefront

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/rcrowley/go-metrics"
)

const (
	wavefrontFormat = "graphite_v2"
	writeError      = "%d: error reporting points to Wavefront"
	batchSize       = 10000
)

// Submits a single metric to the a Wavefront server
func writeSingleMetricToDirect(c *WavefrontConfig, name string, metric interface{}, tags map[string]string) error {
	var points []string
	key := EncodeKey(name, tags)

	if !strings.HasSuffix(c.Prefix, ".") {
		c.Prefix = strings.Join([]string{c.Prefix, "."}, "")
	}
	points = appendMetric(metric, key, c, points)
	return reportPoints(c.DirectReporter, points)
}

func writeRegistryAndFlushToDirect(c *WavefrontConfig) error {
	var points []string
	var retErr error // the last encountered error

	if !strings.HasSuffix(c.Prefix, ".") {
		c.Prefix = strings.Join([]string{c.Prefix, "."}, "")
	}
	c.Registry.Each(func(key string, metric interface{}) {
		points = appendMetric(metric, key, c, points)
		if len(points) >= batchSize {
			// flush and reset points slice
			err := reportPoints(c.DirectReporter, points)
			if err != nil {
				retErr = err
			}
			points = nil
		}
	})

	if len(points) > 0 {
		err := reportPoints(c.DirectReporter, points)
		if err != nil {
			retErr = err
		}
	}
	return retErr
}

func reportPoints(reporter Reporter, points []string) error {
	pointLines := strings.Join(points, "\n")
	resp, err := reporter.Report(wavefrontFormat, pointLines)
	if err != nil {
		return err
	}
	if resp.StatusCode >= 300 {
		return fmt.Errorf(writeError, resp.StatusCode)
	}
	return nil
}

func appendMetric(i interface{}, key string, c *WavefrontConfig, points []string) []string {
	name, tagStr := DecodeKey(key)
	tagStr += hostTagString(c.HostTags)

	switch metric := i.(type) {
	case metrics.Counter:
		return append(points, counterPoint(metric, name, tagStr, c))
	case metrics.Gauge:
		return append(points, gaugePoint(metric, name, tagStr, c))
	case metrics.GaugeFloat64:
		return append(points, gaugeFloat64Point(metric, name, tagStr, c))
	case metrics.Histogram:
		return append(points, histoPoints(metric, name, tagStr, c)...)
	case metrics.Meter:
		return append(points, meterPoints(metric, name, tagStr, c)...)
	case metrics.Timer:
		return append(points, timerPoints(metric, name, tagStr, c)...)
	}
	return points
}

func counterPoint(metric metrics.Counter, name, tagStr string, c *WavefrontConfig) string {
	if hasDeltaPrefix(name) {
		return deltaPoint(metric, name, tagStr, 0, c)
	}
	return fmt.Sprintf("%s%s.count %d %s", c.Prefix, name, metric.Count(), tagStr)
}

func gaugePoint(metric metrics.Gauge, name, tagStr string, c *WavefrontConfig) string {
	return fmt.Sprintf("%s%s.value %d %s", c.Prefix, name, metric.Value(), tagStr)
}

func gaugeFloat64Point(metric metrics.GaugeFloat64, name, tagStr string, c *WavefrontConfig) string {
	return fmt.Sprintf("%s%s.value %f %s", c.Prefix, name, metric.Value(), tagStr)
}

func histoPoints(metric metrics.Histogram, name, tagStr string, c *WavefrontConfig) []string {
	points := make([]string, 5+len(c.Percentiles))
	h := metric.Snapshot()
	ps := h.Percentiles(c.Percentiles)
	i := 0
	points[i], i = fmt.Sprintf("%s%s.count %d %s", c.Prefix, name, h.Count(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.min %d %s", c.Prefix, name, h.Min(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.max %d %s", c.Prefix, name, h.Max(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.mean %.2f %s", c.Prefix, name, h.Mean(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.std-dev %.2f %s", c.Prefix, name, h.StdDev(), tagStr), i+1
	for psIdx, psKey := range c.Percentiles {
		key := strings.Replace(strconv.FormatFloat(psKey*100.0, 'f', -1, 64), ".", "", 1)
		points[i], i = fmt.Sprintf("%s%s.%s-percentile %.2f %s", c.Prefix, name, key, ps[psIdx], tagStr), i+1
	}
	return points
}

func meterPoints(metric metrics.Meter, name, tagStr string, c *WavefrontConfig) []string {
	points := make([]string, 5)
	m := metric.Snapshot()
	points[0] = fmt.Sprintf("%s%s.count %d %s", c.Prefix, name, m.Count(), tagStr)
	points[1] = fmt.Sprintf("%s%s.one-minute %.2f %s", c.Prefix, name, m.Rate1(), tagStr)
	points[2] = fmt.Sprintf("%s%s.five-minute %.2f %s", c.Prefix, name, m.Rate5(), tagStr)
	points[3] = fmt.Sprintf("%s%s.fifteen-minute %.2f %s", c.Prefix, name, m.Rate15(), tagStr)
	points[4] = fmt.Sprintf("%s%s.mean %.2f %s", c.Prefix, name, m.RateMean(), tagStr)
	return points
}

func timerPoints(metric metrics.Timer, name, tagStr string, c *WavefrontConfig) []string {
	points := make([]string, 9+len(c.Percentiles))
	t := metric.Snapshot()
	du := float64(c.DurationUnit)
	ps := t.Percentiles(c.Percentiles)
	i := 0
	points[i], i = fmt.Sprintf("%s%s.count %d %s", c.Prefix, name, t.Count(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.min %d %s", c.Prefix, name, t.Min()/int64(du), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.max %d %s", c.Prefix, name, t.Max()/int64(du), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.mean %.2f %s", c.Prefix, name, t.Mean()/du, tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.std-dev %.2f %s", c.Prefix, name, t.StdDev()/du, tagStr), i+1
	for psIdx, psKey := range c.Percentiles {
		key := strings.Replace(strconv.FormatFloat(psKey*100.0, 'f', -1, 64), ".", "", 1)
		points[i], i = fmt.Sprintf("%s%s.%s-percentile %.2f %s", c.Prefix, name, key, ps[psIdx]/du, tagStr), i+1
	}
	points[i], i = fmt.Sprintf("%s%s.one-minute %.2f %s", c.Prefix, name, t.Rate1(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.five-minute %.2f %s", c.Prefix, name, t.Rate5(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.fifteen-minute %.2f %s", c.Prefix, name, t.Rate15(), tagStr), i+1
	points[i], i = fmt.Sprintf("%s%s.mean-rate %.2f %s", c.Prefix, name, t.RateMean(), tagStr), i+1
	return points
}
