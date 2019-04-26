package apirouter

import (
	"os"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-go-util/http/httpcapture"
	"github.com/anki/sai-go-util/log"
)

type StartCommand struct {
	ServiceName       string // eg. "sai-go-storegate"
	PortNum           int
	ProfileMode       string // cpu, block or mem
	ProfilePath       string
	ProfileServerPort int // if non zero then start a profiling server
	MaxMemoryMB       int // stop serving new HTTP requests if memory limit reached

	CaptureEnable      bool
	CaptureStoreType   string
	CaptureStoreTarget string
	CaptureAppKey      string
}

type RunCfg struct {
	StartupLogEntries func() map[string]interface{}
	PreStartServer    func() error
	PostStopServer    func() error
}

func DefaultStartCommand() cli.Command {
	return &StartCommand{
		ServiceName:        os.Args[0],
		PortNum:            5000,
		ProfilePath:        "/tmp",
		CaptureStoreType:   "/file",
		CaptureStoreTarget: os.TempDir(),
	}
}

func (c *StartCommand) Flags() {
	envconfig.Int(&c.PortNum, "HTTP_PORT", "port", "The port number for the service to listen on")
	envconfig.String(&c.ProfileMode, "PROFILE_MODE", "profile-mode", "Capture profiling information.  Can be set to cpu, mem or block")
	envconfig.String(&c.ProfilePath, "PROFILE_PATH", "profile-path", "Directory to write profiling output to")
	envconfig.Int(&c.ProfileServerPort, "PROFILE_PORT", "profile-port", "If set then an HTTP profile server will be started on the supplied port number")
	envconfig.Int(&c.MaxMemoryMB, "MAX_MEMORY_MB", "", "If set then requests return a 503 error if the server is using too much memory")

	envconfig.Bool(&c.CaptureEnable, "CAPTURE_ENABLE", "", "If true then enable full HTTP request/response capture")
	envconfig.String(&c.CaptureStoreType, "CAPTURE_STORE_TYPE", "", `"file" or "blobstore"`)
	envconfig.String(&c.CaptureStoreTarget, "CAPTURE_STORE_TARGET", "", "For file type, the directory to write to.  For blobstore, the server URL")
	envconfig.String(&c.CaptureAppKey, "CAPTURE_STORE_APPKEY", "", "App key to use with the blobstore")
}

func (c *StartCommand) RunWithConfig(args []string, cfg *RunCfg) {
	// setup the http access log target
	LogWriter = cli.NewAccessLogWriter("")
	logEntry := alog.Info{
		"action":        "startup",
		"status":        "starting",
		"port":          c.PortNum,
		"max_memory_mb": c.MaxMemoryMB,
	}
	for k, v := range buildinfo.Map() {
		logEntry[k] = v
	}

	err := c.setupCaptureStore(logEntry)
	if err != nil {
		alog.Error{"action": "startup", "status": "invalid_capture_store", "error": err}.Log()
		return
	}

	baseHandler := SetupMuxHandlers(uint64(c.MaxMemoryMB), nil)

	err = Initialize()
	if err != nil {
		alog.Error{
			"action": "startup",
			"status": "error",
			"msg":    "Error initializing API modules",
			"error":  err,
		}.Log()
		return
	}

	// log some runtime stats once a minute
	cli.StartRuntimeLogger()

	// setup profiling
	profiler := cli.StartProfiler(c.ProfilePath, c.ProfileMode)
	if profiler != nil {
		defer profiler.Stop()
	}

	// Create the HTTP server(s)
	servers := NewServers(baseHandler, c.PortNum, c.ProfileServerPort)

	// Add a signal handler
	cli.StartSigWatcher(func(sig os.Signal) {
		alog.Info{"action": "shutdown", "status": "shutting_down"}.Log()
		servers.Close()
	})

	if f := cfg.StartupLogEntries; f != nil {
		for k, v := range f() {
			logEntry[k] = v
		}
	}
	logEntry.Log()

	if f := cfg.PreStartServer; f != nil {
		if err := f(); err != nil {
			alog.Error{"action": "startup", "status": "pre_start_error", "error": err}.Log()
			return
		}
	}

	// This will start the web servers and block until they exit.
	if err := servers.Start(); err != nil {
		alog.Error{"action": "startup", "status": "error", "error": err}.Log()
	}

	// Wait for http capture to finish flushing, if it's enabled (no-op if not)
	httpcapture.DefaultRecorder.Wait()

	if f := cfg.PostStopServer; f != nil {
		if err := f(); err != nil {
			alog.Error{"action": "post-stop", "status": "post_stop_error", "error": err}.Log()
			// don't return here; server is still exiting regardless.
		}
	}

	alog.Info{"action": "shutdown", "status": "shutdown_complete"}.Log()
}

func (c *StartCommand) Run(args []string) {
	c.RunWithConfig(args, &RunCfg{})
}

// enable HTTP request/response capturing if so configured.
func (c *StartCommand) setupCaptureStore(logEntry map[string]interface{}) error {
	if !c.CaptureEnable {
		return nil
	}

	// Capture an initialized *http.Client rather than using DefaultClient,
	// as this will take configuration such as metrics and skip tls cert validation
	// into account when sending data to the blob store.
	ac, _ := apiclient.New("httpcapture", apiclient.WithoutCapture())
	httpcapture.DefaultHTTPClient = ac.Client

	store, err := httpcapture.StoreFromParams(c.CaptureAppKey, c.CaptureStoreType, c.CaptureStoreTarget)
	if err != nil {
		return err
	}
	// Set aprouter package global, which SetupMuxHandlers will read.
	CaptureStore = store
	CaptureServiceName = c.ServiceName

	// Set the default recorder, which will be used for out-of-handler request logging
	httpcapture.SetDefaultRecorder(c.ServiceName, store)

	logEntry["capture_enabled"] = true
	logEntry["capture_store_type"] = c.CaptureStoreType
	logEntry["capture_store_target"] = c.CaptureStoreTarget

	return nil
}
