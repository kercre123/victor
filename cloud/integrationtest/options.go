package main

import (
	"fmt"
	"math"
	"time"

	"github.com/anki/sai-go-util/envconfig"
)

type options struct {
	envName string

	enableAccountCreation bool

	defaultCloudDir string
	urlConfigFile   string
	testLogFile     string
	numberOfCerts   int64

	testUserName     string
	testUserPassword string

	heartBeatInterval    time.Duration
	jdocsInterval        time.Duration
	logCollectorInterval time.Duration
	tokenRefreshInterval time.Duration
}

func newFromEnvironment() *options {
	// set some sensible configuration defaults
	options := &options{
		envName: "loadtest",

		urlConfigFile: "integrationtest/server_config.json",
		testLogFile:   "/var/log/syslog",
		numberOfCerts: 1000,

		testUserPassword: "ankisecret",

		enableAccountCreation: false,

		heartBeatInterval:    5 * time.Second,
		jdocsInterval:        5 * time.Minute,
		logCollectorInterval: 30 * time.Minute,
		tokenRefreshInterval: 0,
	}

	// override settings from environment variables where needed
	envconfig.String(&options.envName, "ENVIRONMENT", "", "Test environment")

	var redisAddress = "localhost:6379"
	envconfig.String(&redisAddress, "REDIS_ADDRESS", "", "Redis host and port")
	envconfig.Int64(&options.numberOfCerts, "NUMBER_OF_CERTS", "", "The number of provisioned robot certs (0000..NNNN")

	testID, err := getUniqueTestID(redisAddress)
	if err == nil {
		testID %= options.numberOfCerts
	} else {
		testID = 1
		fmt.Printf("Could not assign unique testID (defaulting to %d), error: %v\n", testID, err)
	}

	paddedIntFormatStr := fmt.Sprintf("%%0%dd", int(math.Log10(float64(options.numberOfCerts)))+1)

	options.defaultCloudDir = fmt.Sprintf("/device_certs/"+paddedIntFormatStr, testID)

	envconfig.Int64(&testID, "TEST_ID", "", "Test ID (used for identifying user)")

	envconfig.String(&options.defaultCloudDir, "CERT_DIR", "", "Key pair directory for client certs")
	envconfig.String(&options.urlConfigFile, "URL_CONFIG_FILE", "", "Config file for Service URLs")
	envconfig.String(&options.testLogFile, "TEST_LOG_FILE", "", "File used in logcollector upload")

	envconfig.String(&options.testUserPassword, "TEST_USER_PASSWORD", "", "Password for test accounts")

	envconfig.Bool(&options.enableAccountCreation, "ENABLE_ACCOUNT_CREATION", "", "Enables account creation as part of test")

	envconfig.Duration(&options.heartBeatInterval, "HEART_BEAT_INTERVAL", "", "Periodic heart beat interval")
	envconfig.Duration(&options.jdocsInterval, "JDOCS_INTERVAL", "", "Periodic interval for JDOCS read / write")
	envconfig.Duration(&options.logCollectorInterval, "LOG_COLLECTOR_INTERVAL", "", "Periodic interval for log collector upload")
	envconfig.Duration(&options.tokenRefreshInterval, "TOKEN_REFRESH_INTERVAL", "", "Periodic interval for token refresh")

	// Create credentials for test user
	options.testUserName = fmt.Sprintf("test."+paddedIntFormatStr+"@anki.com", testID)

	return options
}

func (o *options) printOptions() {
	fmt.Println("Current test configuration:")
	fmt.Printf("  Environment = %q\n", o.envName)

	fmt.Printf("  Service URL file = %q\n", o.urlConfigFile)
	fmt.Printf("  Test log file = %q\n", o.testLogFile)
	fmt.Printf("  Number of rotating certs = %d\n", o.numberOfCerts)

	fmt.Printf("  Test user password = %q\n", o.testUserPassword)

	fmt.Printf("  Enable account creation = %v\n", o.enableAccountCreation)

	fmt.Printf("  Heart beat interval = %v\n", o.heartBeatInterval)
	fmt.Printf("  JDOCS read & write interval = %v\n", o.jdocsInterval)
	fmt.Printf("  Log collector upload interval = %v\n", o.logCollectorInterval)
	fmt.Printf("  Token refresh interval = %v\n", o.tokenRefreshInterval)
}
