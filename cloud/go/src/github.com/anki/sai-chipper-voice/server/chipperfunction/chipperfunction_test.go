package chipperfunction

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

func TestLexTimerParsing(t *testing.T) {
	values := map[string]int64{
		"PT2H":       int64(7200),
		"PT3M":       int64(180),
		"PT30M":      int64(1800),
		"PT30S":      int64(30),
		"PT1S":       int64(1),
		"PT2H30M":    int64(9000),
		"PT2H3M":     int64(7380),
		"PT10H30M":   int64(37800),
		"PT10H3M":    int64(36180),
		"PT2H3S":     int64(7203),
		"PT2H30S":    int64(7230),
		"PT12H31M3S": int64(45063),
		"PT2H31M30S": int64(9090),
		"PT2H4M3S":   int64(7443),
		"PT2H4M31S":  int64(7471),
		"PT3M2S":     int64(182),
		"PT30M2S":    int64(1802),
		"PT3M25S":    int64(205),
		"PT30M25S":   int64(1825),
	}

	for k, v := range values {
		timer := parseLexDuration(k)
		assert.Equal(t, v, timer)
	}
}
