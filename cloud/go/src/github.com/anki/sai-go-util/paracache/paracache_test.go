package paracache

import (
	"errors"
	"sync/atomic"
	"testing"
)

func TestCacheMiss(t *testing.T) {
	var req StringRequest

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		req = request.(StringRequest)
		return "value", nil
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	result, err := c.Resolve(StringRequest("foobar"))
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if result != "value" {
		t.Errorf("Incorrect value returned %#v", result)
	}

	if string(req) != "foobar" {
		t.Errorf("Incorrect request submited %#v", req)
	}
}

func TestErrorPassthru(t *testing.T) {
	returnErr := errors.New("Test Error")

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		return "value", returnErr
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	result, err := c.Resolve(StringRequest("foobar"))

	if result != "value" {
		t.Errorf("Incorrect value returned %#v", result)
	}

	if err != returnErr {
		t.Errorf("Incorrect error returned %#v", err)
	}
}

func TestCacheHit(t *testing.T) {
	var callCount int32 = 0

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		atomic.AddInt32(&callCount, 1)
		return "value", nil
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	result, err := c.Resolve(StringRequest("foobar"))
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	result, err = c.Resolve(StringRequest("foobar"))
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if result != "value" {
		t.Errorf("Incorrect value returned %#v", result)
	}

	cc := atomic.LoadInt32(&callCount)
	if cc != 1 {
		t.Errorf("Cache miss callcount=%d", cc)
	}
}

func TestCachePromote(t *testing.T) {
	var callCount int32 = 0

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		atomic.AddInt32(&callCount, 1)
		return "req-" + string(request.(StringRequest)), nil
	})

	c := NewCache(resolver, 2, 1)
	defer c.Shutdown()

	c.Resolve(StringRequest("one"))
	c.Resolve(StringRequest("two"))

	// should now have two entries in the cache/lru - "one" should be at the head
	if len(c.cache) != 2 || c.lru.Len() != 2 {
		t.Fatalf("Bad cache size cachelen=%d lrulen=%d", len(c.cache), c.lru.Len())
	}

	if c.lru.Front().Value != "two" {
		t.Fatalf("LRU front has incorrect value %#v", c.lru.Front().Value)
	}

	// promote on fetch cache hit
	atomic.StoreInt32(&callCount, 0)
	result, _ := c.Resolve(StringRequest("one"))
	cc := atomic.LoadInt32(&callCount)
	if cc != 0 {
		t.Fatal("Unexpected cache miss")
	}

	if result != "req-one" {
		t.Fatalf("Bad value returned %t", result)
	}

	if len(c.cache) != 2 || c.lru.Len() != 2 {
		t.Fatalf("Bad cache size after promotion cachelen=%d lrulen=%d", len(c.cache), c.lru.Len())
	}

	if c.lru.Front().Value != "one" {
		t.Fatalf("LRU front has incorrect value %#v", c.lru.Front().Value)
	}
}

func TestCacheEvict(t *testing.T) {
	var callCount int32 = 0

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		atomic.AddInt32(&callCount, 1)
		return "req-" + string(request.(StringRequest)), nil
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	c.Resolve(StringRequest("one"))
	c.Resolve(StringRequest("two"))

	// should now have only one item in the cache - "two"
	if len(c.cache) != 1 || c.lru.Len() != 1 {
		t.Fatalf("Bad cache size cachelen=%d lrulen=%d", len(c.cache), c.lru.Len())
	}

	if c.lru.Front().Value != "two" {
		t.Fatalf("LRU front has incorrect value %#v", c.lru.Front().Value)
	}

	atomic.StoreInt32(&callCount, 0)
	// should be a cache hit

	result, err := c.Resolve(StringRequest("two"))
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if result != "req-two" {
		t.Errorf("Incorrect value returned %#v", result)
	}

	cc := atomic.LoadInt32(&callCount)
	if cc != 0 {
		t.Error("Unexpected cache miss")
	}
}

// Check that making a request to the dispatcher for an entry that's
// already cached returns the correct result
func TestDispatchRace(t *testing.T) {
	var callCount int32 = 0

	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		atomic.AddInt32(&callCount, 1)
		return "req-" + string(request.(StringRequest)), nil
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	// cache
	c.Resolve(StringRequest("one"))

	tsk := &task{
		request: StringRequest("one"),
		key:     "one",
		result:  make(chan *result),
	}
	c.lookup <- tsk
	result := <-tsk.result

	if result.value != "req-one" {
		t.Errorf("Incorrect value returned %#v", result)
	}

	cc := atomic.LoadInt32(&callCount)
	if cc != 1 {
		t.Fatal("Unxpected cache miss")
	}
}

// Test that concurrent requests for the same request identifier
// only trigger one backend resolution
func TestWaitRequest(t *testing.T) {
	var callCount int32 = 0

	waitChan := make(chan bool)
	resolving := make(chan bool)
	doneChan := make(chan bool)
	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		atomic.AddInt32(&callCount, 1)
		resolving <- true
		<-waitChan
		return "req-" + string(request.(StringRequest)), nil
	})

	c := NewCache(resolver, 1, 1)
	defer c.Shutdown()

	go func() {
		// will block
		result, _ := c.Resolve(StringRequest("one"))
		if result != "req-one" {
			t.Errorf("Incorrect value (1) returned %#v", result)
		}
		doneChan <- true
	}()

	// hit it with a second request that should also block
	go func() {
		// will block
		result, _ := c.Resolve(StringRequest("one"))
		if result != "req-one" {
			t.Errorf("Incorrect value (2) returned %#v", result)
		}
		doneChan <- true
	}()

	// wait for the resolver to be hit
	<-resolving

	// unblock the resolver
	close(waitChan)

	// wait for our calls to Resolve() to complete
	<-doneChan
	<-doneChan

	// Check that the ResolverFunc was only called once
	cc := atomic.LoadInt32(&callCount)
	if cc != 1 {
		t.Error("Incorrect number of calls to ResolverFunc", cc)
	}
}

// Test that the cache respects the maxWorkers setting
func TestConcurrent(t *testing.T) {
	waitChan := make(chan bool)
	resolving := make(chan bool)
	doneChan := make(chan bool)
	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		resolving <- true
		<-waitChan
		return "req-" + string(request.(StringRequest)), nil
	})

	c := NewCache(resolver, 1, 2) // max 2 concurrent workers
	defer c.Shutdown()

	// start the first two requests
	go func() {
		result, _ := c.Resolve(StringRequest("one"))
		if result != "req-one" {
			t.Errorf("Incorrect value (1) returned %#v", result)
		}
		doneChan <- true
	}()

	go func() {
		result, _ := c.Resolve(StringRequest("two"))
		if result != "req-two" {
			t.Errorf("Incorrect value (2) returned %#v", result)
		}
		doneChan <- true
	}()

	// Wait for both calls to the resolver to be active concurrently, but don't unblock them
	<-resolving
	<-resolving

	// Unblock
	waitChan <- true
	waitChan <- true

	// Wait for out tests to complete
	<-doneChan
	<-doneChan
}

func TestStats(t *testing.T) {
	waitChan := make(chan bool)
	releaseChan := make(chan bool)
	resolver := ResolverFunc(func(request Request) (interface{}, error) {
		if string(request.(StringRequest)) == "block" {
			waitChan <- true
			<-releaseChan
			return "block", nil
		}
		return "", nil
	})

	c := NewCache(resolver, 2, 1)
	defer c.Shutdown()

	// cache miss
	c.Resolve(StringRequest("one"))

	s := c.Stats()
	expected := CacheStats{ItemCount: 1, QueriesTotal: 1, QueriesPeriod: 1, CacheHitsTotal: 0, CacheHitsPeriod: 0}
	if s != expected {
		t.Errorf("Incorrect stats %#v", s)
	}

	// cache hit
	c.Resolve(StringRequest("one"))
	s = c.Stats()
	expected = CacheStats{ItemCount: 1, QueriesTotal: 2, QueriesPeriod: 2, CacheHitsTotal: 1, CacheHitsPeriod: 1}
	if s != expected {
		t.Errorf("Incorrect stats (2) %#v", s)
	}

	// second cache miss
	c.Resolve(StringRequest("two"))

	s = c.Stats()
	expected = CacheStats{ItemCount: 2, QueriesTotal: 3, QueriesPeriod: 3, CacheHitsTotal: 1, CacheHitsPeriod: 1}
	if s != expected {
		t.Errorf("Incorrect stats %#v", s)
	}

	c.ResetPeriodStats()
	s = c.Stats()
	expected = CacheStats{ItemCount: 2, QueriesTotal: 3, QueriesPeriod: 0, CacheHitsTotal: 1, CacheHitsPeriod: 0}
	if s != expected {
		t.Errorf("Incorrect stats %#v", s)
	}

	// force dispatcher cache hit
	tsk := &task{
		request: StringRequest("one"),
		key:     "one",
		result:  make(chan *result),
	}
	c.lookup <- tsk
	<-tsk.result

	s = c.Stats()
	expected = CacheStats{ItemCount: 2, QueriesTotal: 4, QueriesPeriod: 1, CacheHitsTotal: 2, CacheHitsPeriod: 1}
	if s != expected {
		t.Errorf("Incorrect stats %#v", s)
	}

	// force multi-request cache hit
	c.ResetPeriodStats()
	rdone := make(chan bool)
	r := func() {
		c.Resolve(StringRequest("block"))
		rdone <- true
	}
	go r()
	go r()
	go r()

	<-waitChan
	releaseChan <- true

	// wait for Resolves to finish
	<-rdone
	<-rdone
	<-rdone

	// should have added 3 queries and 2 cache hits
	s = c.Stats()
	expected = CacheStats{ItemCount: 2, QueriesTotal: 7, QueriesPeriod: 3, CacheHitsTotal: 4, CacheHitsPeriod: 2}
	if s != expected {
		t.Errorf("Incorrect stats %#v", s)
	}

}
