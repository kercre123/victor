package jdocsdb

import (
	"encoding/json"
	"fmt"
	"strings"

	"github.com/anki/sai-awsutil/dynamoutil"
	"github.com/aws/aws-sdk-go/aws"
)

const (
	// DocType specifies what a document is associated with, which affects the
	// exact database attributes that are used for indexing and searching.
	DocTypeUnknown      = "Unknown"
	DocTypeAccount      = "Acct"
	DocTypeThing        = "Thng"
	DocTypeAccountThing = "AcctThng"
)

const (
	// Attribute naming guidelines:
	//   - Use "_" to avoid all the DynamoDB reserved words for attribute names
	//   - Keep short, so less DB storage is needed
	attrCreated             = "_created"
	attrUpdated             = "_updated"
	attrDocName             = "_name"
	attrAccount             = "_a"
	attrAccountDocName      = "_a_name"
	attrThing               = "_t"
	attrThingDocName        = "_t_name"
	attrAccountThingDocName = "_a_t_name"
	attrDocVersion          = "_dv"
	attrFmtVersion          = "_fv"
	attrClientMetadata      = "_md"
	attrJsonDoc             = "_json"
)

//////////////////////////////////////////////////////////////////////
// Simple Helper Types
//////////////////////////////////////////////////////////////////////

// JdocKey is all the information needed to identify and locate a specific JDOCS
// document for a specific Account, Thing, or Account+Thing.
type JdocKey struct {
	TableName string // name (suffix) of database table document is stored in
	Account   string // ignored for DocType==Thing
	Thing     string // ignored for DocType==Account
	DocType   string
	DocName   string
}

// Jdoc is the actual JSON document + metadata stored by the JDOCS system.
type Jdoc struct {
	DocVersion     uint64
	FmtVersion     uint64
	ClientMetadata string
	JsonDoc        string
}

// ValidateJSON makes sure the document is valid JSON. Ultimately, this will
// probably be evaluated differently, but for now this extra routine is useful
// to detect and report client errors.
func (d *Jdoc) ValidateJSON() error {
	var f interface{}
	return json.Unmarshal([]byte(d.JsonDoc), &f)
}

// ReadDocsInput is a set of document names to read. A map is used for easy and
// efficient lookup of values. The value of the bool is ignored.
type ReadDocsInput map[string]bool

// ReadDocsOutput is a set of Jdoc documents that have been read. A map is used
// for easy and efficient lookup of values.
type ReadDocsOutput map[string]*Jdoc

//////////////////////////////////////////////////////////////////////
// Misc Helper Functions
//////////////////////////////////////////////////////////////////////

// TableDocType extracts the DocType from the table name.
func TableDocType(tableName string) string {
	// DocType is determined by first part of the table name
	allowedDocTypes := []string{
		DocTypeAccountThing,
		DocTypeAccount,
		DocTypeThing,
	}
	for _, docType := range allowedDocTypes {
		if strings.HasPrefix(tableName, docType+"-") {
			return docType
		}
	}
	return DocTypeUnknown
}

// dbKeyItem constructs an item for the DynamoDB primary key for a Jdoc.
func (jdb *JdocsDB) dbKeyItem(docKey JdocKey) (dynamoutil.Item, error) {
	item := dynamoutil.NewItem()

	if jdb.db.GetTable(docKey.TableName) == nil {
		return item, fmt.Errorf("TableName=%s is not defined in the database", docKey.TableName)
	}

	// The DynamoDB primary key (HASH and SORT/RANGE) is different for documents
	// depending on whether they are associated Account, Thing, or AccountThing.
	// This is carefully designed to give efficient QUERY/SCAN performance for
	// common use cases, and use minimal Capacity Units for read/write operations.
	switch docKey.DocType {
	case DocTypeAccount:
		item.SetStringValue(attrAccount, docKey.Account)                           // HASH
		item.SetStringValue(attrAccountDocName, docKey.Account+"_"+docKey.DocName) // RANGE
	case DocTypeThing:
		item.SetStringValue(attrThing, docKey.Thing)                           // HASH
		item.SetStringValue(attrThingDocName, docKey.Thing+"_"+docKey.DocName) // RANGE
	case DocTypeAccountThing:
		item.SetStringValue(attrAccount, docKey.Account)                                                 // HASH
		item.SetStringValue(attrAccountThingDocName, docKey.Account+"_"+docKey.Thing+"_"+docKey.DocName) // RANGE
	default: // it is required and assumed that DocType is valid
		return item, fmt.Errorf("DocType=%s is invalid", docKey.DocType)
	}

	return item, nil
}

//////////////////////////////////////////////////////////////////////

// JdocsDB is the set of database tables for used for all documents managed by
// JDOCS.
type JdocsDB struct {
	db *dynamoutil.DB
}

// NewJdocsDB creates a new JdocsDB object.
//
// tables is the set of tables needed. It is used to initialize all the tables,
// including the primary key definition. The DocType for each table is
// determined by the table name itself.
func NewJdocsDB(tables *map[string]bool, prefix string, cfg *aws.Config) (*JdocsDB, error) {
	// Build the slice of arguments to pass to dynamoutil.NewDB()
	withList := make([]dynamoutil.DBConfig, 0) // not a speech impediment
	for tableName, _ := range *tables {
		docType := TableDocType(tableName)
		switch docType {
		case DocTypeAccount:
			withList = append(withList, dynamoutil.WithTable(&dynamoutil.TableSpec{
				Name: tableName,
				PrimaryKey: dynamoutil.KeySpec{
					KeyName:    attrAccount,
					SubkeyName: attrAccountDocName,
					SubkeyType: dynamoutil.AttributeTypeString,
				},
			}))
		case DocTypeThing:
			withList = append(withList, dynamoutil.WithTable(&dynamoutil.TableSpec{
				Name: tableName,
				PrimaryKey: dynamoutil.KeySpec{
					KeyName:    attrThing,
					SubkeyName: attrThingDocName,
					SubkeyType: dynamoutil.AttributeTypeString,
				},
			}))
		case DocTypeAccountThing:
			withList = append(withList, dynamoutil.WithTable(&dynamoutil.TableSpec{
				Name: tableName,
				PrimaryKey: dynamoutil.KeySpec{
					KeyName:    attrAccount,
					SubkeyName: attrAccountThingDocName,
					SubkeyType: dynamoutil.AttributeTypeString,
				},
			}))
		default:
			return nil, fmt.Errorf("DocType=%s not supported", TableDocType(tableName))
		}
	}

	withList = append(withList, dynamoutil.WithConfig(cfg))
	withList = append(withList, dynamoutil.WithPrefix(prefix+"-")) // XXX

	db, err := dynamoutil.NewDB(withList...)
	if err != nil {
		return nil, err
	}
	return &JdocsDB{db: db}, nil
}

// CreateTables creates the tables that have been defined.
func (jdb *JdocsDB) CreateTables() error {
	return jdb.db.CreateTables()
}

// CheckTables checks the tables that have been created.
func (jdb *JdocsDB) CheckTables() error {
	return jdb.db.CheckTables()
}
