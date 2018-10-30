package main

import (
	"fmt"
	"time"

	"github.com/jawher/mow.cli"
)

type uniqueIDProvider interface {
	provideUniqueTestID() (int, error)
}

type options struct {
	envName *string

	testID *int

	enableDistributedControl *bool
	enableAccountCreation    *bool

	redisAddress    *string
	defaultCloudDir *string
	urlConfigFile   *string
	testLogFile     *string
	numberOfCerts   *int

	testUserName     *string
	testUserPassword *string

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

	options.testID = app.Int(cli.IntOpt{
		Name:   "i test-id",
		Desc:   "Test ID (used for identifying user)",
		EnvVar: "TEST_ID",
		Value:  1,
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

	options.defaultCloudDir = app.String(cli.StringOpt{
		Name:   "k key-dir",
		Desc:   "Key pair directory for client certs",
		EnvVar: "CERT_DIR",
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

	options.testUserName = app.String(cli.StringOpt{
		Name:   "u username",
		Desc:   "Username for test accounts",
		EnvVar: "TEST_USER_NAME",
	})

	options.testUserPassword = app.String(cli.StringOpt{
		Name:   "p password",
		Desc:   "Password for test accounts",
		EnvVar: "TEST_USER_PASSWORD",
		Value:  "ankisecret",
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

	// Note this only works for environment variables
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

func (o *options) finalizeIdentity(idProvider uniqueIDProvider) {
	testID, err := idProvider.provideUniqueTestID()
	if err == nil {
		testID %= *o.numberOfCerts
		*o.testID = testID
	} else {
		testID = *o.testID

		fmt.Printf("Could not assign unique testID (defaulting to %d), error: %v\n", testID, err)
	}

	if *o.defaultCloudDir == "" {
		o.defaultCloudDir = new(string)
		*o.defaultCloudDir = fmt.Sprintf("/device_certs/%08d", testID)
	}

	if *o.testUserName == "" {
		o.testUserName = new(string)
		*o.testUserName = fmt.Sprintf("test.%04d@anki.com", testID)
	}
}
