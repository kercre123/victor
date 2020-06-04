// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The apirouter package provides a standard HTTP MUX router setup for
// Anki API services, including compression, log handling, request id
// generation and session validation.
package apirouter

import (
	"fmt"
	"io"
	"log/syslog"
	"net/http"
	_ "net/http/pprof"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/http/corshandler"
	"github.com/anki/sai-go-util/http/httpcapture"
	"github.com/anki/sai-go-util/http/httplog"
	"github.com/anki/sai-go-util/http/memlimit"
	"github.com/anki/sai-go-util/http/metrics/metricshandler"
	"github.com/anki/sai-go-util/http/metrics/profileserver"
	"github.com/anki/sai-go-util/http/ratelimit"
	"github.com/anki/sai-go-util/http/statushandler"
	"github.com/anki/sai-go-util/http/xfwdhandler"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
	"github.com/gorilla/context"
	"github.com/gorilla/handlers"
	"github.com/gorilla/mux"
	"github.com/gwatts/manners"
)

var (
	MuxRouter = mux.NewRouter()

	// Used with the CORS handler.. might want to make this variable
	//AllowedDomains = []string{"anki.com", "*.anki.com", "localhost", "*.jshell.net"}
	CorsAllowedDomains           = envconfig.StringSlice{"anki.com", "*.anki.com", "*.amazonaws.com", "anki.okta.com", "*.ankicore.com", "localhost"}
	CorsAllowedMethods           = envconfig.StringSlice{"GET", "POST", "PUT", "PATCH", "DELETE"}
	CorsAllowedHeaders           = envconfig.StringSlice{"origin", "accept", "Anki-*", "Content-Type", "authorization"}
	CorsMaxAge                   = 1 * time.Minute
	LogWriter          io.Writer = os.Stdout

	TokenRate      = 0.5                       // 0.5 requests per second
	MaxTokens      = int64(TokenRate * 60 * 5) // 0.5 requests/second for 5 minutes
	SweepFrequency = time.Minute

	// App key tokens that are excluded from rate limiting
	AppTokenWhitelist  []string
	RateLimitWhitelist map[string][]string

	// Where to store HTTP capture data
	// capturing is disabled if this is nil.
	CaptureStore       httpcapture.Store
	CaptureServiceName string
)

func init() {
	configureRouter()
}

func configureRouter() {
	envconfig.StringArray(&CorsAllowedDomains, "CORS_DOMAINS", "cors-domains", "CORS allowed domains")
	envconfig.StringArray(&CorsAllowedMethods, "CORS_METHODS", "cors-methods", "CORS allowed methods")
	envconfig.StringArray(&CorsAllowedHeaders, "CORS_HEADERS", "cors-headers", "CORS allowed headers")
	envconfig.Duration(&CorsMaxAge, "CORS_MAX_AGE", "cors-max-age", "Max CORS preflight check age")
}

// Sets AppTokenWhitelist using a semicolon separated list of tokens
func SetAppTokenWhitelist(whitelist string) map[string][]string {
	for _, token := range strings.Split(whitelist, ";") {
		AppTokenWhitelist = append(AppTokenWhitelist, strings.TrimSpace(token))
	}
	if len(AppTokenWhitelist) != 0 {
		RateLimitWhitelist = map[string][]string{
			"Anki-App-Key": AppTokenWhitelist,
		}
	}
	return RateLimitWhitelist
}

// EnableSyslog configures the CombinedLoggingHandler to send its log
// output to syslog using the provided priority and tag.
func EnableSyslog(p syslog.Priority, tag string, enable bool) (err error) {
	if enable {
		LogWriter, err = syslog.New(p, tag)
		return err
	} else {
		LogWriter = os.Stdout
	}
	return nil
}

// for unit tests
func ResetMux() {
	MuxRouter = mux.NewRouter()
}

// A Handler wraps a standard http.Handler with an HTTP method name
// and path to be used to attach it to the mux.
type Handler struct {

	// Method specifies the HTTP method or methods to invoke this
	// handler on. Multiple methods may be specified as a
	// comma-delimited string (i.e. "PUT,POST").
	Method string

	// Path specifies a literal path to invoke this handler on. Path
	// or PathPrefix must be specified, but not both.
	Path string

	// PathPrefix specifies a prefix to invoke this handler on any
	// full paths that match the prefix. A prefix of '/foo' will match
	// '/foobar' as well as '/foo/bar', so a trailing slash should be
	// used if the prefix is intended to match only sub-paths of a
	// specific path node, as delimited by slashes.
	PathPrefix string

	// Handler is the HTTP handler function to be invoked.
	Handler http.Handler
}

func (h *Handler) Register() {
	methods := strings.Split(h.Method, ",")
	route := MuxRouter.NewRoute().Handler(metricshandler.Handler(h.Path, h.Handler)).Methods(methods...)
	if h.Path != "" {
		route.Path(h.Path)
	} else if h.PathPrefix != "" {
		route.PathPrefix(h.PathPrefix)
	}
}

func RegisterHandlers(handlers []Handler) {
	for _, h := range handlers {
		h.Register()
	}
}

// Module defines a set of related API handlers that can be
// initialized and registered together.
//
// Lifecycle
//
// If an API module implements the cli.Configurable interface, its
// Flags() method will be called when it is registered with
// RegisterModule(). If the module implements the cli.Initializable
// interface, its Initialize() method will be called by
// apirouter.Initialize(), which is called by the standard start
// command during API service startup.
type Module interface {
	GetHandlers() []Handler
}

var modules = []Module{}

// RegisterModule registers an API module with the router, and calls
// its Flags() method if it implements the cli.Configurable interface.
func RegisterModule(module Module) {
	modules = append(modules, module)
	if cfg, ok := module.(cli.Configurable); ok {
		cfg.Flags()
	}
}

// Initialize initializes the API router with all the registered API
// modules, registering their handlers and calling the Initialize()
// method on any API module that implements the cli.Initialize
// interface.
func Initialize() error {
	for _, module := range modules {
		RegisterHandlers(module.GetHandlers())

		if init, ok := module.(cli.Initializable); ok {
			err := init.Initialize()
			if err != nil {
				alog.Error{"action": "initialize", "status": "error", "error": err}.Log()
				return err
			}
		}
	}
	return nil
}

// AddStatusHandler registers a StatusChecker with the /status endpoint
func AddStatusHandler(checker statushandler.StatusChecker) {
	handler := statushandler.StatusHandler{StatusChecker: checker}
	MuxRouter.Handle("/status", handler).Methods("GET")
}

type requestIdHandler struct {
	Handler http.Handler
}

func (h *requestIdHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	// Create request id and attach to context
	requestId, err := strutil.ShortLowerUUID()
	context.Set(r, "requestId", requestId)
	w.Header().Add("Anki-Request-Id", requestId)
	if err != nil {
		panic("Failed to generated request id") // TODO improve!
	}

	if clientId := r.Header.Get("Anki-Client-Id"); clientId != "" {
		context.Set(r, "clientId", clientId)
	}
	h.Handler.ServeHTTP(w, r)
}

func RequestId(r *http.Request) string {
	if rid, ok := context.Get(r, "requestId").(string); ok {
		return rid
	}
	return ""
}

// panicHandler captures any panic that occurs in a child handler and logs it
// keeping stack newlines intact and associating it with a requestId
type panicHandler struct {
	Handler      http.Handler
	ClearContext bool
}

func (h *panicHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	defer func() {
		if err := recover(); err != nil {
			alog.Panic{"action": "panic_http_handler", "status": "panic",
				"http_method": r.Method, "http_url": r.URL.String(), "error": err,
			}.LogR(r)
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Internal server error occured.  ErrorId=" + RequestId(r) + "\n"))
		}
		if h.ClearContext {
			context.Clear(r)
		}
	}()

	h.Handler.ServeHTTP(w, r)
}

type logHandler struct {
	httplog.LogHandler
}

const localTimeHeader = "Local-Time"

// Add the runtime in milliseconds and request id to the end of each log entry
func (h logHandler) AppendToLog(buf []byte, req *http.Request, ts time.Time, status int, size int) []byte {
	buf = append(buf, strconv.Itoa(int(time.Since(ts)/time.Millisecond))...)
	buf = append(buf, ' ')
	buf = append(buf, RequestId(req)...)

	ltsWritten := false
	if lts := req.Header.Get(localTimeHeader); lts != "" {
		lt, err := time.Parse(time.RFC1123Z, lts)
		if err == nil {
			buf = append(buf, ' ')
			buf = append(buf, strconv.Itoa(int(lt.Unix()-ts.Unix()))...)
			ltsWritten = true
		}
	}
	if !ltsWritten {
		buf = append(buf, " -"...)
	}

	return buf
}

var lastExceedLogTime = struct {
	sync.Mutex
	t         time.Time
	sinceLast int
}{}

// Log at most once per second if we're rejecting requests due to memory pressure
func logMemoryExceeded() {
	lastExceedLogTime.Lock()
	if time.Since(lastExceedLogTime.t) > time.Second {
		lastExceedLogTime.t = time.Now()
		sinceLast := lastExceedLogTime.sinceLast + 1
		lastExceedLogTime.sinceLast = 0
		lastExceedLogTime.Unlock()
		m := &runtime.MemStats{}
		runtime.ReadMemStats(m)
		alog.Error{
			"action":        "memory_limit_reached",
			"errors_sent":   sinceLast,
			"goroutines":    runtime.NumGoroutine(),
			"mem_in_use":    m.Alloc,
			"mem_allocated": m.TotalAlloc,
			"mem_system":    m.Sys,
			"heap_alloc":    m.HeapAlloc,
			"heap_sys":      m.HeapSys,
			"heap_idle":     m.HeapIdle,
			"heap_inuse":    m.HeapInuse,
			"heap_released": m.HeapReleased,
			"heap_objects":  m.HeapObjects,
			"stack_inuse":   m.StackInuse,
		}.Log()

	} else {
		lastExceedLogTime.sinceLast++
		lastExceedLogTime.Unlock()
	}
}

// type chain just provides an easy way to daisychain http handlers together
// without requiring the explicit name of the previous handler in the chain.
// Make reordering in the code easier.
type chain []http.Handler

func (c *chain) Add(f func(h http.Handler) http.Handler) {
	*c = append(*c, f((*c)[len(*c)-1]))
}

func (c chain) Last() http.Handler {
	return c[len(c)-1]
}

// Wrap the gorilla mux with a chain of handlers to provide
// compression, logging, request ids, rate limiting, etc etc
// returns the base handler ready to be used with a server
//
// If maxHeapMB is greater than zero then an additional handler is inserted
// that will return a 503 error if the server has too more memory currently
// in use.  It will log an error at most once a second if that's the case
// until free memory increases.
func SetupMuxHandlers(maxHeapMB uint64, muxHandler http.Handler) http.Handler {
	if muxHandler == nil {
		muxHandler = MuxRouter
	}
	chain := chain{muxHandler}

	chain.Add(func(h http.Handler) http.Handler {
		return handlers.CompressHandler(h)
	})

	chain.Add(func(h http.Handler) http.Handler {
		return corshandler.New(&corshandler.Config{
			Handler:            h,
			AllowOriginDomains: CorsAllowedDomains,
			AllowedMethods:     CorsAllowedMethods,
			AllowedHeaders:     CorsAllowedHeaders,
			AllowCredentials:   true,
			MaxAge:             CorsMaxAge,
		})
	})

	// apply simple local rate limiting on a per-IP basis
	// one day we might make this co-ordinate via redis or something
	chain.Add(func(h http.Handler) http.Handler {
		return ratelimit.NewIPLimiter(h, TokenRate, MaxTokens, SweepFrequency, RateLimitWhitelist)
	})

	if maxHeapMB > 0 {
		chain.Add(func(h http.Handler) http.Handler {
			return memlimit.NewMemLimiter(h, maxHeapMB*1<<20, logMemoryExceeded)
		})
	}

	// trap the most likely source of panics, in the handlers, at a point they can still also
	// be caught by the logigng handler below
	chain.Add(func(h http.Handler) http.Handler {
		return &panicHandler{Handler: h, ClearContext: false}
	})

	// Setup apache style access logging
	// will use the x-forwarded-for address that was pushed into request.RemoteAddr
	// by the xfwdHandler below
	chain.Add(func(h http.Handler) http.Handler {
		logHandler := logHandler{
			httplog.CombinedLoggingHandler(LogWriter, h),
		}
		logHandler.SuffixFormatter = logHandler
		return logHandler
	})

	if CaptureStore != nil {
		chain.Add(func(h http.Handler) http.Handler {
			return httpcapture.NewHandler(h, CaptureStore, CaptureServiceName)
		})
	}

	// Make sure each request gets a unique request id
	chain.Add(func(h http.Handler) http.Handler {
		return &requestIdHandler{Handler: h}
	})

	// replace request.RemoteAddr with the X-Forwarded-For address
	chain.Add(func(h http.Handler) http.Handler {
		return xfwdhandler.Handler(h)
	})

	// panic handler is resonsible for clearning up the request context
	// and logging any panics that occur
	MuxRouter.KeepContext = true

	// any panics caught here will be logged to the general log, but not to the access log
	chain.Add(func(h http.Handler) http.Handler {
		return &panicHandler{Handler: h, ClearContext: true}
	})

	if len(chain) < 8 {
		panic("http handler chain unexpectedly short!")
	}

	return chain.Last()
}

// Servers wraps the main HTTP server and optionally a pprof profile server as well.
// It ensures the pprof endpoints are only available on a separate port and that
// both servers shutdown gracefully.
type Servers struct {
	MainServer    *manners.GracefulServer
	ProfileServer *manners.GracefulServer
}

func NewServers(baseHandler http.Handler, mainPort, profilingPort int) *Servers {
	if mainPort <= 0 {
		panic("Invalid port supplied to NewServers")
	}
	s := &Servers{
		MainServer: manners.NewWithServer(&http.Server{
			Addr:     fmt.Sprintf(":%d", mainPort),
			Handler:  baseHandler,
			ErrorLog: alog.MakeLogger(alog.LogInfo, "httperror"),
		}),
	}
	if profilingPort > 0 {
		s.ProfileServer = profileserver.NewServer(profilingPort)
	}
	return s
}

// Start starts both the main and pprof servers and blocks until they
// both exit, or one returns an error.
func (s *Servers) Start() error {
	errChan := make(chan error)
	wc := 1
	go func() { errChan <- s.MainServer.ListenAndServe() }()
	if s.ProfileServer != nil {
		wc++
		go func() { errChan <- s.ProfileServer.ListenAndServe() }()
	}
	for i := 0; i < wc; i++ {
		err := <-errChan
		if err != nil {
			return err
		}
	}
	return nil
}

// Close triggers the shutdown of both servers.
func (s *Servers) Close() {
	s.MainServer.Close()
	if s.ProfileServer != nil {
		s.ProfileServer.Close()
	}
}
