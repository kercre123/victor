package wavefront

import (
	"fmt"
	"strings"
	"unicode/utf8"

	"github.com/rcrowley/go-metrics"
)

var (
	deltaPrefix           = "\u2206"
	altDeltaPrefix        = "\u0394"
	_, deltaPrefixSize    = utf8.DecodeRuneInString(deltaPrefix)
	_, altDeltaPrefixSize = utf8.DecodeRuneInString(altDeltaPrefix)
)

// Gets a delta counter name prefixed with ∆.
// Can be used as an input for RegisterMetric() or GetOrRegisterMetric() functions
func DeltaCounterName(name string) string {
	if hasDeltaPrefix(name) {
		return name
	}
	return deltaPrefix + name
}

func hasDeltaPrefix(name string) bool {
	return strings.HasPrefix(name, deltaPrefix) || strings.HasPrefix(name, altDeltaPrefix)
}

func deltaPoint(metric metrics.Counter, name, tagStr string, ts int64, c *WavefrontConfig) string {
	// handle UTF-8 byte encoding of delta prefix
	var prunedName string
	if strings.HasPrefix(name, deltaPrefix) {
		prunedName = name[deltaPrefixSize:]
	} else if strings.HasPrefix(name, altDeltaPrefix) {
		prunedName = name[altDeltaPrefixSize:]
	}
	value := metric.Count()
	metric.Dec(value)

	// add ∆ to prefix and remove from metric name
	if ts == 0 {
		return fmt.Sprintf("%s%s.count %d %s\n", deltaPrefix+c.Prefix, prunedName, value, tagStr)
	}
	return fmt.Sprintf("%s%s.count %d %d %s\n", deltaPrefix+c.Prefix, prunedName, value, ts, tagStr)
}
