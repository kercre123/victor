package requestrouter

import (
	"context"
	"errors"
	"fmt"
	"math/rand"
	"strconv"
	"sync"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/patrickmn/go-cache"

	"github.com/anki/sai-go-util/log"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

const (
	minSleepInterval             = 60 * time.Second
	sleepIntervalRange           = 30 * time.Second
	DefaultFWString              = "default"
	DefaultDasRobotVersionString = ""

	// DefaultRobotId is assign to a request if we cannot convert RobotId to a decimal
	DefaultRobotId = 1

	// VersionCacheExpiration sets the expiration time for VersionCache
	VersionCacheExpiration = 30 * time.Minute
	VersionCacheCleanup    = time.Hour

	// DefaultDialogflowVersion is an empty string.
	// In DF, this means we hit the "draft" version, which is the latest version.
	DefaultDialogflowVersion = ""

	// DefaultLexAlias for default Lex version
	DefaultLexAlias = "lex_default" // underscore only for lex
)

var (
	ErrorNoVersionsFound = errors.New("No intent-version found for this firmware")
)

// RequestRouter
// keeps an up-to-date routing ratio, query route-store to find an intent-version for a fw version
// Details: https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/450297910/Chipper+Request+Routing
//
type RequestRouter struct {
	Store *RouteStore

	// Parameter is the hash key for routing-ratio
	Parameter string

	// Interval specifies time to wait before querying for parameter value again
	Interval time.Duration

	// Ratio is the most current ratio stored in parameters table
	Ratio Ratio

	// VersionCache stores a cache of fw-version to intent-version map retrieved from RouteStore
	VersionCache *cache.Cache

	// DialogflowEnabled indicates we have DF enabled, should always be true
	DialogflowEnabled bool

	// LexEnabled indicates we have Lex enabled, could be false
	LexEnabled bool

	// stop signal that client is stopping, exit loop
	stop chan struct{}

	// ready indicates that router start-up is done
	ready chan bool

	// wg waits for loop to end
	wg sync.WaitGroup

	// isRunning indicates if router has been started
	isRunning bool

	sync.RWMutex
}

type RequestRouteInput struct {
	// Requested is the service indicated in the request, usually it's pb.IntentService_DEFAULT
	Requested pb.IntentService

	// LangCode is the language specified in the request
	LangCode pb.LanguageCode

	// Mode is the robot mode, voice-command or games
	Mode pb.RobotMode

	// RobotId is a hex string
	RobotId string

	// FwVersion is a normalized firmware version string with this format "xx.xx.xx"
	// Note, development string is likely to be "00.0x.xx"
	FwVersion string
}

func (r *RequestRouteInput) ToLogValues() map[string]interface{} {
	return map[string]interface{}{
		"_req_svc":  r.Requested.String(),
		"_req_lang": r.LangCode.String(),
		"_req_mode": r.Mode.String(),
		"_device":   r.RobotId,
		"_fw":       r.FwVersion,
	}
}

// NewRouter creates a new request router
func New(prefix string, sleepInterval *time.Duration, cfg *aws.Config) *RequestRouter {
	var store *RouteStore = nil
	var err error
	if cfg != nil {
		store, err = NewRouteStore(prefix, cfg)
		if err != nil {
			// Log a warning. request router can still work without route store
			alog.Warn{
				"action": "create_route_store",
				"status": alog.StatusFail,
				"error":  err,
				"msg":    "fail to create route store for new request router",
			}.Log()
		}
	}

	if sleepInterval == nil {
		interval := getSleepInterval()
		sleepInterval = &interval
	}

	return &RequestRouter{
		Store:        store,
		Parameter:    ParameterRatio,
		Interval:     *sleepInterval,
		Ratio:        DefaultRatio,
		VersionCache: cache.New(VersionCacheExpiration, VersionCacheCleanup),
		stop:         make(chan struct{}),
		ready:        make(chan bool, 1),
		isRunning:    false,
	}

}

// CreateDefaultVersions creates some defaults for dialogflow and lex bots
func CreateDefaultVersions(
	dfProj, dfCred, dfGamesProj, dfGamesCred string,
	lexBotName, lexGamesBotName string) []FirmwareIntentVersion {

	var versions []FirmwareIntentVersion

	for _, lang := range []string{"en-us", "en-gb", "en-au"} {
		// voice command
		versions = append(versions,
			*CreateVersion(
				GetPbServiceName(pb.IntentService_DIALOGFLOW),
				lang, "vc", DefaultFWString,
				dfProj, DefaultDialogflowVersion, dfCred,
				"", "", true),
		)

		// games project
		versions = append(versions,
			*CreateVersion(
				GetPbServiceName(pb.IntentService_DIALOGFLOW),
				lang, "game", DefaultFWString,
				dfGamesProj, DefaultDialogflowVersion, dfGamesCred,
				"", "", true),
		)
	}

	// add lex

	versions = append(versions,
		*CreateVersion(
			GetPbServiceName(pb.IntentService_LEX),
			"en-us", "vc", DefaultFWString,
			"", "", "",
			lexBotName, DefaultLexAlias, true),
	)

	versions = append(versions,
		*CreateVersion(
			GetPbServiceName(pb.IntentService_LEX),
			"en-us", "game", DefaultFWString,
			"", "", "",
			lexGamesBotName, DefaultLexAlias, true),
	)

	return versions
}

// Init adds some default versions to the cache, these never expires
func (r *RequestRouter) Init(dfEnabled, lexEnabled bool, versions ...FirmwareIntentVersion) {
	for _, v := range versions {
		cacheKey := getVersionCacheKey(
			GetPbServiceName(v.IntentVersion.IntentSvc),
			v.Lang,
			v.Mode,
			v.FwVersion)

		r.VersionCache.Set(cacheKey, v, cache.NoExpiration)
	}

	r.DialogflowEnabled = dfEnabled
	r.LexEnabled = lexEnabled

	alog.Info{
		"action":             "init_request_router",
		"status":             "added_default_versions",
		"num":                len(versions),
		"dialogflow_enabled": r.DialogflowEnabled,
		"lex_enabled":        r.LexEnabled,
	}.Log()

}

// Start loop goroutine
func (r *RequestRouter) Start() {
	r.Lock()
	running := r.isRunning
	r.isRunning = true
	r.Unlock()

	if !running {
		if r.Store != nil {
			r.wg.Add(1)
			go r.loop()
			alog.Debug{
				"action": "start_request_router",
				"status": alog.StatusOK,
				"cache":  r.VersionCache.Items(),
			}.Log()
		} else {
			alog.Warn{
				"action": "start_request_router",
				"status": alog.StatusOK,
				"msg":    "route-store is nil, start with only defaults",
			}.Log()
			r.ready <- true
		}
	} else {
		alog.Debug{
			"action": "start_request_router",
			"status": alog.StatusFail,
			"msg":    "request router is already running",
		}.Log()
	}
}

// Ready returns a channel indicating that the router is started and ready
func (r *RequestRouter) Ready() <-chan bool {
	return r.ready
}

// Stop the loop
func (r *RequestRouter) Stop() {
	r.Lock()
	running := r.isRunning
	r.isRunning = false
	r.Unlock()

	if running && r.Store != nil {
		// send stop signal to loop
		alog.Debug{"action": "send_stop_signal"}.Log()
		close(r.stop)
		time.Sleep(time.Millisecond)

		// wait for loop to complete
		alog.Debug{"action": "wait_loop_end"}.Log()
		r.wg.Wait()
	}
}

// chooseIntentService picks the the 3rd-party service to use
func chooseIntentService(
	ctx context.Context,
	input RequestRouteInput,
	ratio RatioValue,
	dfEnabled, lexEnabled bool) pb.IntentService {

	// if language is not en-us, only use Dialogflow
	// future todo: failovers allowed to use lex
	if input.LangCode != pb.LanguageCode_ENGLISH_US {
		return pb.IntentService_DIALOGFLOW
	}

	// if request specify a specific service, return what the request asks for
	if input.Requested == pb.IntentService_DIALOGFLOW && dfEnabled {
		return pb.IntentService_DIALOGFLOW
	}

	if input.Requested == pb.IntentService_LEX {
		if lexEnabled {
			return pb.IntentService_LEX
		}
		alog.Error{"action": "request_lex_routing",
			"status": alog.StatusError,
			"msg":    "Lex is not enabled!!",
		}.Log()
	}

	// hash robotId to number between 0, 99
	hashedRobotId, err := strconv.ParseInt(input.RobotId, 16, 64)
	if err != nil {
		alog.Warn{
			"action":   "hash_robot_id",
			"status":   alog.StatusError,
			"msg":      "robot id is not a hex string",
			"robot_id": input.RobotId,
		}.LogC(ctx)

		// non-hex robotid goes to Dialogflow
		hashedRobotId = DefaultRobotId

	}

	// choose between Dialogflow and Lex. MS is not ready as of 08/31/2018.
	mod := int(hashedRobotId % maxRatioSum)

	alog.Debug{
		"action":    "choose_service",
		"device":    input.RobotId,
		"hashed_id": hashedRobotId,
		"mod":       mod,
	}.LogC(ctx)

	if lexEnabled && mod >= ratio.Dialogflow {
		return pb.IntentService_LEX
	}
	return pb.IntentService_DIALOGFLOW
}

// Route returns a proper Version given the inputs
func (r *RequestRouter) Route(ctx context.Context, input RequestRouteInput) (*FirmwareIntentVersion, error) {

	// pick an intent service
	s := chooseIntentService(ctx, input, r.Ratio.Value, r.DialogflowEnabled, r.LexEnabled)
	alog.Info{"action": "choose_service", "svc": s.String()}.LogC(ctx)

	// look for the last enabled version for the service
	svcName := GetPbServiceName(s)
	langString := LangCodeName[input.LangCode]
	modeString := ModeShortName[input.Mode]

	// check if we have this version in cache
	cacheKey := getVersionCacheKey(svcName, langString, modeString, input.FwVersion)
	if version, found := r.VersionCache.Get(cacheKey); found {
		v := version.(FirmwareIntentVersion)

		alog.Info{
			"action":     "route_request",
			"status":     alog.StatusOK,
			"device":     input.RobotId,
			"fw_version": input.FwVersion,
			"cache_key":  cacheKey,
			"routed":     &v,
			"msg":        "found version in cache",
		}.LogC(ctx)

		return &v, nil
	}

	alog.Debug{"action": "cache_not_found", "cache_key": cacheKey}.Log()

	// if fw string is valid, check dynamodb, otherwise return default
	if ok := validFwString(input.FwVersion); ok && r.Store != nil {

		// get version from route store (dynamodb)
		svcHash, err := GetServiceHashKey(svcName, langString, modeString)
		if err != nil {
			alog.Warn{
				"action": "route_request",
				"status": alog.StatusError,
				"error":  err,
				"msg":    "Fail to get hash key to query route-store",
			}.LogC(ctx)

		} else {

			alog.Info{
				"action":   "get_version_from_dynamo",
				"hash_key": svcHash.Key,
				"sort_key": input.FwVersion,
			}.LogC(ctx)

			newVersion, err := r.Store.GetLastEnabledVersion(ctx, svcHash.Key, input.FwVersion)
			if err != nil {
				alog.Warn{
					"action":     "route_request",
					"status":     alog.StatusError,
					"error":      err,
					"msg":        "Fail to get version from route-store",
					"fw_version": input.FwVersion,
					"hash_key":   svcHash.Key,
				}.LogC(ctx)
			} else {
				// found a version, add to cache
				expiration := cache.DefaultExpiration
				if newVersion.FwVersion == input.FwVersion {
					expiration = cache.NoExpiration
				}
				r.VersionCache.Set(cacheKey, *newVersion, expiration)

				alog.Info{
					"action":     "route_request",
					"status":     alog.StatusOK,
					"device":     input.RobotId,
					"fw_version": input.FwVersion,
					"hash_key":   svcHash.Key,
					"routed":     &newVersion,
					"msg":        "found version in route-store, add to cache",
				}.LogC(ctx)

				return newVersion, nil
			}
		}
	}

	// nothing found, use default
	alog.Debug{"action": "route_request", "status": "use_default"}.LogC(ctx)

	defaultCacheKey := getVersionCacheKey(svcName, langString, modeString, DefaultFWString)
	version, found := r.VersionCache.Get(defaultCacheKey)
	if !found {
		alog.Warn{
			"action":    "route_request",
			"status":    alog.StatusError,
			"msg":       "default missing, return error",
			"fw":        input.FwVersion,
			"device":    input.RobotId,
			"cache_key": defaultCacheKey,
		}.LogC(ctx)

		return nil, ErrorNoVersionsFound
	}

	logEntry := alog.Info{
		"action":       "route_request",
		"status":       alog.StatusOK,
		"device":       input.RobotId,
		"cache_key":    cacheKey,
		"fw_version":   input.FwVersion,
		"msg":          "no version found, use default version",
		"add_to_cache": false,
	}

	v := version.(FirmwareIntentVersion)

	// add to cache so we won't hit dynamo, set to expire in 30 mins
	if defaultCacheKey != cacheKey {
		r.VersionCache.Set(cacheKey, v, cache.DefaultExpiration)
		logEntry["add_to_cache"] = true
	}

	logEntry.LogC(ctx)
	return &v, nil

}

// getVersionCacheKey return the key to VersionCache. e.g. "dialogflow_en-us_vc-0.12"
func getVersionCacheKey(svc, fw, lang, mode string) string {
	key := fmt.Sprintf("%s_%s_%s_%s", svc, lang, mode, fw)
	alog.Debug{"action": "get_version_cache_key", "key": key}.Log()
	return key
}

// getSleepInterval generates a random sleep time between 60 to 90 seconds (in ms)
func getSleepInterval() time.Duration {
	gen := rand.New(rand.NewSource(time.Now().UnixNano()))

	r := gen.Int63n(int64(sleepIntervalRange.Seconds() * 1000)) // random milliseconds
	interval := minSleepInterval + time.Duration(r*int64(time.Millisecond))

	alog.Info{
		"action":   "get_sleep_interval",
		"interval": interval,
		"random":   r,
	}.Log()
	return interval
}

// loop periodically queries for any changes in Ratio
func (r *RequestRouter) loop() {
	defer r.wg.Done()
	updateTimer := time.NewTimer(r.Interval)
	defer updateTimer.Stop()

	updateRatio := func() {
		if r.Store == nil {
			// for some reasons, we have no access to dynamo
			// TODO: log it as an error??
			return
		}

		ratio, err := r.Store.GetRecentRatios(context.Background(), 1)
		if err != nil {
			alog.Error{
				"action": "router_update_ratio",
				"status": alog.StatusError,
				"err":    err,
				"msg":    "Fail to update ratio",
			}.Log()
			return
		}

		if len(ratio) > 0 {
			r.Ratio = *ratio[0]

		} else {
			alog.Warn{
				"action": "router_update_ratio",
				"status": alog.StatusError,
				"msg":    "No ratio retrieved, weird",
			}.Log()
		}
	}

	updateRatio()
	alog.Debug{"action": "start_router", "ratio": r.Ratio.String()}.Log()
	r.ready <- true

	for {
		select {
		case <-r.stop:
			alog.Info{
				"action": "router_stop",
				"status": "exiting_loop",
			}.Log()
			return
		case <-updateTimer.C:
			updateRatio()
			updateTimer.Reset(r.Interval)
		}
	}
}
