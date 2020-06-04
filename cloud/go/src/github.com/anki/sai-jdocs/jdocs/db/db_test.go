package jdocsdb

import (
	"encoding/json"
	"fmt"
	"math"
	"testing"

	"github.com/anki/sai-go-util/dockerutil"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
)

const (
	// These constants determine the full space of different documents
	// (names/accounts/things) that are used in test cases. For decent test
	// coverage, each of these should be >= 2.
	numDocsPerType = 2
	numAccounts    = 2
	numThings      = 2

	testTableAcct     = DocTypeAccount + "-table"
	testTableThng     = DocTypeThing + "-table"
	testTableAcctThng = DocTypeAccountThing + "-table"
)

type JdocsDBSuite struct {
	t          *testing.T
	ds         dockerutil.DynamoSuite
	testDocSet map[string]string // DocName --> TableName
	jdb        *JdocsDB
}

func (s *JdocsDBSuite) SetT(t *testing.T) {
	s.t = t
}

func (s *JdocsDBSuite) T() *testing.T {
	return s.t
}

// SetupSuite() is called once, prior to any test cases being run.
func (s *JdocsDBSuite) SetupSuite() {
	s.ds.SetupSuite()
	tablePrefix := "unstable"

	// define the set of documents that will be used by the test cases
	s.testDocSet = make(map[string]string)
	for i := 0; i < numDocsPerType; i++ {
		s.testDocSet[makeTestDocName(DocTypeAccount /**/, i+1)] = testTableAcct
		s.testDocSet[makeTestDocName(DocTypeThing /****/, i+1)] = testTableThng
		s.testDocSet[makeTestDocName(DocTypeAccountThing, i+1)] = testTableAcctThng
	}

	// create test harness database tables
	tables := make(map[string]bool) // set of TableName values
	for _, tableName := range s.testDocSet {
		tables[tableName] = true
	}

	var err error
	s.jdb, err = NewJdocsDB(&tables, tablePrefix, s.ds.Config)
	if err != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", err))
	}
	err = s.jdb.CreateTables()
	if err != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", err))
	}
}

func (s *JdocsDBSuite) TearDownSuite() {
	s.ds.TearDownSuite()
}

// Because "go test" doesn't know anything about the testify/suite
// package, we need one file-level test case that instantiates and
// runs the suite.
func TestJdocsDBSuite(t *testing.T) {
	suite.Run(t, new(JdocsDBSuite))
}

//////////////////////////////////////////////////////////////////////

// makeTestDocName constructs a simple and predictable document name for use in
// the test harness
func makeTestDocName(docType string, index int) string {
	return fmt.Sprintf("%s.Doc%d", docType, index)
}

func makeTestJdocKey(tableName, docName string, aNum, tNum int) *JdocKey {
	key := JdocKey{
		TableName: tableName,
		Account:   fmt.Sprintf("a%d", aNum),
		Thing:     fmt.Sprintf("t%d", tNum),
		DocType:   TableDocType(tableName),
		DocName:   docName,
	}
	if key.DocType == DocTypeAccount {
		key.Thing = ""
	}
	if key.DocType == DocTypeThing {
		key.Account = ""
	}
	return &key
}

// TestDocData is a simple document type where the fields match the keys and
// metadata.
type TestDocData struct {
	Account    string `json:"Account"`
	Thing      string `json:"Thing"`
	DocName    string `json:"DocName"`
	FmtVersion uint64 `json:"FmtVersion"`
	DocVersion uint64 `json:"DocVersion"`
}

func makeTestJdoc(t *testing.T, doc *TestDocData) *Jdoc {
	if doc.DocVersion == 0 {
		return &Jdoc{
			DocVersion:     0,
			FmtVersion:     0,
			ClientMetadata: "",
			JsonDoc:        "",
		}
	}
	jsonDoc, err := json.Marshal(doc)
	assert.Nil(t, err)
	return &Jdoc{
		DocVersion:     doc.DocVersion,
		FmtVersion:     doc.FmtVersion,
		ClientMetadata: doc.Account + "_" + doc.Thing + "_" + doc.DocName,
		JsonDoc:        string(jsonDoc),
	}
}

// createTestVec is a test vector (single test input case) for CreateDoc()
// testing.
type createTestVec struct {
	errorDesc string
	isBadKey  bool
	key       JdocKey
	doc       Jdoc
}

// updateTestVec is a test vector (single test input case) for UpdateDoc()
// testing.
type updateTestVec struct {
	errorDesc string
	key       JdocKey
	doc       Jdoc
}

// readTestVec is a test vector (single test input case) for ReadDoc() testing.
type readTestVec struct {
	errorDesc string
	key       JdocKey
}

// readsTestVec is a test vector (single test input case) for ReadDocs() testing.
type readsTestVec struct {
	errorDesc  string
	key        JdocKey
	docsToRead ReadDocsInput
}

//////////////////////////////////////////////////////////////////////

// ActOnTestDocs iterates through a set of Accounts a[1]..a[numAccounts] and a
// set of Things t[1]..t[numThings], then performs some action on all
// permutations of Account/Thing/AccountThing documents for those sets of
// Accounts and Things.
//
// FmtVersion and DocVersion are specified explicity.
//
// Document contents (JSON), metadata, etc are simple for easy verification.
func (s *JdocsDBSuite) ActOnTestDocs(t *testing.T, jdb *JdocsDB, fmtVersion, docVersion uint64, actFn func(*JdocKey, *Jdoc)) {
	for docName, tableName := range s.testDocSet {
		tdd := TestDocData{
			DocName:    docName,
			FmtVersion: fmtVersion,
			DocVersion: docVersion,
		}
		docType := TableDocType(tableName)
		switch docType {
		case DocTypeAccount:
			for acct := 0; acct < numAccounts; acct++ {
				key := makeTestJdocKey(tableName, docName, acct+1, 0)
				tdd.Account = key.Account
				tdd.Thing = key.Thing
				doc := makeTestJdoc(t, &tdd)
				actFn(key, doc)
			}
		case DocTypeThing:
			for thng := 0; thng < numThings; thng++ {
				key := makeTestJdocKey(tableName, docName, 0, thng+1)
				tdd.Account = key.Account
				tdd.Thing = key.Thing
				doc := makeTestJdoc(t, &tdd)
				actFn(key, doc)
			}
		case DocTypeAccountThing:
			for acct := 0; acct < numAccounts; acct++ {
				for thng := 0; thng < numThings; thng++ {
					key := makeTestJdocKey(tableName, docName, acct+1, thng+1)
					tdd.Account = key.Account
					tdd.Thing = key.Thing
					doc := makeTestJdoc(t, &tdd)
					actFn(key, doc)
				}
			}
		}
	}
}

// CreateTestDocs performs a CreateDoc() action on all documents in the test
// set. See ActOnTestDocs().
func (s *JdocsDBSuite) CreateTestDocs(t *testing.T, jdb *JdocsDB, fmtVersion, docVersion uint64) {
	actFn := func(key *JdocKey, doc *Jdoc) {
		err := jdb.CreateDoc(*key, *doc)
		assert.Nil(t, err)
	}
	s.ActOnTestDocs(t, jdb, fmtVersion, docVersion, actFn)
}

// UpdateTestDocs performs a UpdateDoc() action on all documents in the test
// set. See ActOnTestDocs().
func (s *JdocsDBSuite) UpdateTestDocs(t *testing.T, jdb *JdocsDB, fmtVersion, docVersion uint64) {
	actFn := func(key *JdocKey, doc *Jdoc) {
		err := jdb.UpdateDoc(*key, *doc)
		assert.Nil(t, err)
	}
	s.ActOnTestDocs(t, jdb, fmtVersion, docVersion, actFn)
}

// VerifyTestDocs performs a ReadDoc() + verify action on all documents in the
// test set. See ActOnTestDocs().
func (s *JdocsDBSuite) VerifyTestDocs(t *testing.T, jdb *JdocsDB, fmtVersion, docVersion uint64) {
	actFn := func(key *JdocKey, doc *Jdoc) {
		gotJdoc, err := jdb.ReadDoc(*key, false)
		assert.Nil(t, err)
		assert.Equal(t, *doc, *gotJdoc)
	}
	s.ActOnTestDocs(t, jdb, fmtVersion, docVersion, actFn)
}

// DeleteTestDocs performs a DeleteDoc() action on all documents in the test
// set. See ActOnTestDocs().
func (s *JdocsDBSuite) DeleteTestDocs(t *testing.T, jdb *JdocsDB) {
	actFn := func(key *JdocKey, doc *Jdoc) {
		err := jdb.DeleteDoc(*key)
		assert.Nil(t, err)
	}
	s.ActOnTestDocs(t, jdb, 0, 0, actFn)
}

//////////////////////////////////////////////////////////////////////

func (s *JdocsDBSuite) TestCreateDoc() {
	t := s.T()

	// no documents should exist
	fmtVersion := uint64(0)
	docVersion := uint64(0)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// create and verify documents with "expected" version numbers
	fmtVersion = uint64(1)
	docVersion = uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// delete documents, and verify they don't exist anymore
	s.DeleteTestDocs(t, s.jdb)
	fmtVersion = uint64(0)
	docVersion = uint64(0)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// create and verify same documents, with different version numbers
	// (CreateDoc() doesn't check version values)
	fmtVersion = uint64(2)
	docVersion = uint64(1) // avoid 0, because that is special case for helper functions
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// delete documents, and verify they don't exist anymore
	s.DeleteTestDocs(t, s.jdb)
	fmtVersion = uint64(0)
	docVersion = uint64(0)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Test special cases
	key := JdocKey{
		TableName: testTableAcct,
		Account:   "a1",
		Thing:     "",
		DocType:   DocTypeAccount,
		DocName:   "MyDoc",
	}
	doc := Jdoc{
		DocVersion:     0,              // special case
		FmtVersion:     math.MaxUint64, // special case = largest possible fmt version
		ClientMetadata: "",             // special case = no client metadata
		JsonDoc:        "{}",           // special case = empty JSON doc
	}

	cerr := s.jdb.CreateDoc(key, doc)
	assert.Nil(t, cerr)

	gotJdoc, rerr := s.jdb.ReadDoc(key, false)
	assert.Nil(t, rerr)
	assert.Equal(t, doc, *gotJdoc)

	derr := s.jdb.DeleteDoc(key)
	assert.Nil(t, derr)
}

func (s *JdocsDBSuite) TestCreateDocNewErrors() {
	t := s.T()
	errorVectors := []createTestVec{
		createTestVec{
			"Invalid JSON: Empty string",
			false, /* isBadKey */
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: ``},
		},
		createTestVec{
			"Invalid JSON: Misc text",
			false, /* isBadKey */
			JdocKey{TableName: testTableAcct, Account: "a2", Thing: "", DocType: DocTypeAccount, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `This is not JSON`},
		},
		createTestVec{
			"Invalid JSON: Missing curly brace",
			false, /* isBadKey */
			JdocKey{TableName: testTableAcct, Account: "a3", Thing: "", DocType: DocTypeAccount, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{"Field1": "Value1" `},
		},
		createTestVec{
			"Account is empty string for Account-keyed doc",
			true, /* isBadKey */
			JdocKey{TableName: testTableAcct, Account: "", Thing: "", DocType: DocTypeAccount, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		},
		createTestVec{
			"Account is empty string for AccountThing-keyed doc",
			true, /* isBadKey */
			JdocKey{TableName: testTableAcctThng, Account: "", Thing: "t1", DocType: DocTypeAccountThing, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		},
		createTestVec{
			"Thing is empty string for Thing-keyed doc",
			true, /* isBadKey */
			JdocKey{TableName: testTableThng, Account: "", Thing: "", DocType: DocTypeThing, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		},
		// createTestVec{
		// 	// XXX: This particular situation is not checked explicitly, and does not cause DynamoDB error
		// 	"Thing is empty string for AccountThing-keyed doc",
		// 	true, /* isBadKey */
		// 	JdocKey{TableName: testTableAcctThng, Account: "a1", Thing: "", DocType: DocTypeAccountThing, DocName: "MyDoc"},
		// 	Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		// },
		createTestVec{
			"Invalid DocType",
			true, /* isBadKey */
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: "DocTypeShouldCauseError", DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		},
		createTestVec{
			"Invalid TableName",
			true, /* isBadKey */
			JdocKey{TableName: DocTypeAccount + "-bad-table", Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: "MyDoc"},
			Jdoc{DocVersion: 1, FmtVersion: 1, ClientMetadata: "", JsonDoc: `{}`},
		},
	}

	for i, vec := range errorVectors {
		cerr := s.jdb.CreateDoc(vec.key, vec.doc)
		if cerr == nil {
			assert.Fail(t, fmt.Sprintf("Test vector [%d] did not cause an error. errorDesc=%s: ", i, vec.errorDesc))
		}
		if !vec.isBadKey {
			// valid key => ReadDoc() should return empty doc, no error
			gotJdoc, rerr := s.jdb.ReadDoc(vec.key, true)
			assert.Nil(t, rerr)
			expDoc := Jdoc{DocVersion: 0, FmtVersion: 0, ClientMetadata: "", JsonDoc: ""}
			assert.Equal(t, expDoc, *gotJdoc)
		}
	}
}

func (s *JdocsDBSuite) TestCreateDocDuplicateErrors() {
	t := s.T()

	// Create legit docs (no errors)
	doc := Jdoc{
		DocVersion:     1,
		FmtVersion:     1,
		ClientMetadata: "",
		JsonDoc:        "{}",
	}

	docKeys := []JdocKey{
		JdocKey{
			TableName: testTableAcct,
			Account:   "a1",
			Thing:     "t1",
			DocType:   DocTypeAccount,
			DocName:   "MyDocA",
		},
		JdocKey{
			TableName: testTableThng,
			Account:   "a1",
			Thing:     "t1",
			DocType:   DocTypeThing,
			DocName:   "MyDocT",
		},
		JdocKey{
			TableName: testTableAcctThng,
			Account:   "a1",
			Thing:     "t1",
			DocType:   DocTypeAccountThing,
			DocName:   "MyDocAT",
		},
	}
	for _, key := range docKeys {
		cerr := s.jdb.CreateDoc(key, doc)
		assert.Nil(t, cerr)
	}

	// Try to create documents that conflict with existing documents
	// => expect errors
	doc.DocVersion = 2
	for _, key := range docKeys {
		cerr := s.jdb.CreateDoc(key, doc)
		assert.NotNil(t, cerr)
	}

	newKeys := []JdocKey{
		JdocKey{
			TableName: testTableAcct,
			Account:   "a1",
			Thing:     "t2", // error: different Thing value doesn't matter for DocTypeAccount
			DocType:   DocTypeAccount,
			DocName:   "MyDocA",
		},
		JdocKey{
			TableName: testTableThng,
			Account:   "a2", // error: different Account value doesn't matter for DocTypeThing
			Thing:     "t1",
			DocType:   DocTypeThing,
			DocName:   "MyDocT",
		},
	}
	for _, key := range newKeys {
		cerr := s.jdb.CreateDoc(key, doc)
		assert.NotNil(t, cerr)
	}

	// DB cleanup: Delete all documents
	for _, key := range docKeys {
		derr := s.jdb.DeleteDoc(key)
		assert.Nil(t, derr)
	}
}

func (s *JdocsDBSuite) TestUpdateDoc() {
	t := s.T()

	// Create first version of documents, then verify
	fmtVersion := uint64(1)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Update and verify documents
	fmtVersion = uint64(1)
	docVersion = uint64(2) // +1
	s.UpdateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	fmtVersion = uint64(1)
	docVersion = uint64(3) // +1
	s.UpdateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	fmtVersion = uint64(2) // +1
	docVersion = uint64(4) // +1
	s.UpdateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	fmtVersion = uint64(4) // +2
	docVersion = uint64(5) // +1
	s.UpdateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestUpdateDocErrors() {
	t := s.T()

	// Create first version of documents, then verify
	fmtVersion := uint64(2)
	docVersion := uint64(2)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	errorVectors := []updateTestVec{
		updateTestVec{
			"Invalid JSON: Empty document",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: ``},
		},
		updateTestVec{
			"Invalid JSON: Mal-formed",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{"Field1": "Val1" `},
		},
		updateTestVec{
			"Bad DocVersion (too low)",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Bad DocVersion (too low -1)",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion - 1, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Bad DocVersion (too high)",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion + 2, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Bad FmtVersion (too low)",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion - 1, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Account is empty string for Account-keyed doc",
			JdocKey{TableName: testTableAcct, Account: "", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Account is empty string for AccountThing-keyed doc",
			JdocKey{TableName: testTableAcctThng, Account: "", Thing: "t1", DocType: DocTypeAccountThing, DocName: makeTestDocName(DocTypeThing, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Thing is empty string for Thing-keyed doc",
			JdocKey{TableName: testTableThng, Account: "", Thing: "", DocType: DocTypeThing, DocName: makeTestDocName(DocTypeAccountThing, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		// updateTestVec{
		//  // XXX: This particular situation is not checked explicitly, and does not cause DynamoDB error
		// 	"Thing is empty string for AccountThing-keyed doc",
		// 	JdocKey{TableName: testTableAcctThng, Account: "a1", Thing: "", DocType: DocTypeAccountThing, DocName: makeTestDocName(DocTypeAccountThing, 1)},
		//  Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		// },
		updateTestVec{
			"Invalid DocType",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: "DocTypeShouldCauseError", DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Invalid TableName",
			JdocKey{TableName: DocTypeAccount + "-bad-table", Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Account document doesn't exist",
			JdocKey{TableName: testTableAcct, Account: "a0", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"Thing document doesn't exist",
			JdocKey{TableName: testTableThng, Account: "", Thing: "t1", DocType: DocTypeThing, DocName: makeTestDocName(DocTypeThing, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"AccountThing document doesn't exist",
			JdocKey{TableName: testTableAcctThng, Account: "a0", Thing: "t0", DocType: DocTypeAccountThing, DocName: makeTestDocName(DocTypeAccountThing, 1)},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
		updateTestVec{
			"DocName doesn't exist",
			JdocKey{TableName: testTableAcctThng, Account: "a1", Thing: "t1", DocType: DocTypeAccountThing, DocName: "NoDoc"},
			Jdoc{DocVersion: docVersion, FmtVersion: fmtVersion, ClientMetadata: "", JsonDoc: `{}`},
		},
	}

	for i, vec := range errorVectors {
		uerr := s.jdb.UpdateDoc(vec.key, vec.doc)
		if uerr == nil {
			assert.Fail(t, fmt.Sprintf("Test vector [%d] did not cause an error. errorDesc=%s: ", i, vec.errorDesc))
		}
	}

	// Since all of of the updates should have had errors, we can verify the
	// ORIGINAL values for each document
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestReadDoc() {
	t := s.T()

	// Create documents to read
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read documents with different value for the "unused" key
	//  => Should return the correct non-empty document
	doExistKeys := []*JdocKey{
		makeTestJdocKey(testTableAcct, makeTestDocName(DocTypeAccount, 1), 1, numThings+1),
		makeTestJdocKey(testTableThng, makeTestDocName(DocTypeThing, 1), numAccounts+1, 1),
	}
	for _, key := range doExistKeys {
		gotJdoc, rerr := s.jdb.ReadDoc(*key, true)
		assert.Nil(t, rerr)
		assert.Equal(t, fmtVersion, gotJdoc.FmtVersion)
		assert.Equal(t, docVersion, gotJdoc.DocVersion)
		expMetadata := "IsReplacedBelow"
		switch TableDocType(key.TableName) {
		case DocTypeAccount:
			expMetadata = key.Account + "_" + "" + "_" + key.DocName
		case DocTypeThing:
			expMetadata = "" + "_" + key.Thing + "_" + key.DocName
		}
		assert.Equal(t, expMetadata, gotJdoc.ClientMetadata)
		// XX: No checking of JsonDoc field
	}

	// DocName/Account/Thing doesn't exist => read empty doc (no error)
	noExistKeys := []*JdocKey{
		makeTestJdocKey(testTableAcct, makeTestDocName(DocTypeAccount, 1), numAccounts+1, 0),
		makeTestJdocKey(testTableThng, makeTestDocName(DocTypeThing, 1), 0, numThings+1),
		makeTestJdocKey(testTableAcctThng, makeTestDocName(DocTypeAccountThing, 1), numAccounts+1, 1),
		makeTestJdocKey(testTableAcctThng, makeTestDocName(DocTypeAccountThing, 1), 1, numThings+1),
		makeTestJdocKey(testTableAcct, "InvalidDocName", 1, 1),
		makeTestJdocKey(testTableThng, "InvalidDocName", 1, 1),
		makeTestJdocKey(testTableAcctThng, "InvalidDocName", 1, 1),
	}
	for _, key := range noExistKeys {
		gotJdoc, rerr := s.jdb.ReadDoc(*key, true)
		assert.Nil(t, rerr)
		assert.Equal(t, Jdoc{}, *gotJdoc)
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestReadDocErrors() {
	t := s.T()

	// Create documents to read
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	errorVectors := []readTestVec{
		readTestVec{
			"Account is empty string for Account-keyed doc",
			JdocKey{TableName: testTableAcct, Account: "", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
		},
		readTestVec{
			"Account is empty string for AccountThing-keyed doc",
			JdocKey{TableName: testTableAcctThng, Account: "", Thing: "t1", DocType: DocTypeAccountThing, DocName: makeTestDocName(DocTypeAccountThing, 1)},
		},
		readTestVec{
			"Thing is empty string for Thing-keyed doc",
			JdocKey{TableName: testTableThng, Account: "", Thing: "", DocType: DocTypeThing, DocName: makeTestDocName(DocTypeThing, 1)},
		},
		// readTestVec{
		//  // XXX: This particular situation is not checked explicitly, and does not cause DynamoDB error
		// 	"Thing is empty string for AccountThing-keyed doc",
		// 	JdocKey{TableName: testTableAcctThng, Account: "a1", Thing: "", DocType: DocTypeAccountThing, DocName: makeTestDocName(DocTypeAccountThing, 1)},
		// },
		readTestVec{
			"Invalid DocType",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: "DocTypeShouldCauseError", DocName: makeTestDocName(DocTypeAccount, 1)},
		},
		readTestVec{
			"Invalid TableName",
			JdocKey{TableName: DocTypeAccount + "-bad-table", Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
		},
	}

	for i, vec := range errorVectors {
		_, rerr := s.jdb.ReadDoc(vec.key, false)
		if rerr == nil {
			assert.Fail(t, fmt.Sprintf("Test vector [%d] did not cause an error. errorDesc=%s: ", i, vec.errorDesc))
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestReadDocs() {
	t := s.T()

	// Create documents to read
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents (using ReadDoc())
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Now use ReadDocs() to batch-read different combinations documents, for a
	// specific Account+Thing.
	for _, tableName := range []string{testTableAcct, testTableThng, testTableAcctThng} {
		key := *makeTestJdocKey(tableName, "", 1, 1)

		// read 0,1,.. documents stored in this table. When the document name index
		// exceeds numDocsPerType, we are requesting non-existent documents. For
		// these, the response should be an empty document, no error.
		numNonExistDocs := 3
		for numDocs := 0; numDocs < (numDocsPerType + numNonExistDocs + 1); numDocs++ {
			readDocsI := make(ReadDocsInput)
			for i := 0; i < numDocs; i++ {
				readDocsI[makeTestDocName(TableDocType(tableName), i+1)] = true
			}
			readDocsO := make(ReadDocsOutput)
			rerr := s.jdb.ReadDocs(key, &readDocsI, &readDocsO)
			assert.Nil(t, rerr)
			assert.Equal(t, len(readDocsI), len(readDocsO))

			// Verify document keys in ReadDocsOutput
			for docName, _ := range readDocsI {
				expKey := makeTestJdocKey(tableName, docName, 1, 1)
				_, exists := readDocsO[expKey.DocName]
				assert.True(t, exists)
			}
			// Verify document values in ReadDocsOutput
			for i := 0; i < numDocs; i++ {
				docName := makeTestDocName(TableDocType(tableName), i+1)
				expKey := makeTestJdocKey(tableName, docName, 1, 1)
				expDoc := Jdoc{}
				if i < numDocsPerType {
					expDoc = *makeTestJdoc(t, &TestDocData{
						Account:    expKey.Account,
						Thing:      expKey.Thing,
						DocName:    docName,
						FmtVersion: fmtVersion,
						DocVersion: docVersion,
					})
				}
				gotDoc := *(readDocsO[docName])
				assert.Equal(t, expDoc, gotDoc)
			}
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestReadDocsAccum() {
	t := s.T()

	// Create documents to read
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents (using ReadDoc())
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// For this test, ReadDocOutputs result is not cleared between calls to
	// ReadDocs(). Instead, new results should accumulate.
	readDocsO := make(ReadDocsOutput)
	expDocsRead := 0
	for _, tableName := range []string{testTableAcct, testTableThng, testTableAcctThng} {
		key := *makeTestJdocKey(tableName, "", 1, 1)

		// read 0,1,.. documents stored in this table. Only read documents that
		// actually exist.
		for numDocs := 1; numDocs < (numDocsPerType + 1); numDocs++ {
			readDocsI := make(ReadDocsInput)
			for i := 0; i < numDocs; i++ {
				readDocsI[makeTestDocName(TableDocType(tableName), i+1)] = true
			}
			expDocsRead++
			rerr := s.jdb.ReadDocs(key, &readDocsI, &readDocsO)
			assert.Nil(t, rerr)
			assert.Equal(t, expDocsRead, len(readDocsO))

			// Verify new document keys in ReadDocsOutput
			for docName, _ := range readDocsI {
				expKey := makeTestJdocKey(tableName, docName, 1, 1)
				_, exists := readDocsO[expKey.DocName]
				assert.True(t, exists)
			}
			// Verify all document values read so far in ReadDocsOutput
			for i := 0; i < numDocs; i++ {
				docName := makeTestDocName(TableDocType(tableName), i+1)
				expKey := makeTestJdocKey(tableName, docName, 1, 1)
				expDoc := Jdoc{}
				if i < numDocsPerType {
					expDoc = *makeTestJdoc(t, &TestDocData{
						Account:    expKey.Account,
						Thing:      expKey.Thing,
						DocName:    docName,
						FmtVersion: fmtVersion,
						DocVersion: docVersion,
					})
				}
				gotDoc := *(readDocsO[docName])
				assert.Equal(t, expDoc, gotDoc)
			}
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestReadDocsErrors() {
	t := s.T()

	// Create documents to read
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	errorVectors := []readsTestVec{
		readsTestVec{
			"Account is empty string for Account-keyed doc",
			JdocKey{TableName: testTableAcct, Account: "", Thing: "", DocType: DocTypeAccount},
			ReadDocsInput{makeTestDocName(DocTypeAccount, 1): true},
		},
		readsTestVec{
			"Account is empty string for AccountThing-keyed doc",
			JdocKey{TableName: testTableAcctThng, Account: "", Thing: "t1", DocType: DocTypeAccountThing},
			ReadDocsInput{makeTestDocName(DocTypeAccountThing, 1): true},
		},
		readsTestVec{
			"Thing is empty string for Thing-keyed doc",
			JdocKey{TableName: testTableThng, Account: "", Thing: "", DocType: DocTypeThing},
			ReadDocsInput{makeTestDocName(DocTypeThing, 1): true},
		},
		// readsTestVec{
		//  // XXX: This particular situation is not checked explicitly, and does not cause DynamoDB error
		// 	"Thing is empty string for AccountThing-keyed doc",
		// 	JdocKey{TableName: testTableAcctThng, Account: "a1", Thing: "", DocType: DocTypeAccountThing},
		//  ReadDocsInput{makeTestDocName(DocTypeAccountThing, 1): true},
		// },
		readsTestVec{
			"Invalid DocType",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: "DocTypeShouldCauseError"},
			ReadDocsInput{makeTestDocName(DocTypeAccount, 1): true},
		},
		readsTestVec{
			"Invalid TableName",
			JdocKey{TableName: DocTypeAccount + "-bad-table", Account: "a1", Thing: "", DocType: DocTypeAccount},
			ReadDocsInput{makeTestDocName(DocTypeAccount, 1): true},
		},
	}

	for i, vec := range errorVectors {
		outDocs := make(ReadDocsOutput)
		rerr := s.jdb.ReadDocs(vec.key, &vec.docsToRead, &outDocs)
		if rerr == nil {
			assert.Fail(t, fmt.Sprintf("Test vector [%d] did not cause an error. errorDesc=%s: ", i, vec.errorDesc))
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestDeleteDoc() {
	t := s.T()

	// Try deleting documents before they are created
	s.DeleteTestDocs(t, s.jdb)

	// Create documents to be deleted
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Read + verify documents
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Delete all documents (1st time)
	fmtVersion = 0
	docVersion = 0
	s.DeleteTestDocs(t, s.jdb)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Delete all documents (2nd time) => should be no error
	s.DeleteTestDocs(t, s.jdb)
	s.VerifyTestDocs(t, s.jdb, fmtVersion, docVersion)
}

// deleteTestVec is a test vector (single test input case) for DeleteDoc()
// testing.
type deleteTestVec struct {
	errorDesc string
	key       JdocKey
}

func (s *JdocsDBSuite) TestDeleteDocErrors() {
	t := s.T()

	// Create documents to be deleted
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	errorVectors := []deleteTestVec{
		deleteTestVec{
			"Invalid DocType",
			JdocKey{TableName: testTableAcct, Account: "a1", Thing: "", DocType: "DocTypeShouldCauseError", DocName: makeTestDocName(DocTypeAccount, 1)},
		},
		deleteTestVec{
			"Invalid TableName",
			JdocKey{TableName: DocTypeAccount + "-bad-table", Account: "a1", Thing: "", DocType: DocTypeAccount, DocName: makeTestDocName(DocTypeAccount, 1)},
		},
	}
	for i, vec := range errorVectors {
		derr := s.jdb.DeleteDoc(vec.key)
		if derr == nil {
			assert.Fail(t, fmt.Sprintf("Test vector [%d] did not cause an error. errorDesc=%s: ", i, vec.errorDesc))
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}

func (s *JdocsDBSuite) TestSearch() {
	t := s.T()

	// Create documents to search
	fmtVersion := uint64(2)
	docVersion := uint64(1)
	s.CreateTestDocs(t, s.jdb, fmtVersion, docVersion)

	// Search on an Account-associated doc will return either 0 or 1 document
	for d := 0; d < (numDocsPerType + 1); d++ {
		for a := 0; a < numAccounts; a++ {
			fKey := *makeTestJdocKey(testTableAcct, makeTestDocName(DocTypeAccount, d), a+1, 0)
			resultKeys, err := s.jdb.FindDocsAssociatedWithAccount(fKey)
			assert.Nil(t, err)
			if d == 0 {
				assert.Equal(t, 0, len(resultKeys))
			} else {
				assert.Equal(t, 1, len(resultKeys))
				if len(resultKeys) > 0 { // avoid panic
					assert.Equal(t, fKey, resultKeys[0])
				}
			}
		}
	}

	// Search on an AccountThing-associated doc can return >1 document
	for d := 0; d < (numDocsPerType + 1); d++ {
		for a := 0; a < numAccounts; a++ {
			fKey := *makeTestJdocKey(testTableAcctThng, makeTestDocName(DocTypeAccountThing, d), a+1, 0)
			resultKeys, err := s.jdb.FindDocsAssociatedWithAccount(fKey)
			assert.Nil(t, err)
			if d == 0 {
				assert.Equal(t, 0, len(resultKeys))
			} else {
				assert.Equal(t, numThings, len(resultKeys))
				// the order of results is not guaranteed, so use a map to help confirm
				// that every expected key was returned
				gotKeyCount := make(map[JdocKey]int)
				for t := 0; t < numThings; t++ {
					k := *makeTestJdocKey(testTableAcctThng, makeTestDocName(DocTypeAccountThing, d), a+1, t+1)
					gotKeyCount[k] = -1 // expected
				}
				for _, key := range resultKeys {
					gotKeyCount[key]++
				}
				for key, count := range gotKeyCount {
					if count != 0 {
						assert.Fail(t, "Key %v occurred wrong number of times; exp=%d, got=%d", key, 1, count+1)
					}
				}
			}
		}
	}

	// DB cleanup: Delete all documents
	s.DeleteTestDocs(t, s.jdb)
}
