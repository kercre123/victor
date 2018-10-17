package main

import (
	"fmt"
	"math"
	"time"

	"github.com/jawher/mow.cli"
)

type options struct {
	envName *string

	defaultTestID *int

	enableAccountCreation *bool

	redisAddress    *string
	defaultCloudDir *string
	urlConfigFile   *string
	testLogFile     *string
	numberOfCerts   *int

	testUserName     *string
	testUserPassword *string

	heartBeatInterval    *int
	jdocsInterval        *int
	logCollectorInterval *int
	tokenRefreshInterval *int
}

func newFromEnvironment(app *cli.Cli) *options {
	options := new(options)

	options.envName = app.String(cli.StringOpt{
		Name:   "e env",
		Desc:   "Test environment",
		EnvVar: "ENVIRONMENT",
		Value:  "loadtest",
	})

	options.defaultTestID = app.Int(cli.IntOpt{
		Name:   "i test-id",
		Desc:   "Test ID (used for identifying user)",
		EnvVar: "TEST_ID",
		Value:  1,
	})

	options.redisAddress = app.String(cli.StringOpt{
		Name:   "r redis-endpoint",
		Desc:   "Redis host and port",
		EnvVar: "REDIS_ADDRESS",
		Value:  "localhost:6379",
	})

	options.numberOfCerts = app.Int(cli.IntOpt{
		Name:   "n num-certs",
		Desc:   "The number of provisioned robot certs (0000..NNNN)",
		EnvVar: "NUMBER_OF_CERTS",
		Value:  1000,
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

	options.enableAccountCreation = app.Bool(cli.BoolOpt{
		Name:   "a account-creation",
		Desc:   "Enables account creation as part of test",
		EnvVar: "ENABLE_ACCOUNT_CREATION",
		Value:  false,
	})

	options.heartBeatInterval = app.Int(cli.IntOpt{
		Name:   "h heart-beat-interval",
		Desc:   "Periodic heart beat interval",
		EnvVar: "HEART_BEAT_INTERVAL",
		Value:  int(30 * time.Second),
	})

	options.jdocsInterval = app.Int(cli.IntOpt{
		Name:   "j jdocs-interval",
		Desc:   "Periodic interval for JDOCS read / write",
		EnvVar: "JDOCS_INTERVAL",
		Value:  int(5 * time.Minute),
	})

	options.logCollectorInterval = app.Int(cli.IntOpt{
		Name:   "l log-collector-interval",
		Desc:   "Periodic interval for log collector upload",
		EnvVar: "LOG_COLLECTOR_INTERVAL",
		Value:  int(30 * time.Minute),
	})

	options.tokenRefreshInterval = app.Int(cli.IntOpt{
		Name:   "t token-refresh-interval",
		Desc:   "Periodic interval for token refresh",
		EnvVar: "TOKEN_REFRESH_INTERVAL",
		Value:  0,
	})

	return options
}

func (o *options) finalizeIdentity() {
	testID, err := getUniqueTestID(*o.redisAddress)
	if err == nil {
		testID %= *o.numberOfCerts
	} else {
		testID = *o.defaultTestID
		fmt.Printf("Could not assign unique testID (defaulting to %d), error: %v\n", testID, err)
	}

	if *o.defaultCloudDir == "" {
		paddedIntFormatStr := fmt.Sprintf("%%0%dd", int(math.Log10(float64(*o.numberOfCerts)))+1)
		o.defaultCloudDir = new(string)
		*o.defaultCloudDir = fmt.Sprintf("/device_certs/"+paddedIntFormatStr, testID)
	}

	if *o.testUserName == "" {
		o.testUserName = new(string)
		*o.testUserName = fmt.Sprintf("test.%04d@anki.com", testID)
	}
}
