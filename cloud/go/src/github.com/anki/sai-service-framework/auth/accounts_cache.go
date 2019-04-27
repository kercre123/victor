package auth

import (
	"time"

	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
	"github.com/patrickmn/go-cache"
)

const (
	DefaultTTL             = 30 * time.Minute
	DefaultCleanupInterval = 1 * time.Minute
)

var (
	countCacheHit    = metricsutil.GetCounter("accounts.cache.hit", authMetrics)
	countCacheMiss   = metricsutil.GetCounter("accounts.cache.miss", authMetrics)
	countCacheUpdate = metricsutil.GetCounter("accounts.cache.update", authMetrics)
)

// AccountsCredentialsCache defines a cache for caching account
// credentials based on an appkey and session. Because cache
// implementations often have background go routines to manage
// expiration that need to be stopped on shutdown, CredentialsCache
// implements the svc.ServiceObject interface, to ensure caches always
// get started & stopped correctly.
type AccountsCredentialsCache struct {
	enabled         *bool
	cacheTTL        *svc.Duration
	cleanupInterval *svc.Duration
	cache           *cache.Cache
}

type AccountsCredentialsCacheOpt func(c *AccountsCredentialsCache)

func WithCacheTTL(ttl time.Duration) AccountsCredentialsCacheOpt {
	return func(c *AccountsCredentialsCache) {
		value := svc.Duration(ttl)
		c.cacheTTL = &value
	}
}

func WithCacheCleanupInterval(interval time.Duration) AccountsCredentialsCacheOpt {
	return func(c *AccountsCredentialsCache) {
		value := svc.Duration(interval)
		c.cleanupInterval = &value
	}
}

func WithCacheEnabled(enabled bool) AccountsCredentialsCacheOpt {
	return func(c *AccountsCredentialsCache) {
		c.enabled = &enabled
	}
}

func NewAccountsCredentialsCache(opts ...AccountsCredentialsCacheOpt) *AccountsCredentialsCache {
	var (
		defaultTTL     = svc.Duration(DefaultTTL)
		defaultCleanup = svc.Duration(DefaultCleanupInterval)
		enabled        = true
	)
	c := &AccountsCredentialsCache{
		enabled:         &enabled,
		cacheTTL:        &defaultTTL,
		cleanupInterval: &defaultCleanup,
	}
	for _, opt := range opts {
		opt(c)
	}
	if *c.enabled {
		c.cache = cache.New(c.cacheTTL.Duration(), c.cleanupInterval.Duration())
	}
	return c
}

func (c *AccountsCredentialsCache) CommandSpec() string {
	return "[--accounts-cache-enabled] [--accounts-cache-ttl] [--accounts-cache-cleanup-interval]"
}

func (c *AccountsCredentialsCache) CommandInitialize(cmd *cli.Cmd) {
	c.enabled = svc.BoolOpt(cmd, "accounts-cache-enabled", true,
		"Enable caching of credentials from the Accounts service")
	c.cacheTTL = svc.DurationOpt(cmd, "accounts-cache-ttl", DefaultTTL,
		"Length of time to cache credentials for")
	c.cleanupInterval = svc.DurationOpt(cmd, "accounts-cache-cleanup-interval", DefaultCleanupInterval,
		"Interval at which to sweep the cache for expired items")
}

func (c *AccountsCredentialsCache) Get(appkey, session string) *AccountsCredentials {
	if c.cache != nil {
		if value, ok := c.cache.Get(c.cacheKey(appkey, session)); ok {
			if creds, ok := value.(*AccountsCredentials); ok {
				countCacheHit.Inc(1)
				return creds
			}
		}
		countCacheMiss.Inc(1)
	}
	return nil
}

func (c *AccountsCredentialsCache) Set(appkey, session string, creds *AccountsCredentials) {
	if c.cache != nil {
		countCacheUpdate.Inc(1)
		ttl := cache.DefaultExpiration // use configured expiration
		if appkey != "" && session == "" {
			// appkey-only, these don't need to expire
			ttl = cache.NoExpiration
		}
		c.cache.Set(c.cacheKey(appkey, session), creds, ttl)
	}
}

func (c *AccountsCredentialsCache) cacheKey(appkey, session string) string {
	return appkey + session
}
