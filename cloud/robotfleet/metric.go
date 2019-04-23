package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"time"

	"github.com/rcrowley/go-metrics"
	"github.com/wavefronthq/go-metrics-wavefront"
)

const (
	metricSource = "robot_fleet"
	metricPrefix = "go.service.robotfleet"
)

type actionMetrics struct {
	// Note: we can make this a metrics.Gauge to reduce reportinh from 14 to 1 datapoint per measurement
	//       at the expense of time resolution
	latency metrics.Timer
	errors  metrics.Counter
}

func newActionMetrics(name string, enabled bool) *actionMetrics {
	if !enabled {
		return &actionMetrics{}
	}

	return &actionMetrics{
		latency: metrics.NewRegisteredTimer(fmt.Sprintf("%s.latency", name), metrics.DefaultRegistry),
		errors:  metrics.NewRegisteredCounter(fmt.Sprintf("%s.errors", name), metrics.DefaultRegistry),
	}
}

func (m *actionMetrics) reportActionMetrics(latency time.Duration, err error) {
	if m.latency != nil {
		m.latency.Update(latency)
	}

	if m.errors != nil && err != nil {
		m.errors.Inc(1)
	}
}

type actionMetricRegistry struct {
	actionMap map[string]*actionMetrics
}

func newActionMetricRegistry() *actionMetricRegistry {
	return &actionMetricRegistry{actionMap: make(map[string]*actionMetrics)}
}

func (m *actionMetricRegistry) getActionMetrics(actionID string) *actionMetrics {
	return m.actionMap[actionID]
}

func (m *actionMetricRegistry) registerActionMetrics(actionID, metricName string, enabled bool) *actionMetrics {
	if m.actionMap[actionID] == nil {
		m.actionMap[actionID] = newActionMetrics(metricName, enabled)
	}

	return m.actionMap[actionID]
}

func setupMetrics(proxyAddress string, taskID int, reportingInterval time.Duration) {
	if proxyAddress == "" || reportingInterval == 0 {
		fmt.Printf("Disabling WaveFront metrics, reporting to stdout instead (with reporting interval %v)\n", reportingInterval)

		go metrics.Log(metrics.DefaultRegistry, reportingInterval, log.New(os.Stdout, "metrics: ", log.Lmicroseconds))

		return
	}

	hostTags := map[string]string{
		"source":  metricSource,
		"task_id": strconv.Itoa(taskID),
	}

	tcpAddress, err := net.ResolveTCPAddr("tcp", proxyAddress)
	if err != nil {
		fmt.Printf("Could not resolve WaveFront proxy address %q (error=%v)\n", proxyAddress, err)
		return
	}

	fmt.Printf("Sending metrics to WaveFront on socket %q (%q) with reporting interval %v\n", proxyAddress, tcpAddress, reportingInterval)

	go wavefront.WavefrontProxy(metrics.DefaultRegistry, reportingInterval, hostTags, metricPrefix, tcpAddress)
}
