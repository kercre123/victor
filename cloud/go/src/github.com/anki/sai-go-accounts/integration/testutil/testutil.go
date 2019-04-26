// Utilities useful for integration tests that need to spin up the Accounts API and
// database in a docker container envioronment.

package testutil

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"strings"
	"syscall"
	"time"

	"github.com/anki/sai-go-util/dockerutil"
)

var (
	ServerWaitTimeout time.Duration = 20 * time.Second
)

// WaitForServer polls the given endpoint using GET until it
// gets a 200 response or reaches the ServerWaitTimeout limit
func WaitForServer(url string) error {
	var (
		resp *http.Response
		body []byte
		err  error
	)
	end := time.Now().Add(ServerWaitTimeout)
	for time.Now().Before(end) {
		resp, err = http.Get(url)
		if err == nil && resp.StatusCode == http.StatusOK {
			if resp != nil {
				resp.Body.Close()
			}
			return nil
		}
		if resp != nil {
			body, _ = ioutil.ReadAll(resp.Body)
			resp.Body.Close()
		}
		time.Sleep(100 * time.Millisecond)
	}
	if err != nil {
		return err
	}
	if resp != nil {
		return fmt.Errorf("Got error from status endpoint at %s: %s", url, string(body))
	}
	return errors.New("Unknown error while waiting for server at " + url)
}

type Scope string

const (
	Admin   Scope = "admin"
	Cron    Scope = "cron"
	Service Scope = "service"
	User    Scope = "user"
	Support Scope = "support"
)

type AccountsDockerEnv struct {
	DumpLog           bool
	PostmarkApiKey    string
	AccountsApiPort   string
	ProfilePort       string
	UserAppKey        string
	FbAppId           string
	FbAppSecret       string
	AccountsBin       string
	mysqlContainer    *dockerutil.Container
	AccountsApiUrl    string
	ProfileUrl        string
	accountsCmd       *exec.Cmd
	accountsWaiter    chan error
	AppKeys           map[Scope]string
	cmdOutput         *bytes.Buffer
	AdminSessionToken string
	CronSessionToken  string
	ProfileMode       string
	SQSTestSuite      dockerutil.SQSSuite
	SQSQueueUrl       string
}

// MysqlContainer returns the instance of the running mysql container, or nil.
func (a *AccountsDockerEnv) MysqlContainer() *dockerutil.Container {
	return a.mysqlContainer
}

func (a *AccountsDockerEnv) SQSSuite() dockerutil.SQSSuite {
	return a.SQSTestSuite
}

// Url returns the base URL of the accounts api server
func (a *AccountsDockerEnv) Url() string {
	return a.AccountsApiUrl
}

// Launch a MySQL docker process, create the database  and start the accounts api process
// configured to talk to it.
func (a *AccountsDockerEnv) Start() error {
	var err error

	// start SQS container
	fmt.Println("Starting SQS docker container")
	a.SQSTestSuite.SetupSuite()

	// the url returned byt CreateQueue is wrong... points to localhost instead of the docker forward
	a.SQSTestSuite.CreateQueue("integration-test")
	a.SQSQueueUrl = a.SQSTestSuite.Endpoint + "/queue/integration-test"

	// start mysql
	a.AccountsApiUrl = "http://localhost:" + a.AccountsApiPort
	a.ProfileUrl = "http://localhost:" + a.ProfilePort

	fmt.Println("Starting mysql docker container")
	a.mysqlContainer, err = dockerutil.StartMysqlContainer("root", "testdb")
	if err != nil {
		fmt.Printf("Failed to start MySQL docker container: %s\n", err)
		os.Exit(1)
	}
	accountsEnv := []string{
		"HTTP_PORT=" + a.AccountsApiPort,
		"PROFILE_PORT=" + a.ProfilePort,
		"DB_TYPE=" + "mysql",
		"MYSQL_DB=" + "testdb",
		"MYSQL_IPADDR=" + a.mysqlContainer.IpAddress(),
		"MYSQL_USER=" + "root",
		"MYSQL_PASSWORD=" + "root",
		"MYSQL_PORT=" + a.mysqlContainer.Port(),
		"POSTMARK_API_KEY=" + a.PostmarkApiKey,
		"FB_APP_ID=" + a.FbAppId,
		"FB_APP_SECRET=" + a.FbAppSecret,
		"URL_ROOT=" + a.AccountsApiUrl,
		"ORDERS_QUEUE_URL=" + a.SQSQueueUrl,
		"AWS_SQS_ENDPOINT=" + a.SQSTestSuite.Endpoint,
		"AWS_REGION=us-east-1",
		"AWS_ACCESS_KEY=x",
		"AWS_SECRET_KEY=x",
		"AWS_SQS_DISABLESSL=true",
	}

	// Create the database
	cmd := exec.Command(a.AccountsBin, "create-tables")
	cmd.Env = accountsEnv
	if output, err := cmd.CombinedOutput(); err != nil {
		a.Stop()
		return fmt.Errorf("Failed to create database err=%v:\n%s\n", err, string(output))
	}
	fmt.Println(cmd.ProcessState.Pid(), "Created DB tables")

	// Add the app keys
	keys := []string{}
	for scope, key := range a.AppKeys {
		keys = append(keys, key)
		cmd = exec.Command(a.AccountsBin, "create-app-key", "-token", key, "-scopemask", string(scope))
		cmd.Env = accountsEnv
		if output, err := cmd.CombinedOutput(); err != nil {
			a.Stop()
			return fmt.Errorf("Failed to create app key key=%s, scope = %s, err=%v:\n%s\n", key, string(scope), err, string(output))
		}
		fmt.Println(cmd.ProcessState.Pid(), "Created App key", key)
	}
	// create one rate-limited token
	cmd = exec.Command(a.AccountsBin, "create-app-key", "-token", "rate-limited-key", "-scopemask", "user")
	cmd.Env = accountsEnv
	cmd.CombinedOutput()
	fmt.Println(cmd.ProcessState.Pid(), "Created App key rate-limited-key")
	accountsEnv = append(accountsEnv, "APP_KEY_WHITELIST="+strings.Join(keys, ";"))

	// Create an admin session token
	// Create the database
	cmd = exec.Command(a.AccountsBin, "create-session", "-scope", "admin", "-ownerid", "integration@example.com")
	cmd.Env = accountsEnv
	var output []byte
	if output, err = cmd.CombinedOutput(); err != nil {
		a.Stop()
		return fmt.Errorf("Failed to create admin session err=%v:\n%s\n", err, string(output))
	}
	fmt.Println(cmd.ProcessState.Pid(), "Created admin session")
	a.AdminSessionToken = strings.TrimSpace(string(output))

	// Create a cron session token
	cmd = exec.Command(a.AccountsBin, "create-session", "-scope", "cron", "-ownerid", "integration@example.com")
	cmd.Env = accountsEnv
	if output, err = cmd.CombinedOutput(); err != nil {
		a.Stop()
		return fmt.Errorf("Failed to create cron session err=%v:\n%s\n", err, string(output))
	}
	fmt.Println(cmd.ProcessState.Pid(), "Created cron session")
	a.CronSessionToken = strings.TrimSpace(string(output))

	fmt.Println("PROFILE", a.ProfileMode)
	if a.ProfileMode != "" {
		accountsEnv = append(accountsEnv, "PROFILE_MODE="+a.ProfileMode)
	}

	// start the accounts api server
	fmt.Println("Starting accounts server")
	a.accountsCmd = exec.Command(a.AccountsBin, "start")
	a.accountsCmd.Env = accountsEnv
	a.cmdOutput = new(bytes.Buffer)
	a.accountsCmd.Stdout = a.cmdOutput
	a.accountsCmd.Stderr = a.cmdOutput
	if err := a.accountsCmd.Start(); err != nil {
		a.Stop()
		return fmt.Errorf("Failed to start %s: %s\n", a.AccountsBin, err)
	}
	a.accountsWaiter = make(chan error)
	go func() { a.accountsWaiter <- a.accountsCmd.Wait() }()

	// make sure it's still running 250ms later (ie. it hasn't bombed due to a bad setting)
	select {
	case <-time.After(250 * time.Millisecond):
	case err := <-a.accountsWaiter:
		a.accountsCmd = nil
		a.Stop()
		if a.DumpLog {
			fmt.Println(a.cmdOutput.String())
		}
		return fmt.Errorf("Failed to start (fast exit) %s: %v\n", a.AccountsBin, err)
	}
	if err := WaitForServer(a.AccountsApiUrl + "/status"); err != nil {
		a.Stop()
		return err
	}
	return nil
}

// Kill the accounts server and shutdown the docker container
func (a *AccountsDockerEnv) Stop() (output string) {
	if a.accountsCmd != nil {
		a.accountsCmd.Process.Signal(syscall.SIGTERM)
		select {
		case <-a.accountsWaiter:
			// should probably check error here and do something with it
		case <-time.After(10 * time.Second):
			a.accountsCmd.Process.Kill()
		}
		pid := a.accountsCmd.ProcessState.Pid()
		fmt.Println("EXITED PID", pid)
		output = a.cmdOutput.String()
		if a.DumpLog {
			fmt.Println(output)
		}
	}
	if a.mysqlContainer != nil {
		fmt.Println("Stopping mysql container")
		a.mysqlContainer.Stop(true, 5, true)
		a.mysqlContainer = nil
	}
	if a.SQSTestSuite.Container != nil {
		fmt.Println("Stopping SQS container")
		a.SQSTestSuite.TearDownSuite()
	}

	return output
}
