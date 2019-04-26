// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// A simple IP based token bucket rate limiter.
package ratelimit

import (
	"net/http"
	"sync"
	"time"

	"github.com/juju/ratelimit"
)

// Bucketer defines an interface to define how requests should be grouped
// together for rate limiting purposes.  It must return a hashable value
// eg. an ip address.
type Bucketer interface {
	BucketId(w http.ResponseWriter, r *http.Request) interface{}
}

// LimitReporter defines an optional interface for bucketers that will
// be called whenever a request is throttled.
type LimitReporter interface {
	// exceedCount holds the number of sequential failures to get a token
	RequestLimited(r *http.Request, bucketId interface{}, exceedCount int)
}

type IPLimitBucketer struct{}

func (b IPLimitBucketer) BucketId(w http.ResponseWriter, r *http.Request) interface{} {
	return r.RemoteAddr
}

// RateLimiter wraps an HTTP handler and applies a non-shared rate
// limit on a per-IP basis.
type RateLimiter struct {
	m                 sync.Mutex
	buckets           map[interface{}]*bucket
	bucketer          Bucketer
	sweepFrequency    time.Duration
	rate              float64
	capacity          int64
	whitelistByHeader map[string][]string
	shutdown          chan struct{}
	handler           http.Handler
}

// NewLimiter returns a new RateLimiter
//
// handler is the HTP handler to wrap
// rate is the maximum number of tokens/second/ip (1 request uses 1 token)
// capacity is the initial capacity of the token bucket per ip
// sweepfrequency determines how often the background task clears idle IP addresses
// from the tracking map to avoid leaking memory
// whitelist is an optional list of ip addresses the limit should not be applied to
//
// Picking a rate of 10 and a capacity of 120, for example, would mean that a client
// could average 10 requests/second over a 2 minute period before being rate limited.
func NewLimiter(bucketer Bucketer, handler http.Handler, rate float64, capacity int64, sweepFrequency time.Duration, whitelistByHeader map[string][]string) *RateLimiter {
	i := &RateLimiter{
		buckets:           make(map[interface{}]*bucket),
		bucketer:          bucketer,
		handler:           handler,
		rate:              rate,
		capacity:          capacity,
		whitelistByHeader: whitelistByHeader,
		sweepFrequency:    sweepFrequency,
		shutdown:          make(chan struct{}),
	}
	go i.sweeper()
	return i
}

func NewIPLimiter(handler http.Handler, rate float64, capacity int64, sweepFrequency time.Duration, whitelistByHeader map[string][]string) *RateLimiter {
	return NewLimiter(IPLimitBucketer{}, handler, rate, capacity, sweepFrequency, whitelistByHeader)
}

type bucket struct {
	*ratelimit.Bucket
	lastSeen    time.Time
	exceedCount int
}

func (b *bucket) TakeOne() (ok bool, exceedCount int) {
	if b.TakeAvailable(1) == 1 {
		// got token
		b.exceedCount = 0
		return true, 0
	}
	b.exceedCount++
	return false, b.exceedCount
}

// This is only safe to call before the http server has been started.
func (i *RateLimiter) SetWhitelist(w map[string][]string) {
	i.whitelistByHeader = w
}

func (i *RateLimiter) Close() {
	close(i.shutdown)
}

func (i *RateLimiter) maxRefillTime() time.Duration {
	return time.Duration(float64(i.capacity) / i.rate * float64(time.Second))
}

func (i *RateLimiter) sweeper() {
	for {
		select {
		case <-time.After(i.sweepFrequency):
			i.sweep()
		case <-i.shutdown:
			return
		}
	}
}

func (i *RateLimiter) sweep() {
	// any buckets that haven't been touched since before maxRefillTIme are guaranteed
	// to be full.
	expireBefore := time.Now().Add(-i.maxRefillTime())
	i.m.Lock()
	defer i.m.Unlock()

	for k, b := range i.buckets {
		if b.lastSeen.Before(expireBefore) {
			// this is safe to do within the loop, according to the language spec
			delete(i.buckets, k)
		}
	}
}

func (i *RateLimiter) getBucketForRequest(w http.ResponseWriter, r *http.Request) (b *bucket, bid interface{}) {
	bid = i.bucketer.BucketId(w, r)
	if bid == nil {
		// bucketer has declined to apply rate limiting to the request
		return nil, nil
	}
	i.m.Lock()
	defer i.m.Unlock()
	b = i.buckets[bid]
	if b == nil {
		b = &bucket{
			Bucket: ratelimit.NewBucketWithRate(i.rate, i.capacity),
		}
		i.buckets[bid] = b
	}
	b.lastSeen = time.Now()
	return b, bid
}

func (i *RateLimiter) isWhitelisted(r *http.Request) bool {
	for k, vals := range i.whitelistByHeader {
		if header := r.Header.Get(k); header != "" {
			for _, val := range vals {
				if header == val {
					return true
				}
			}
		}
	}
	return false
}

func (i *RateLimiter) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if !i.isWhitelisted(r) {
		b, bid := i.getBucketForRequest(w, r)
		if b != nil {
			if ok, exceedCount := b.TakeOne(); !ok {
				http.Error(w, "Rate limit exceeded", 429)
				if n, ok := i.bucketer.(LimitReporter); ok {
					n.RequestLimited(r, bid, exceedCount)
				}
				return
			}
		}
	}
	i.handler.ServeHTTP(w, r)
}
