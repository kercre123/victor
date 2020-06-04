// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The db pacakge deals with configurating and initializing a database
package db

import (
	"database/sql"
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
	"github.com/coopernurse/gorp"
	"github.com/go-sql-driver/mysql"
	"github.com/mattn/go-sqlite3"
)

var (
	Dbmap  *gorp.DbMap
	Tables []Table

	DbType         string
	DbTrace        bool
	MysqlUser      = "root"
	MysqlPassword  = "root"
	MysqlDb        = "testdb"
	MysqlIpAddress string
	MysqlPort      = "3306"

	SqliteDbPath = "/tmp/test.db"
	//SqliteDbPath = ":memory:"

	// If the DB type supports "select .. for update" queries,
	// then this will be set to "FOR UPDATE" once the DB is opened.
	ForUpdate = ""

	// Map table name to a comma separated list of columns (table.col1, table.col2, table.col3) for a table
	// populated by OpenDb()
	ColumnsFor = map[string]string{}
)

func init() {
	envconfig.String(&DbType, "DB_TYPE", "", "Database type (mysql|sqlite)")
	envconfig.String(&MysqlUser, "MYSQL_USER", "", "MySQL username")
	envconfig.String(&MysqlPassword, "MYSQL_PASSWORD", "", "MySQL password")
	envconfig.String(&MysqlDb, "MYSQL_DB", "", "MySQL database name")
	envconfig.String(&MysqlIpAddress, "MYSQL_IPADDR", "", "MySQL IP address")
	envconfig.String(&MysqlPort, "MYSQL_PORT", "", "MySQL TCP Port")
	envconfig.String(&SqliteDbPath, "SQLITE_DB_PATH", "", "Sqlite database filename")
	envconfig.Bool(&DbTrace, "DB_TRACE", "dbtrace", "Enables logging of all database queries")
}

type Table struct {
	Model interface{}
	Name  string
}

var (
	// Used for id offsets; do not change.
	idEpoch = time.Date(2014, time.January, 1, 0, 0, 0, 0, time.UTC)
)

// AssignId assigns a 21 byte random id to each row.
//
// The Id uses characters from the set "23456789abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ"
//
// The first 4 bytes encode the number of hours since idEpoch
// which is meant to help bucketing of indexes in the DB.
//
// The remaining bytes are random.
func AssignId(obj interface{}) error {
	// Add a prefix to the id representing the number of hours since the epoch
	// 50 years == 19 bits.
	delta := uint64(time.Since(idEpoch).Hours())
	timePrefix := strutil.EncodeInt(strutil.AllSafe, 4, delta) // 4 bytes

	// 19 bytes from the AllChar set gets us ~108 bits of randomness
	uuid, err := strutil.Str(strutil.AllSafe, 19)
	if err != nil {
		return err
	}

	uuid = timePrefix + uuid
	if len(uuid) != 23 {
		panic("Bad ID! " + uuid)
	}

	entry := reflect.ValueOf(obj).Elem()
	if f := entry.FieldByName("Id"); f.IsValid() {
		f.SetString(uuid)
		return nil
	}
	panic("AssignId expects a struct with an Id field!")
}

// models register their defined Table models in init() using this function.
// initdb() registers these Tables with gorp at startup.
func RegisterTable(name string, model interface{}) {
	Tables = append(Tables, Table{model, name})
}

func openMysqlDb() error {
	db, err := sql.Open("mysql",
		fmt.Sprintf("%s:%s@tcp(%s:%s)/%s?parseTime=true",
			MysqlUser, MysqlPassword, MysqlIpAddress, MysqlPort, MysqlDb))
	for i := 0; i < 40; i++ {
		err = db.Ping()
		if err == nil {
			if i > 0 {
				fmt.Println("Connected.")
			}
			break
		}
		if i == 0 {
			fmt.Print("Waiting on MySQL..")
		} else {
			fmt.Print(".")
		}
		time.Sleep(500 * time.Millisecond)
	}
	if err != nil {
		panic(fmt.Sprintf("Failed to create test database: %s", err))
	}

	// construct a gorp DbMap
	Dbmap = &gorp.DbMap{Db: db, Dialect: gorp.MySQLDialect{
		Engine: "InnoDB", Encoding: "utf8",
	}}
	ForUpdate = "FOR UPDATE"
	return nil
}

func openSqliteDb() error {
	// connect to db using standard Go database/sql API
	// use whatever database/sql driver you wish
	db, err := sql.Open("sqlite3", SqliteDbPath)
	if err != nil {
		return err
	}

	// construct a gorp DbMap
	Dbmap = &gorp.DbMap{Db: db, Dialect: gorp.SqliteDialect{}}

	return nil
}

func OpenDb() error {
	switch DbType {
	case "mysql":
		if err := openMysqlDb(); err != nil {
			return err
		}
	case "sqlite":
		if err := openSqliteDb(); err != nil {
			return err
		}
	default:
		return fmt.Errorf("Unknown DB type %q - Must be one of \"sqlite\" or \"mysql\"", DbType)
	}

	if DbTrace {
		Dbmap.TraceOn("db", alog.Logger)
	}

	// register Tables with gorp
	for _, t := range Tables {
		var tm *gorp.TableMap

		// Find the struct field marked as the primary key, defaults to "Id" if none set
		typ := reflect.TypeOf(t.Model)
		field, found := typ.FieldByNameFunc(func(name string) bool {
			field, _ := typ.FieldByName(name)
			for _, flag := range strings.Split(field.Tag.Get("dbflags"), ",") {
				if flag == "prikey" {
					return true
				}
			}
			return false
		})

		if found {
			tm = Dbmap.AddTableWithName(t.Model, t.Name).SetKeys(false, field.Name)
		} else {
			tm = Dbmap.AddTableWithName(t.Model, t.Name).SetKeys(false, "Id")
		}

		// build a list of all known column names used in place of "*" in "SELECT *" queries
		// Allows new columns to be added to the DB before they're added to the model.
		cols := make([]string, 0, len(tm.Columns))
		for _, c := range tm.Columns {
			if c.ColumnName != "-" {
				cols = append(cols, Dbmap.Dialect.QuotedTableForQuery("", t.Name)+"."+Dbmap.Dialect.QuoteField(c.ColumnName))
			}
		}
		ColumnsFor[t.Name] = strings.Join(cols, ", ")
	}

	return nil
}

// returns true if the error represents a sql constraint violation (duplicate key, etc)
func IsConstraintViolation(err interface{}) bool {
	switch v := err.(type) {
	case sqlite3.Error:
		return v.Code == sqlite3.ErrConstraint
	case mysql.MySQLError:
		return v.Number == 0x426
	case *mysql.MySQLError:
		return v.Number == 0x426
	}
	return false
}

func CreateTables() error {
	schemas := dbSchemas[DbType]
	for _, table := range Tables {
		if err := CreateSchema(schemas[table.Name]); err != nil {
			//fmt.Printf("Failed to create table %s: %s\n", table.Name, schemas[table.Name])
			return fmt.Errorf("Failed to create table %s: %s", table.Name, err)
		}
	}
	return nil
}

func CreateSchema(schema string) error {
	_, err := Dbmap.Exec(schema)
	if err != nil {
		fmt.Printf("Failed to create schema error=%v schema=%s\n\n", err, schema)
	}
	return err
}

// Checks that the database is healthy and returns true if ok
func CheckStatus() error {
	_, err := Dbmap.SelectStr("SELECT apikey_id FROM apikeys LIMIT 1")
	if err != nil {
		alog.Warn{"action": "db_check", "status": "failed", "error": err}.Log()
	}
	return err
}
