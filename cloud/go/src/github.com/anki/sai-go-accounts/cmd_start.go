// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package main

import (
	"fmt"
	"log/syslog"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-accounts/apis/session"
	"github.com/anki/sai-go-accounts/apis/user"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/email"
	sessionmodel "github.com/anki/sai-go-accounts/models/session"
	ankisqs "github.com/anki/sai-go-accounts/sqs"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/http/statushandler"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/endpoints"
	awssession "github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sqs"
)

var (
	portNum      int = 8000
	enableSyslog bool
	syslogAddr   string

	auditApiUrl string
	auditApiKey string

	awsRegion        = endpoints.UsWest2RegionID
	awsAccessKey     string
	awsSecretKey     string
	inQueueURL       string
	sqsEndpoint      string
	awsSqsDisableSSL string

	// Check the database status every 5 seconds
	statusCheckFrequency = 5 * time.Second

	// Require 3 failures/successes to change the status returned
	// via the status endpoint
	statusTriggerCount = 3

	httpSyslogTag      = "sai-accounts-access-combined"
	httpSyslogPriority = syslog.LOG_INFO | syslog.LOG_LOCAL6
	alogSyslogTag      = "sai-accounts-status"
	alogSyslogPriority = syslog.LOG_INFO | syslog.LOG_LOCAL7

	ApiLocator string

	FormsUrlRoot string
	BuildBranch  string // to be filled by build steps

	appKeyWhitelist string // app keys which do not have rate limiting applied

	profileMode string // cpu, block or mem
	profilePath string = "/tmp"

	profileServerPort int // if non zero then start a profiling server

	maxMemoryMB int // stop serving new HTTP requests if memory limit reached

	samlAdminGroups   string
	samlSupportGroups string
	samlIDPName       string

	postmarkWebhookPassword string
	adminPasswordHash       string
)

// Define the command that starts the accounts api server.
var runServerCmd = &Command{
	Name:       "start",
	RequiresDb: true,
	Exec:       runServer,
	Flags:      runServerFlags,
}

func runServerFlags(cmd *Command) {
	envconfig.String(&awsSqsDisableSSL, "AWS_SQS_DISABLESSL", "disable-ssl", "Disable SSL when connecting to SQS")
	envconfig.String(&sqsEndpoint, "AWS_SQS_ENDPOINT", "sqs-endpoint", "URL to override location of SQS")
	envconfig.String(&awsRegion, "AWS_REGION", "aws-region", "AWS region")
	envconfig.String(&awsAccessKey, "AWS_ACCESS_KEY", "aws-access-key", "AWS access key")
	envconfig.String(&awsSecretKey, "AWS_SECRET_KEY", "aws-secret-key", "AWS secret key")
	envconfig.String(&inQueueURL, "ORDERS_QUEUE_URL", "queue-url", "SQS inbound raw data queue URL")
	envconfig.Int(&portNum, "HTTP_PORT", "port", "The port number for the service to listen on")
	envconfig.Bool(&enableSyslog, "ENABLE_SYSLOG", "syslog", "Set to true to send logs to syslog instead of stdout")
	envconfig.String(&syslogAddr, "SYSLOG_ADDR", "syslog-addr", "Specify the syslog address to dial; defaults to the system logger. eg. tcp:1.2.3.4:1000 or unix:/tmp/log")
	envconfig.String(&ApiLocator, "API_LOCATOR", "", "Optional dloc value to send the to the web for API URL discovery")
	envconfig.String(&FormsUrlRoot, "URL_ROOT", "", "URL Root to send users for URL links: https://accounts.api.anki.com")
	envconfig.String(&appKeyWhitelist, "APP_KEY_WHITELIST", "", "App keys that are excluded from whitelisting, comma separated")
	envconfig.String(&profileMode, "PROFILE_MODE", "profile-mode", "Capture profiling information.  Can be set to cpu, mem or block")
	envconfig.String(&profilePath, "PROFILE_PATH", "profile-path", "Directory to write profiling output to")
	envconfig.Int(&profileServerPort, "PROFILE_PORT", "profile-port", "If set then an HTTP profile server will be started on the supplied port number")
	envconfig.Int(&maxMemoryMB, "MAX_MEMORY_MB", "", "If set then requests return a 503 error if the server is using too much memory")
	envconfig.String(&samlAdminGroups, "SAML_ADMIN_GROUPS", "saml-admin-groups", "SAML group names that should be assigned admin privileges")
	envconfig.String(&samlSupportGroups, "SAML_SUPPORT_GROUPS", "saml-support-groups", "SAML group names that should be assigned support privileges")
	envconfig.String(&samlIDPName, "SAML_IDP", "saml-id", "Name of SAML identity provider to use.  One of "+strings.Join(session.IDPNames(), ","))
	envconfig.String(&postmarkWebhookPassword, "POSTMARK_WEBHOOK_PW", "postmark-webhook-pw", "Password that Postmark will supply when calling webhook endpoints")
	envconfig.String(&auditApiUrl, "AUDIT_API_URL", "audit-api", "The base URL for the account audit API server")
	envconfig.String(&auditApiKey, "AUDIT_API_KEY", "audit-apikey", "The API key used to identify this server with the account audit API")
	envconfig.String(&adminPasswordHash, "ADMIN_PASSWORD", "admin-password", "A hashed password to provide admin access to the API")
}

func runServer(cmd *Command, args []string) {
	cli.SetupLogging()
	// Set up module defaults
	email.FormsUrlRoot = FormsUrlRoot
	email.ApiLocator = ApiLocator
	user.FormsUrlRoot = FormsUrlRoot
	if adminPasswordHash != "" {
		session.SetAdminPasswordHash(adminPasswordHash)
	}

	// Sanity checks on email templates, etc
	email.InitAnkiEmailer()
	email.VerifyEmailer()

	// Ensure we query the local DB for session validation
	validate.DefaultSessionValidator = &sessionmodel.LocalValidator{}

	// setup the http access log target
	apirouter.LogWriter = cli.NewAccessLogWriter("")

	statusMonitor := statushandler.NewMonitor(
		statusCheckFrequency, statusTriggerCount, statushandler.StatusCheckerFunc(db.CheckStatus))

	for _, gn := range strings.Split(samlAdminGroups, ",") {
		session.AddSAMLGroup(gn, sessionmodel.AdminScope)
	}
	for _, gn := range strings.Split(samlSupportGroups, ",") {
		session.AddSAMLGroup(gn, sessionmodel.SupportScope)
	}

	if samlIDPName != "" {
		if err := session.SetActiveSAMLIDP(samlIDPName); err != nil {
			alog.Error{
				"action": "startup",
				"status": "error",
				"error":  fmt.Sprintf("Failed to set SAML IDP: %s", err),
			}.Log()
			cli.Exit(1)
		}
	}

	if postmarkWebhookPassword != "" {
		user.PostmarkHookAuthUsers["postmark"] = postmarkWebhookPassword
	}

	if auditApiUrl != "" {
		if err := audit.Configure(
			auditApiUrl,
			auditApiKey); err != nil {
			fmt.Println("Failed to initialize Audit API client: ", err)
			os.Exit(1)
		}
	}

	user.RegisterAPI()
	session.RegisterAPI()
	wl := apirouter.SetAppTokenWhitelist(appKeyWhitelist)
	session.RateLimiter.SetWhitelist(wl)
	baseHandler := apirouter.SetupMuxHandlers(uint64(maxMemoryMB), nil)
	apirouter.AddStatusHandler(statusMonitor)

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
		"action":      "startup",
		"status":      "starting",
		"port":        portNum,
		"profileport": profileServerPort,
		"maxMemoryMB": maxMemoryMB,
	}
	for k, v := range buildinfo.Map() {
		logEntry[k] = v
	}
	logEntry.Log()

	// setup aws session
	disableSSL, _ := strconv.ParseBool(awsSqsDisableSSL)
	awscfg := aws.NewConfig().WithDisableSSL(disableSSL)

	if sqsEndpoint != "" {
		awscfg = awscfg.WithEndpoint(sqsEndpoint)
	}

	if len(awsAccessKey) > 0 && len(awsSecretKey) > 0 {
		awscfg = awscfg.WithCredentials(credentials.NewStaticCredentials(awsAccessKey, awsSecretKey, ""))
	}

	if len(awsRegion) > 0 {
		awscfg = awscfg.WithRegion(awsRegion)
	}

	sess := awssession.Must(awssession.NewSession(awscfg))

	sqsSvc := sqs.New(sess)
	metricsutil.AddAWSMetricsHandlers(sqsSvc.Client)

	// start SQS reader
	var qr *ankisqs.QueueReader

	if inQueueURL != "" {
		qr = &ankisqs.QueueReader{
			Reader: &ankisqs.AwsSQSReader{
				SQS:                 sqsSvc,
				QueueUrl:            inQueueURL,
				WaitTimeSeconds:     5,
				MaxNumberOfMessages: 10,
			},
		}
		qr.Start(1)
	}

	// This will start the web servers and block until they exit.
	if err := servers.Start(); err != nil {
		alog.Error{"action": "startup", "status": "error", "error": err}.Log()
	}

	if qr != nil {
		alog.Info{"action": "shutdown_processor", "status": "waiting", "processor": "sqs_reader"}.Log()
		qr.Stop() // Returns only when it's finished reading from the SQS queue and fed downstream
		alog.Info{"action": "shutdown_processor", "status": "complete", "processor": "sqs_reader"}.Log()
	}

	if profiler != nil {
		profiler.Stop()
	}

	alog.Info{"action": "shutdown", "status": "shutdown_complete"}.Log()
}
