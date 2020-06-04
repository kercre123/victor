package main

import (
	"fmt"
	"os"
	"time"

	"github.com/anki/sai-blobstore/api"
	"github.com/anki/sai-blobstore/blobstore"
	"github.com/anki/sai-blobstore/db"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/dynamoutil"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/http/statushandler"
	"github.com/anki/sai-go-util/log"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
)

var (
	portNum int = 8100

	accountsApiUrl string = "http://localhost:8000/"

	// A key specific to this server for use with the accounts server
	apiKey string = "blobstore-server"

	bucketName       string
	bucketPathPrefix string

	// Check the database status every 5 seconds
	statusCheckFrequency = 5 * time.Second

	// Require 3 failures/successes to change the status returned
	// via the status endpoint
	statusTriggerCount = 3

	appKeyWhitelist string // app keys which do not have rate limiting applied

	profileMode string // cpu, block or mem
	profilePath string = "/tmp"

	profileServerPort int // if non zero then start a profiling server

	maxMemoryMB int // stop serving new HTTP requests if memory limit reached
)

// Define the command that starts the blobstore api server.
var runServerCmd = &Command{
	Name:       "start",
	RequiresDb: true,
	Exec:       runServer,
	Flags:      runServerFlags,
}

func runServerFlags(cmd *Command) {
	envconfig.Int(&portNum, "HTTP_PORT", "port", "The port number for the service to listen on")
	envconfig.String(&accountsApiUrl, "ACCOUNTS_API_URL", "accounts-api", "The base URL for the Accounts API server")
	envconfig.String(&apiKey, "API_KEY", "apikey", "The API key used to identify this server with the accounts api")
	envconfig.String(&appKeyWhitelist, "APP_KEY_WHITELIST", "", "App keys that are excluded from whitelisting, comma separated")
	envconfig.String(&bucketName, "BUCKET", "bucket", "S3 Bucket to read/write blobs to")
	envconfig.String(&bucketPathPrefix, "BUCKET_PREFIX", "bucket-prefix", "Path prefix to use for S3 objects")
	envconfig.String(&profileMode, "PROFILE_MODE", "profile-mode", "Capture profiling information.  Can be set to cpu, mem or block")
	envconfig.String(&profilePath, "PROFILE_PATH", "profile-path", "Directory to write profiling output to")
	envconfig.Int(&profileServerPort, "PROFILE_PORT", "profile-port", "If set then an HTTP profile server will be started on the supplied port number")
	envconfig.Int(&maxMemoryMB, "MAX_MEMORY_MB", "", "If set then requests return a 503 error if the server is using too much memory")
}

func runServer(cmd *Command, args []string) {
	if len(accountsApiUrl) == 0 {
		fmt.Println("Invalid setting for ACCOUNTS_API_URL")
	}
	if accountsApiUrl[len(accountsApiUrl)-1] != '/' {
		accountsApiUrl += "/"
	}

	api.RegisterAPI()

	// setup the http access log target
	apirouter.LogWriter = cli.NewAccessLogWriter("")

	apirouter.SetAppTokenWhitelist(appKeyWhitelist)
	baseHandler := apirouter.SetupMuxHandlers(uint64(maxMemoryMB), nil)
	setupDb(true)

	// Connect the session validation handlers to the Accounts API server.
	ac := &validate.AccountsServer{
		Server: accountsApiUrl,
		AppKey: apiKey,
	}
	ac.Init()

	validate.DefaultSessionValidator = ac

	statusMonitor := statushandler.NewMonitor(
		statusCheckFrequency, statusTriggerCount, statushandler.StatusCheckerFunc(dynamoutil.CheckStatus))
	apirouter.AddStatusHandler(statusMonitor)

	api.Store = &blobstore.Store{
		S3:     s3.New(session.New()),
		Bucket: bucketName,
		Prefix: bucketPathPrefix,
	}

	// log some runtime stats once a minute
	cli.StartRuntimeLogger()

	// setup profiling
	profiler := cli.StartProfiler(profilePath, profileMode)

	// Create the HTTP server(s)
	servers := apirouter.NewServers(baseHandler, portNum, profileServerPort)

	// Add a signal handler
	cli.StartSigWatcher(func(sig os.Signal) {
		alog.Info{"action": "shutdown", "status": "shutting_down"}.Log()
		servers.Close()
	})

	logEntry := alog.Info{
		"action":       "startup",
		"status":       "starting",
		"s3_bucket":    bucketName,
		"s3_prefix":    bucketPathPrefix,
		"dynamo_table": db.TableName(),
		"port":         portNum,
		"maxMemoryMB":  maxMemoryMB,
		"accounts_api": accountsApiUrl,
	}
	for k, v := range buildinfo.Map() {
		logEntry[k] = v
	}
	logEntry.Log()

	// This will start the web servers and block until they exit.
	if err := servers.Start(); err != nil {
		alog.Error{"action": "startup", "status": "error", "error": err}.Log()
	}
	if profiler != nil {
		profiler.Stop()
	}
	alog.Info{"action": "shutdown", "status": "shutdown_complete"}.Log()
}
