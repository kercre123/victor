package testtime

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestPassthrough(t *testing.T) {
	assert := assert.New(t)
	pt := New()
	assert.WithinDuration(time.Now(), pt.Now(), 1*time.Second)
}

func TestNowDelta(t *testing.T) {
	assert := assert.New(t)

	now := time.Now()
	pt := NewTestable()
	pt.WithNowDelta(time.Hour, func() {
		assert.WithinDuration(now.Add(time.Hour), pt.Now(), 1*time.Second)
	})

	assert.WithinDuration(now, pt.Now(), 1*time.Second)
}

func TestNestedNowDelta(t *testing.T) {
	assert := assert.New(t)

	now := time.Now()
	pt := NewTestable()

	pt.WithNowDelta(time.Hour, func() {
		assert.WithinDuration(now.Add(time.Hour), pt.Now(), 1*time.Second)
		pt.WithNowDelta(time.Hour, func() {
			assert.WithinDuration(now.Add(2*time.Hour), pt.Now(), 1*time.Second)
		})
		assert.WithinDuration(now.Add(time.Hour), pt.Now(), 1*time.Second)
	})

	assert.WithinDuration(now, pt.Now(), 1*time.Second)
}

func TestStaticNow(t *testing.T) {
	assert := assert.New(t)

	pt := NewTestable()
	plusHour := time.Now().Add(time.Hour)
	pt.WithStaticNow(plusHour, func() {
		assert.Equal(plusHour, pt.Now())
	})
	assert.WithinDuration(time.Now(), pt.Now(), 1*time.Second)
}

func TestCustomNow(t *testing.T) {
	assert := assert.New(t)

	pt := NewTestable()
	plusHour := time.Now().Add(time.Hour)
	now := func(oldTime NowFunc) time.Time { return plusHour }
	pt.WithCustomNow(now, func() {
		assert.Equal(plusHour, pt.Now())
	})
	assert.WithinDuration(time.Now(), pt.Now(), 1*time.Second)
}

func TestNowPassthru(t *testing.T) {
	assert := assert.New(t)

	pt := New()
	assert.WithinDuration(time.Now(), pt.Now(), 1*time.Second)
}
