// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// paracache provides a thread-safe wrapper for caching arbitrary
// requests based on comparable key (eg. a string)

package paracache

import (
	"container/list"
	"sync"
)

// A Request is passed through to the resolver
// Key() must return a value suitable for mapping this request
// to a map entry in the cache
type Request interface {
	Key() interface{}
}

// StringRequest implements Request and returns itself as a key
type StringRequest string

func (s StringRequest) Key() interface{} {
	return string(s)
}

type result struct {
	request Request
	key     interface{}
	value   interface{}
	err     error
}

type task struct {
	request Request
	key     interface{}
	result  chan *result
}

type job struct {
	request Request
	key     interface{}
}

// Objects implementing the Resolver interface are responsible for returning data for a cache miss
// The resutl will be stored in the cache, or the error passed through to the caller.
type Resolver interface {
	Resolve(request Request) (result interface{}, err error)
}

// ResolverFunc wraps a function in a Resolver interface
type ResolverFunc func(request Request) (result interface{}, err error)

func (rf ResolverFunc) Resolve(request Request) (result interface{}, err error) {
	return rf(request)
}

type cacheEntry struct {
	value interface{}
	el    *list.Element
}

// A Cache object holds a LRU cache of resolver responses.
// It distributes cache misses to a concurrent pool of worker resolvers.
type Cache struct {
	m          sync.RWMutex
	cache      map[interface{}]*cacheEntry
	lru        *list.List
	maxItems   int
	maxWorkers int
	lookup     chan *task
	stop       chan struct{}
	resolver   Resolver
	stats      CacheStats
	statsm     sync.Mutex
}

type CacheStats struct {
	ItemCount       int
	QueriesTotal    int64
	QueriesPeriod   int64
	CacheHitsTotal  int64
	CacheHitsPeriod int64
}

// NewCache creates a new cache using the supplied Resolver to handle cacahe misses
// maxItems specifies the maximum number of items to cache with new entries pushing old
// entries out on an LRU basis.
// maxWorkers speficies the maximum number of concurrent calls to the resolver that may
// be in progress at any one time.
func NewCache(resolver Resolver, maxItems, maxWorkers int) *Cache {
	c := &Cache{
		cache:      make(map[interface{}]*cacheEntry),
		lru:        &list.List{},
		lookup:     make(chan *task),
		stop:       make(chan struct{}),
		resolver:   resolver,
		maxItems:   maxItems,
		maxWorkers: maxWorkers,
	}
	go c.dispatcher()
	return c
}

func (c *Cache) worker(jobs <-chan *job, results chan<- *result) {
	// queries the netacutiy API and returns the result
	for job := range jobs {
		value, err := c.resolver.Resolve(job.request)
		results <- &result{
			request: job.request,
			key:     job.key,
			value:   value,
			err:     err,
		}
	}
}

func (c *Cache) statsHit(reqCount int64, cacheHits int64) {
	c.statsm.Lock()
	c.stats.QueriesTotal += reqCount
	c.stats.QueriesPeriod += reqCount
	if cacheHits > 0 {
		c.stats.CacheHitsTotal += cacheHits
		c.stats.CacheHitsPeriod += cacheHits
	}
	c.statsm.Unlock()
}

// Stats returns a copy of the current cache statistics
func (c *Cache) Stats() CacheStats {
	c.statsm.Lock()
	defer c.statsm.Unlock()
	c.m.RLock()
	cacheLen := len(c.cache)
	c.m.RUnlock()
	stats := c.stats
	stats.ItemCount = cacheLen
	return stats
}

// ResetPeriodStats resets the counters for *Period stats to zero
func (c *Cache) ResetPeriodStats() {
	c.statsm.Lock()
	defer c.statsm.Unlock()
	c.stats.QueriesPeriod = 0
	c.stats.CacheHitsPeriod = 0
}

func (c *Cache) addToCache(key interface{}, value interface{}) {
	c.m.Lock()
	defer c.m.Unlock()
	// evict old if full
	if len(c.cache) >= c.maxItems {
		el := c.lru.Back()
		delete(c.cache, el.Value)
		c.lru.Remove(el)
	}
	el := c.lru.PushFront(key)
	c.cache[key] = &cacheEntry{value: value, el: el}
}

func (c *Cache) touch(key interface{}) {
	c.m.Lock()
	defer c.m.Unlock()

	c.lru.MoveToFront(c.cache[key].el)
}

// Manages a pool of fetchers that query the NetAcutiy API
func (c *Cache) dispatcher() {
	jobs := make(chan *job)
	results := make(chan *result)

	inProgress := make(map[interface{}][]*task)
	for i := 0; i < c.maxWorkers; i++ {
		go c.worker(jobs, results)
	}

	for {
		select {
		case t := <-c.lookup:
			if p := inProgress[t.key]; p != nil {
				inProgress[t.key] = append(p, t)

			} else {
				// check we haven't resolved it since the request was put on the queue
				c.m.RLock()
				el, ok := c.cache[t.key]
				c.m.RUnlock()
				if ok {
					// cache hit
					c.touch(t.key)
					c.statsHit(1, 1)
					t.result <- &result{value: el.value}
					continue
				}

				// Start resolving
				inProgress[t.key] = []*task{t}
				jobs <- &job{request: t.request, key: t.key}
			}

		case r := <-results:
			if r.err == nil {
				// update cache
				c.addToCache(r.key, r.value)
			}

			pc := int64(len(inProgress[r.key]))
			c.statsHit(pc, pc-1)
			// notify watchers of result
			for _, w := range inProgress[r.key] {
				w.result <- r
			}
			delete(inProgress, r.key)

		case <-c.stop:
			close(jobs)
			return
		}
	}
}

// Shutdown stops goroutines used to serve requests.
// Further requests to Resolve() will block for non-cached entries.
func (c *Cache) Shutdown() {
	close(c.stop)
}

// Resolve converts a request into a result and answer using the registed
// Resolver if the entry isn't already cached.
func (c *Cache) Resolve(req Request) (value interface{}, err error) {
	key := req.Key()

	// fast path
	c.m.RLock()
	el, ok := c.cache[key]
	c.m.RUnlock()
	if ok {
		c.touch(key)
		c.statsHit(1, 1)
		return el.value, nil
	}

	t := &task{
		request: req,
		key:     key,
		result:  make(chan *result),
	}
	c.lookup <- t
	result := <-t.result
	return result.value, result.err
}
