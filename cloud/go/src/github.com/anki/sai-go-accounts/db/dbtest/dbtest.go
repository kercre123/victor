// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// dbtest provides database suport for unit test suites
package dbtest

import (
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"testing"

	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-util"
	"github.com/anki/sai-go-util/dockerutil"
	_ "github.com/go-sql-driver/mysql"
	"github.com/stretchr/testify/suite"
)

var (
	defaultContainerAction       = "PURGE" // stop and delete mysql containers by default after tests complete.
	defaultContainerTimeout uint = 10      // wait up to 10 seconds for a docker container to stop gracefully.
)

// Create a test suite that knows how to setup & teardown a mysql db container
// and can reset the state of the database between individual tests.
type DBSuite struct {
	suite.Suite
	activeContainer *dockerutil.Container
}

func (s *DBSuite) IsSqlite() bool {
	return os.Getenv("DB_TYPE") == "sqlite"
}

func (s *DBSuite) SetupSuite() {
	var err error
	dbType := os.Getenv("DB_TYPE")
	switch dbType {
	case "mysql":
		s.activeContainer, err = dockerutil.StartMysqlContainer(db.MysqlPassword, db.MysqlDb)
		if err != nil {
			s.T().Fatalf("Failed to start docker container for MySQL; skipping tests: %s", err)
		}
		if err = s.activeContainer.WaitForContainer(); err != nil {
			s.T().Skipf("Failed to start docker container for local MySQL server; skipping tests: %s", err)
		}

		db.MysqlIpAddress = s.activeContainer.IpAddress()
		db.MysqlPort = s.activeContainer.Port()

	case "sqlite":
		// "go test" runs tests for multiple packages in parallel by default
		// Create a unique filename for sqlite to use so tests don't race against each other
		// trying to write to the same file
		f, err := ioutil.TempFile("", "accounts-test-db")
		if err != nil {
			s.T().Fatalf("Failed to create temporary database file for sqlite; skipping tets: %s", err)
		}
		db.SqliteDbPath = f.Name()
	}
	if err = db.OpenDb(); err != nil {
		s.T().Fatalf("Failed to initialize database; skipping tests: %s", err)
	}
}

func (s *DBSuite) TearDownSuite() {
	var terminateAction = defaultContainerAction
	util.SetFromEnv(&terminateAction, "DOCKER_TERMINATE_ACTION")
	terminateAction = strings.ToUpper(terminateAction)
	if s.activeContainer != nil {
		switch terminateAction {
		case "PURGE":
			fmt.Println("Stopping & Deleting Container " + s.activeContainer.Container.ID)
			s.activeContainer.Stop(true, defaultContainerTimeout, true)
			os.Remove(db.SqliteDbPath)
		case "STOP":
			fmt.Println("Stopping Container " + s.activeContainer.Container.ID)
			s.activeContainer.Stop(false, defaultContainerTimeout, true)
			os.Remove(db.SqliteDbPath)
		default:
			fmt.Printf("Leaving container %s intact at IP address %s\n", s.activeContainer.Container.ID, s.activeContainer.IpAddress())
		}
	}
}

// reset db state between tests
func (s *DBSuite) SetupTest() {
	if err := RecreateTestTables(); err != nil {
		panic(err)
	}
}

func RecreateTestTables() error {
	if os.Getenv("DB_TYPE") == "mysql" {
		// we don't want to enforce referential integrity while recreating tables
		db.Dbmap.Exec("SET FOREIGN_KEY_CHECKS=0")
		defer db.Dbmap.Exec("SET FOREIGN_KEY_CHECKS=1")
	}

	if err := db.Dbmap.DropTablesIfExists(); err != nil {
		return err
	}
	return db.CreateTables()
}
func RunDbTest(t *testing.T, f func()) {
	if db.Dbmap == nil {
		if err := db.OpenDb(); err != nil {
			t.Skipf("Skipped as DB is not available err=%s", err)
			return
		}
	}
	if db.DbType == "sqlitex" {
		db.Dbmap.Exec("PRAGMA synchronous = OFF")
	}
	if err := RecreateTestTables(); err != nil {
		t.Skipf("Failed to create test db tables: %s", err)
		return
	}
	f()
}
