package wavefront

import (
	"bufio"
	"fmt"
	"github.com/rcrowley/go-metrics"
	"net"
	"strconv"
	"strings"
	"time"
)

func writeRegistryAndFlushToProxy(c *WavefrontConfig) error {
	now := time.Now().Unix()
	conn, err := net.DialTCP("tcp", nil, c.Addr)
	if nil != err {
		return err
	}
	defer conn.Close()
	w := bufio.NewWriter(conn)
	if !strings.HasSuffix(c.Prefix, ".") {
		c.Prefix = strings.Join([]string{c.Prefix, "."}, "")
	}
	c.Registry.Each(func(key string, metric interface{}) {
		WriteMetricAndFlush(w, metric, key, now, c)
	})
	return nil
}

// Submits a single metric to the proxy
func writeSingleMetricToProxy(c *WavefrontConfig, name string, metric interface{}, tags map[string]string) error {
	now := time.Now().Unix()
	conn, err := net.DialTCP("tcp", nil, c.Addr)
	if nil != err {
		return err
	}
	defer conn.Close()
	w := bufio.NewWriter(conn)

	key := EncodeKey(name, tags)
	if !strings.HasSuffix(c.Prefix, ".") {
		c.Prefix = strings.Join([]string{c.Prefix, "."}, "")
	}
	WriteMetricAndFlush(w, metric, key, now, c)
	return nil
}

func WriteMetricAndFlush(w *bufio.Writer, i interface{}, key string, ts int64, c *WavefrontConfig) {
	name, tagStr := DecodeKey(key)
	tagStr += hostTagString(c.HostTags)

	switch metric := i.(type) {
	case metrics.Counter:
		writeCounter(w, metric, name, tagStr, ts, c)
	case metrics.Gauge:
		writeGauge(w, metric, name, tagStr, ts, c)
	case metrics.GaugeFloat64:
		writeGaugeFloat64(w, metric, name, tagStr, ts, c)
	case metrics.Histogram:
		writeHistogram(w, metric, name, tagStr, ts, c)
	case metrics.Meter:
		writeMeter(w, metric, name, tagStr, ts, c)
	case metrics.Timer:
		writeTimer(w, metric, name, tagStr, ts, c)
	}
	w.Flush()
}

func writeCounter(w *bufio.Writer, metric metrics.Counter, name, tagStr string, ts int64, c *WavefrontConfig) {
	if hasDeltaPrefix(name) {
		fmt.Fprintf(w, deltaPoint(metric, name, tagStr, ts, c))
	} else {
		fmt.Fprintf(w, "%s%s.count %d %d %s\n", c.Prefix, name, metric.Count(), ts, tagStr)
	}
}

func writeGauge(w *bufio.Writer, metric metrics.Gauge, name, tagStr string, ts int64, c *WavefrontConfig) {
	fmt.Fprintf(w, "%s%s.value %d %d %s\n", c.Prefix, name, metric.Value(), ts, tagStr)
}

func writeGaugeFloat64(w *bufio.Writer, metric metrics.GaugeFloat64, name, tagStr string, ts int64, c *WavefrontConfig) {
	fmt.Fprintf(w, "%s%s.value %f %d %s\n", c.Prefix, name, metric.Value(), ts, tagStr)
}

func writeHistogram(w *bufio.Writer, metric metrics.Histogram, name, tagStr string, ts int64, c *WavefrontConfig) {
	h := metric.Snapshot()
	ps := h.Percentiles(c.Percentiles)
	fmt.Fprintf(w, "%s%s.count %d %d %s\n", c.Prefix, name, h.Count(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.min %d %d %s\n", c.Prefix, name, h.Min(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.max %d %d %s\n", c.Prefix, name, h.Max(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.mean %.2f %d %s\n", c.Prefix, name, h.Mean(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.std-dev %.2f %d %s\n", c.Prefix, name, h.StdDev(), ts, tagStr)
	for psIdx, psKey := range c.Percentiles {
		key := strings.Replace(strconv.FormatFloat(psKey*100.0, 'f', -1, 64), ".", "", 1)
		fmt.Fprintf(w, "%s%s.%s-percentile %.2f %d %s\n", c.Prefix, name, key, ps[psIdx], ts, tagStr)
	}
}

func writeMeter(w *bufio.Writer, metric metrics.Meter, name, tagStr string, ts int64, c *WavefrontConfig) {
	m := metric.Snapshot()
	fmt.Fprintf(w, "%s%s.count %d %d %s\n", c.Prefix, name, m.Count(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.one-minute %.2f %d %s\n", c.Prefix, name, m.Rate1(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.five-minute %.2f %d %s\n", c.Prefix, name, m.Rate5(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.fifteen-minute %.2f %d %s\n", c.Prefix, name, m.Rate15(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.mean %.2f %d %s\n", c.Prefix, name, m.RateMean(), ts, tagStr)
}

func writeTimer(w *bufio.Writer, metric metrics.Timer, name, tagStr string, ts int64, c *WavefrontConfig) {
	t := metric.Snapshot()
	du := float64(c.DurationUnit)
	ps := t.Percentiles(c.Percentiles)
	fmt.Fprintf(w, "%s%s.count %d %d %s\n", c.Prefix, name, t.Count(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.min %d %d %s\n", c.Prefix, name, t.Min()/int64(du), ts, tagStr)
	fmt.Fprintf(w, "%s%s.max %d %d %s\n", c.Prefix, name, t.Max()/int64(du), ts, tagStr)
	fmt.Fprintf(w, "%s%s.mean %.2f %d %s\n", c.Prefix, name, t.Mean()/du, ts, tagStr)
	fmt.Fprintf(w, "%s%s.std-dev %.2f %d %s\n", c.Prefix, name, t.StdDev()/du, ts, tagStr)
	for psIdx, psKey := range c.Percentiles {
		key := strings.Replace(strconv.FormatFloat(psKey*100.0, 'f', -1, 64), ".", "", 1)
		fmt.Fprintf(w, "%s%s.%s-percentile %.2f %d %s\n", c.Prefix, name, key, ps[psIdx]/du, ts, tagStr)
	}
	fmt.Fprintf(w, "%s%s.one-minute %.2f %d %s\n", c.Prefix, name, t.Rate1(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.five-minute %.2f %d %s\n", c.Prefix, name, t.Rate5(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.fifteen-minute %.2f %d %s\n", c.Prefix, name, t.Rate15(), ts, tagStr)
	fmt.Fprintf(w, "%s%s.mean-rate %.2f %d %s\n", c.Prefix, name, t.RateMean(), ts, tagStr)
}
