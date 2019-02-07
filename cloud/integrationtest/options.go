package main

import (
	"fmt"
	"time"

	"github.com/jawher/mow.cli"
)

type options struct {
	envName *string

	robotsPerProcess *int
	tasksPerCluster  *int
	reportingTasks   *int

	enableDistributedControl *bool
	enableAccountCreation    *bool

	redisAddress     *string
	wavefrontAddress *string
	defaultCloudDir  *string
	urlConfigFile    *string
	testLogFile      *string
	numberOfCerts    *int

	defaultTestUserName *string
	testUserPassword    *string

	rampupDuration   time.Duration
	rampdownDuration time.Duration

	reportingInterval time.Duration

	robotsPerCluster int

	heartBeatInterval       time.Duration
	heartBeatStdDev         time.Duration
	tokenRefreshInterval    time.Duration
	tokenRefreshStdDev      time.Duration
	jdocsInterval           time.Duration
	jdocsStdDev             time.Duration
	logCollectorInterval    time.Duration
	logCollectorStdDev      time.Duration
	connectionCheckInterval time.Duration
	connectionCheckStdDev   time.Duration
}

type instanceOptions struct {
	taskID       int
	robotID      int
	testUserName string
	cloudDir     string

	rampupDelay   time.Duration
	rampdownDelay time.Duration
}

func parseIntervalString(intervalStr *string) time.Duration {
	if intervalStr == nil {
		return 0
	}

	duration, err := time.ParseDuration(*intervalStr)
	if err != nil {
		return 0
	}

	return duration
}

func newFromEnvironment(app *cli.Cli) *options {
	options := new(options)

	options.envName = app.String(cli.StringOpt{
		Name:   "e env",
		Desc:   "Test environment",
		EnvVar: "ENVIRONMENT",
		Value:  "loadtest",
	})

	options.robotsPerProcess = app.Int(cli.IntOpt{
		Name:   "robots-per-process",
		Desc:   "Number of robot instances per process",
		EnvVar: "ROBOTS_PER_PROCESS",
		Value:  1,
	})

	options.tasksPerCluster = app.Int(cli.IntOpt{
		Name:   "tasks-per-cluster",
		Desc:   "Number of tasks per ECS/Fargate cluster",
		EnvVar: "TASKS_PER_CLUSTER",
		Value:  1,
	})

	options.reportingTasks = app.Int(cli.IntOpt{
		Name:   "reporting-tasks-per-cluster",
		Desc:   "Number of tasks per ECS/Fargate cluster reporting metrics",
		EnvVar: "REPORTING_TASKS_PER_CLUSTER",
		Value:  1000,
	})

	reportingInterval := app.String(cli.StringOpt{
		Name:   "metrics-reporting-interval",
		Desc:   "Wavefront metrics reporing interval (time.Duration string)",
		EnvVar: "METRICS_REPORTING_INTERVAL",
		Value:  "30s",
	})

	options.enableAccountCreation = app.Bool(cli.BoolOpt{
		Name:   "a account-creation",
		Desc:   "Enables account creation as part of test",
		EnvVar: "ENABLE_ACCOUNT_CREATION",
		Value:  false,
	})

	options.enableDistributedControl = app.Bool(cli.BoolOpt{
		Name:   "a account-creation",
		Desc:   "Enables remote control for starting/stopping",
		EnvVar: "ENABLE_DISTRIBUTED_CONTROL",
		Value:  false,
	})

	options.redisAddress = app.String(cli.StringOpt{
		Name:   "r redis-endpoint",
		Desc:   "Redis host and port",
		EnvVar: "REDIS_ADDRESS",
		Value:  "localhost:6379",
	})

	options.wavefrontAddress = app.String(cli.StringOpt{
		Name:   "r wavefront-endpoint",
		Desc:   "Wavefront host and port",
		EnvVar: "WAVEFRONT_ADDRESS",
		Value:  "",
	})

	options.defaultCloudDir = app.String(cli.StringOpt{
		Name:   "k key-dir",
		Desc:   "Key pair directory for client certs",
		EnvVar: "DEFAULT_CLOUD_DIR",
	})

	options.urlConfigFile = app.String(cli.StringOpt{
		Name:   "c url-config",
		Desc:   "Config file for Service URLs",
		EnvVar: "URL_CONFIG_FILE",
		Value:  "integrationtest/server_config.json",
	})

	options.testLogFile = app.String(cli.StringOpt{
		Name:   "f log-file",
		Desc:   "File used in logcollector upload",
		EnvVar: "TEST_LOG_FILE",
		Value:  "/var/log/syslog",
	})

	options.numberOfCerts = app.Int(cli.IntOpt{
		Name:   "n num-certs",
		Desc:   "The number of provisioned robot certs (0000..NNNN)",
		EnvVar: "NUMBER_OF_CERTS",
		Value:  1000,
	})

	options.defaultTestUserName = app.String(cli.StringOpt{
		Name:   "u username",
		Desc:   "Default username for test accounts",
		EnvVar: "DEFAULT_TEST_USER_NAME",
	})

	options.testUserPassword = app.String(cli.StringOpt{
		Name:   "p password",
		Desc:   "Password for test accounts",
		EnvVar: "TEST_USER_PASSWORD",
		Value:  "ankisecret",
	})

	rampupDuration := app.String(cli.StringOpt{
		Name:   "ramp-down-duration",
		Desc:   "Robot fleet ramp-up duration (time.Duration string)",
		EnvVar: "RAMP_UP_DURATION",
		Value:  "0s",
	})

	rampdownDuration := app.String(cli.StringOpt{
		Name:   "ramp-up-duration",
		Desc:   "Robot fleet ramp-down duration (time.Duration string)",
		EnvVar: "RAMP_DOWN_DURATION",
		Value:  "0s",
	})

	heartBeatInterval := app.String(cli.StringOpt{
		Name:   "heart-beat-interval",
		Desc:   "Periodic heart beat interval (time.Duration string)",
		EnvVar: "HEART_BEAT_INTERVAL",
		Value:  "30s",
	})

	jdocsInterval := app.String(cli.StringOpt{
		Name:   "jdocs-interval",
		Desc:   "Periodic interval for JDOCS read / write (time.Duration string)",
		EnvVar: "JDOCS_INTERVAL",
		Value:  "5m",
	})

	logCollectorInterval := app.String(cli.StringOpt{
		Name:   "log-collector-interval",
		Desc:   "Periodic interval for log collector upload (time.Duration string)",
		EnvVar: "LOG_COLLECTOR_INTERVAL",
		Value:  "30m",
	})

	tokenRefreshInterval := app.String(cli.StringOpt{
		Name:   "token-refresh-interval",
		Desc:   "Periodic interval for token refresh (time.Duration string)",
		EnvVar: "TOKEN_REFRESH_INTERVAL",
		Value:  "0",
	})

	connectionCheckInterval := app.String(cli.StringOpt{
		Name:   "connection-check-interval",
		Desc:   "Periodic interval for voice connection check (time.Duration string)",
		EnvVar: "CONNECTION_CHECK_INTERVAL",
		Value:  "5m",
	})

	heartBeatStdDev := app.String(cli.StringOpt{
		Name:   "heart-beat-stddev",
		Desc:   "Periodic standard deviation for heart beat (time.Duration string)",
		EnvVar: "HEART_BEAT_STDDEV",
		Value:  "0",
	})

	jdocsStdDev := app.String(cli.StringOpt{
		Name:   "jdocs-stddev",
		Desc:   "Periodic standard deviation for JDOCS read / write (time.Duration string)",
		EnvVar: "JDOCS_STDDEV",
		Value:  "0",
	})

	logCollectorStdDev := app.String(cli.StringOpt{
		Name:   "log-collector-stddev",
		Desc:   "Periodic standard deviation for log collector upload (time.Duration string)",
		EnvVar: "LOG_COLLECTOR_STDDEV",
		Value:  "0",
	})

	tokenRefreshStdDev := app.String(cli.StringOpt{
		Name:   "token-refresh-stddev",
		Desc:   "Periodic standard deviation for token refresh (time.Duration string)",
		EnvVar: "TOKEN_REFRESH_STDDEV",
		Value:  "0",
	})

	connectionCheckStdDev := app.String(cli.StringOpt{
		Name:   "connection-check-stddev",
		Desc:   "Periodic standrd deviation for voice connection check (time.Duration string)",
		EnvVar: "CONNECTION_CHECK_STDDEV",
		Value:  "5m",
	})

	options.robotsPerCluster = *options.tasksPerCluster * *options.robotsPerProcess

	// Note this only works for environment variables
	options.rampupDuration = parseIntervalString(rampupDuration)
	options.rampdownDuration = parseIntervalString(rampdownDuration)

	options.reportingInterval = parseIntervalString(reportingInterval)

	options.heartBeatInterval = parseIntervalString(heartBeatInterval)
	options.jdocsInterval = parseIntervalString(jdocsInterval)
	options.logCollectorInterval = parseIntervalString(logCollectorInterval)
	options.tokenRefreshInterval = parseIntervalString(tokenRefreshInterval)
	options.connectionCheckInterval = parseIntervalString(connectionCheckInterval)

	options.heartBeatStdDev = parseIntervalString(heartBeatStdDev)
	options.jdocsStdDev = parseIntervalString(jdocsStdDev)
	options.logCollectorStdDev = parseIntervalString(logCollectorStdDev)
	options.tokenRefreshStdDev = parseIntervalString(tokenRefreshStdDev)
	options.connectionCheckStdDev = parseIntervalString(connectionCheckStdDev)

	return options
}

func (o *options) calculateDelay(rampDuration time.Duration, robotID int) time.Duration {
	// Note: We ramp robots linearly in time (based on their index and task offset)
	return time.Duration((int(rampDuration) * robotID) / o.robotsPerCluster)
}

func (o *options) createIdentity(provider robotIdentityProvider, robotIndex, taskID int) *instanceOptions {
	// Note: we ensure that robots in the same container are spread across the entire cluster's ID range
	// (and hence also across the entire ramp duration in order to distribute ramp load across containers)
	// We also wrap around robot IDs (e.g. to ensure its cert directory exists)
	robotID := ((robotIndex * *o.tasksPerCluster) + taskID) % *o.numberOfCerts

	var rampupDelay, rampdownDelay time.Duration
	if provider != nil {
		var err error

		rampupDelay, err = provider.arrivalTime(robotID)
		if err != nil {
			rampupDelay = o.calculateDelay(o.rampupDuration, robotID)
		}

		rampdownDelay, err = provider.departureTime(robotID)
		if err != nil {
			rampdownDelay = o.calculateDelay(o.rampdownDuration, robotID)
		}
	} else {
		rampupDelay = o.calculateDelay(o.rampupDuration, robotID)
		rampdownDelay = o.calculateDelay(o.rampdownDuration, robotID)
	}

	options := &instanceOptions{
		taskID:       taskID,
		robotID:      robotID,
		testUserName: *o.defaultTestUserName,
		cloudDir:     *o.defaultCloudDir,

		rampupDelay:   rampupDelay,
		rampdownDelay: rampdownDelay,
	}

	if options.cloudDir == "" {
		options.cloudDir = fmt.Sprintf("/device_certs/%08d", robotID)
	}

	if options.testUserName == "" {
		options.testUserName = fmt.Sprintf("test.%08d@example.com", robotID)
	}

	return options
}
