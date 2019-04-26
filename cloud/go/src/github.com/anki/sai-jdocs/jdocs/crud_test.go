package jdocs

import (
	"encoding/json"
	"fmt"
	"strings"
	"testing"

	"github.com/anki/sai-go-util/dockerutil"
	"github.com/anki/sai-jdocs/jdocs/db"   // package jdocsdb
	"github.com/anki/sai-jdocs/jdocs/err"  // package jdocserr
	"github.com/anki/sai-jdocs/jdocs/test" // package jdocstest
	"github.com/anki/sai-jdocs/proto/jdocspb"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"
)

//////////////////////////////////////////////////////////////////////

const (
	// crudTestNumEntities controls the Account/Thing permutations, which affects
	// how many documents will be in play for testing.
	//
	// For example, with crudTestNumEntities=2:
	//   Each DocName with DocType=Acct      will have a doc for Acct      = a1, a2
	//   Each DocName with DocType=AcctThing will have a doc for Acct/Thng = a1/t1, a1/t2, a2/t1, a2/t2
	//   Each DocName with DocType=Thng      will have a doc for Acct      = t1, t2
	//
	//   If there are two different documents of each DocType, then in total this
	//   would work out to numDocs = 2*(2+4+2) = 16 documents.
	crudTestNumEntities    = 2
	crudTestNumDataStrings = 4
	crudTestClientType     = ClientTypeUnknown
)

// NOTE: DocName values are carefully chosen to make writing tests easier. In
// particular, DocNames are '.'-separated strings, with the middle string being
// an exact string match for the DocType value.
const testJsonSpecList = `{
  "Specs": [
    {
      "DocName":     "test.Acct.0",
      "TableName":   "Acct-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    },
    {
      "DocName":     "test.Acct.1",
      "TableName":   "Acct-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    },
    {
      "DocName":     "test.Thng.0",
      "TableName":   "Thng-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    },
    {
      "DocName":     "test.Thng.1",
      "TableName":   "Thng-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    },
    {
      "DocName":     "test.AcctThng.0",
      "TableName":   "AcctThng-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    },
    {
      "DocName":     "test.AcctThng.1",
      "TableName":   "AcctThng-test",
      "HasGDPRData": false,
      "AllowCreate": ["ClientTypeUnknown"],
      "AllowRead":   ["ClientTypeUnknown"],
      "AllowUpdate": ["ClientTypeUnknown"],
      "AllowDelete": ["ClientTypeUnknown"]
    }
  ]
}`

// Example: "test.AcctThng.1" --> "AcctThng"
func DocTypeFromDocName(docName string) string {
	fields := strings.Split(docName, ".")
	return fields[1]
}

//////////////////////////////////////////////////////////////////////

type JdocsAPISuite struct {
	t            *testing.T
	dynamoSuite  dockerutil.DynamoSuite
	jdocSpecList *JdocSpecList
	api          *API
}

func (s *JdocsAPISuite) SetT(t *testing.T) {
	s.t = t
}

func (s *JdocsAPISuite) T() *testing.T {
	return s.t
}

// SetupSuite() is called once, prior to any test cases being run.
func (s *JdocsAPISuite) SetupSuite() {
	s.dynamoSuite.SetupSuite()
	tablePrefix := "unstable"

	if jerr := json.Unmarshal([]byte(testJsonSpecList), &s.jdocSpecList); jerr != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", jerr))
	}

	// create test harness database tables
	tables := make(map[string]bool)
	for _, spec := range s.jdocSpecList.Specs {
		tables[spec.TableName] = true
	}
	if jdb, err := jdocsdb.NewJdocsDB(&tables, tablePrefix, s.dynamoSuite.Config); err != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", err))
	} else {
		err = jdb.CreateTables()
		if err != nil {
			assert.FailNow(s.T(), fmt.Sprintf("%v", err))
		}
	}

	s.api = NewAPI()
	err := s.api.Init(testJsonSpecList, tablePrefix, s.dynamoSuite.Config)
	assert.Nil(s.T(), err)
}

func (s *JdocsAPISuite) TearDownSuite() {
	s.dynamoSuite.TearDownSuite()
}

// Because "go test" doesn't know anything about the testify/suite
// package, we need one file-level test case that instantiates and
// runs the suite.
func TestJdocsAPISuite(t *testing.T) {
	suite.Run(t, new(JdocsAPISuite))
}

//////////////////////////////////////////////////////////////////////

// WriteDoc creates a document on a DocKey, and then writes that document. Can
// be used for create or update. Success is expected.
func (s *JdocsAPISuite) WriteDocExpectSuccess(key jdocstest.DocKey) error {
	t := s.T()

	// XXX: Make the document to write, making sure to factor in auto-incr of
	// DocVersion by the service
	doc, err := jdocstest.NewJdoc(key)
	doc.DocVersion--
	if err != nil {
		return err
	}
	wresp, werr := s.api.WriteDoc(crudTestClientType, &jdocspb.WriteDocReq{
		Account: key.Account,
		Thing:   key.Thing,
		DocName: key.DocName,
		Doc:     doc,
	})
	assert.Nil(t, werr)
	assert.Equal(t, jdocspb.WriteDocResp_ACCEPTED, wresp.Status)
	assert.Equal(t, key.DocVersion, wresp.LatestDocVersion)
	return nil
}

// VerifyReadDocsRespItem verifies that a ReadDocsResp_Item matches an expected
// value, item by item.
func (s *JdocsAPISuite) VerifyReadDocsRespItem(exp, got *jdocspb.ReadDocsResp_Item) {
	t := s.T()

	assert.Equal(t, exp.Status, got.Status)
	assert.Equal(t, exp.Doc.DocVersion, got.Doc.DocVersion)
	assert.Equal(t, exp.Doc.FmtVersion, got.Doc.FmtVersion)
	assert.Equal(t, exp.Doc.ClientMetadata, got.Doc.ClientMetadata)
	assert.Equal(t, exp.Doc.JsonDoc, got.Doc.JsonDoc)
	// TODO: JsonDoc equal needs to be more than string ==
}

// ReadAndVerifyDoc reads a single document and verifies that it matches
// expected value. (The key is enough to determine all the document contents.
// Note that key.DocVersion==0 is a special case for a non-existent document.
func (s *JdocsAPISuite) ReadAndVerifyDoc(crudTestClientType ClientType, key jdocstest.DocKey) {
	t := s.T()

	rresp, rerr := s.api.ReadDocs(crudTestClientType, &jdocspb.ReadDocsReq{
		Account: key.Account,
		Thing:   key.Thing,
		Items: []*jdocspb.ReadDocsReq_Item{
			&jdocspb.ReadDocsReq_Item{
				DocName:      key.DocName,
				MyDocVersion: 0,
			},
		},
	})
	assert.Nil(t, rerr)
	assert.Equal(t, 1, len(rresp.Items))

	expDoc, derr := jdocstest.NewJdoc(key)
	assert.Nil(t, derr)
	if (derr == nil) && len(rresp.Items) > 0 {
		expItem := &jdocspb.ReadDocsResp_Item{
			Status: jdocspb.ReadDocsResp_CHANGED,
			Doc: &jdocspb.Jdoc{
				DocVersion:     expDoc.DocVersion,
				FmtVersion:     expDoc.FmtVersion,
				ClientMetadata: expDoc.ClientMetadata,
				JsonDoc:        expDoc.JsonDoc,
			},
		}
		if expDoc.DocVersion == 0 {
			expItem.Status = jdocspb.ReadDocsResp_NOT_FOUND
		}
		s.VerifyReadDocsRespItem(expItem, rresp.Items[0])
	}
}

//////////////////////////////////////////////////////////////////////

// ActOnDocSet iterates through a set of Accounts a[1]..a[numEntities] and a set
// of Things t[1]..t[numEntities], then performs some action on all unique
// permutations of Account/Thing/AccountThing documents for those sets of
// Accounts and Things.
//
// defaultKey fields that are copied unconditionally to the key for every
// document: DocVersion, FmtVersion, NumDataStrings.
//
// defaultKey field Account is copied to the key for Thing documents.
//
// defaultKey field Thing is copied to the key for Account documents.
func ActOnDocSet(docSpecs *JdocSpecList, numEntities int, defaultKey jdocstest.DocKey, actFn func(jdocstest.DocKey) error) error {
	for _, docSpec := range docSpecs.Specs {
		key := jdocstest.DocKey{
			DocName:        docSpec.DocName,
			DocType:        jdocsdb.TableDocType(docSpec.TableName),
			DocVersion:     defaultKey.DocVersion,
			FmtVersion:     defaultKey.FmtVersion,
			NumDataStrings: defaultKey.NumDataStrings,
		}
		docType := jdocsdb.TableDocType(docSpec.TableName)
		switch docType {
		case jdocsdb.DocTypeAccount:
			for acct := 0; acct < numEntities; acct++ {
				key.Account = jdocstest.AccountName(acct + 1)
				key.Thing = defaultKey.Thing
				if err := actFn(key); err != nil {
					return err
				}
			}
		case jdocsdb.DocTypeThing:
			for thng := 0; thng < numEntities; thng++ {
				key.Account = defaultKey.Account
				key.Thing = jdocstest.ThingName(thng + 1)
				if err := actFn(key); err != nil {
					return err
				}
			}
		case jdocsdb.DocTypeAccountThing:
			for acct := 0; acct < numEntities; acct++ {
				for thng := 0; thng < numEntities; thng++ {
					key.Account = jdocstest.AccountName(acct + 1)
					key.Thing = jdocstest.ThingName(thng + 1)
					if err := actFn(key); err != nil {
						return err
					}
				}
			}
		}
	}
	return nil
}

// WriteDocSet is used to write a new version for all documents. Can be used to
// create or update. Success is expected.
func (s *JdocsAPISuite) WriteDocSet(docVersion, fmtVersion uint64) {
	t := s.T()

	err := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
		jdocstest.DocKey{ // defaultKey
			Account:        "",
			Thing:          "",
			DocVersion:     docVersion,
			FmtVersion:     fmtVersion,
			NumDataStrings: crudTestNumDataStrings,
		},
		func(key jdocstest.DocKey) error {
			return s.WriteDocExpectSuccess(key)
		})
	assert.Nil(t, err)
}

func (s *JdocsAPISuite) DeleteDocSet() {
	t := s.T()

	err := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
		jdocstest.DocKey{ // defaultKey
			Account: "",
			Thing:   "",
		},
		func(myKey jdocstest.DocKey) error {
			_, derr := s.api.DeleteDoc(crudTestClientType, &jdocspb.DeleteDocReq{
				Account: myKey.Account,
				Thing:   myKey.Thing,
				DocName: myKey.DocName,
			})
			assert.Nil(t, derr)
			return nil
		})
	assert.Nil(t, err)
}

//////////////////////////////////////////////////////////////////////

type writeDocTestVector struct {
	DocVersion          uint64
	FmtVersion          uint64
	ExpStatus           jdocspb.WriteDocResp_Status
	ExpLatestDocVersion uint64
	ExpLatestFmtVersion uint64
}

func (s *JdocsAPISuite) TestCreateDocAndDeleteDoc() {
	t := s.T()

	// Create test cases that should fail (soft errors)
	createSoftErrVectors := []writeDocTestVector{
		//                                                                      latest
		//                 dv fv expStatus                                      dv fv
		writeDocTestVector{0, 0, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, 0, 0},
		writeDocTestVector{1, 0, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 0, 0},
		writeDocTestVector{1, 1, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 0, 0},
		writeDocTestVector{2, 1, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 0, 0},
	}
	for _, vec := range createSoftErrVectors {
		defaultKey := jdocstest.DocKey{
			Account:        "ignoredAcct",
			Thing:          "ignoredThng",
			DocVersion:     vec.DocVersion,
			FmtVersion:     vec.FmtVersion,
			NumDataStrings: crudTestNumDataStrings,
		}
		actErr := ActOnDocSet(s.jdocSpecList, crudTestNumEntities, defaultKey,
			func(myKey jdocstest.DocKey) error {
				// create attempt
				doc, err := jdocstest.NewJdoc(myKey)
				if err != nil {
					return err
				}
				wresp, werr := s.api.WriteDoc(crudTestClientType, &jdocspb.WriteDocReq{
					Account: myKey.Account,
					Thing:   myKey.Thing,
					DocName: myKey.DocName,
					Doc:     doc,
				})
				assert.Nil(t, werr)
				assert.Equal(t, vec.ExpStatus, wresp.Status)
				assert.Equal(t, vec.ExpLatestDocVersion, wresp.LatestDocVersion)

				// verify document does not exist
				s.ReadAndVerifyDoc(crudTestClientType,
					jdocstest.DocKey{
						Account:        myKey.Account,
						Thing:          myKey.Thing,
						DocName:        myKey.DocName,
						DocVersion:     vec.ExpLatestDocVersion,
						FmtVersion:     vec.ExpLatestFmtVersion,
						NumDataStrings: crudTestNumDataStrings, // ignored
					})
				return nil
			})
		assert.Nil(t, actErr)
	}

	// Successful create
	s.WriteDocSet(1 /*docVersion*/, 1 /*fmtVersion*/)

	// Verify created documents, using different garbage values for any unused
	// Account or Thing fields
	vActErr := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
		jdocstest.DocKey{ // defaultKey
			Account:        "ignoredAcct",
			Thing:          "ignoredThng",
			DocVersion:     1,
			FmtVersion:     1,
			NumDataStrings: crudTestNumDataStrings,
		},
		func(myKey jdocstest.DocKey) error {
			s.ReadAndVerifyDoc(crudTestClientType, myKey)
			return nil
		})
	assert.Nil(t, vActErr)

	// Delete and verify
	dActErr := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
		jdocstest.DocKey{ // defaultKey
			Account: "ignoredAcct",
			Thing:   "ignoredThng",
		},
		func(myKey jdocstest.DocKey) error {
			// delete each doc multiple times => should be no error
			for i := 0; i < 3; i++ {
				_, derr := s.api.DeleteDoc(crudTestClientType, &jdocspb.DeleteDocReq{
					Account: myKey.Account,
					Thing:   myKey.Thing,
					DocName: myKey.DocName,
				})
				assert.Nil(t, derr)

				// verify doc is deleted
				myKey.DocVersion = 0
				s.ReadAndVerifyDoc(crudTestClientType, myKey)
			}
			return nil
		})
	assert.Nil(t, dActErr)
}

//////////////////////////////////////////////////////////////////////

func (s *JdocsAPISuite) TestUpdateDoc() {
	t := s.T()

	s.WriteDocSet(1 /*docVersion*/, 2 /*fmtVersion*/)

	// Update test cases (mix of rejected and accepted)
	writeVectors := []writeDocTestVector{
		// NOTE: In this table, DocVersion = desired, after write
		//                 attempted                                            latest (actual)
		//                 dv fv    expStatus                                   dv fv
		writeDocTestVector{2, 0, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, 1, 2},
		writeDocTestVector{2, 1, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, 1, 2},
		writeDocTestVector{1, 2, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 1, 2},
		writeDocTestVector{3, 2, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 1, 2},
		writeDocTestVector{4, 2, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 1, 2},
		writeDocTestVector{2, 2, jdocspb.WriteDocResp_ACCEPTED /*************/, 2, 2},
		writeDocTestVector{2, 2, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, 2, 2},
		writeDocTestVector{3, 3, jdocspb.WriteDocResp_ACCEPTED /*************/, 3, 3},
		writeDocTestVector{4, 3, jdocspb.WriteDocResp_ACCEPTED /*************/, 4, 3},
		writeDocTestVector{5, 3, jdocspb.WriteDocResp_ACCEPTED /*************/, 5, 3},
		writeDocTestVector{6, 2, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, 5, 3},
		writeDocTestVector{6, 0, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, 5, 3},
		writeDocTestVector{6, 3, jdocspb.WriteDocResp_ACCEPTED /*************/, 6, 3},
	}
	for _, vec := range writeVectors {
		actErr := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
			jdocstest.DocKey{ // defaultKey
				Account:        "",
				Thing:          "",
				DocVersion:     vec.DocVersion,
				FmtVersion:     vec.FmtVersion,
				NumDataStrings: crudTestNumDataStrings,
			},
			func(myKey jdocstest.DocKey) error {
				// write attempt
				doc, err := jdocstest.NewJdoc(myKey)
				doc.DocVersion-- // XXX
				if err != nil {
					return err
				}
				wresp, werr := s.api.WriteDoc(crudTestClientType, &jdocspb.WriteDocReq{
					Account: myKey.Account,
					Thing:   myKey.Thing,
					DocName: myKey.DocName,
					Doc:     doc,
				})
				assert.Nil(t, werr)
				assert.Equal(t, vec.ExpStatus, wresp.Status)
				assert.Equal(t, vec.ExpLatestDocVersion, wresp.LatestDocVersion)

				// verify document
				delKey := myKey
				delKey.DocVersion = vec.ExpLatestDocVersion
				delKey.FmtVersion = vec.ExpLatestFmtVersion
				s.ReadAndVerifyDoc(crudTestClientType, delKey)
				return nil
			})
		assert.Nil(t, actErr)
	}

	s.DeleteDocSet() // cleanup
}

//////////////////////////////////////////////////////////////////////

func (s *JdocsAPISuite) TestReadDocs() {
	t := s.T()

	// Read valid doc names that do not yet exist
	rneErr := ActOnDocSet(s.jdocSpecList, crudTestNumEntities,
		jdocstest.DocKey{ // defaultKey
			Account:    "ignoredAcct",
			Thing:      "ignoredThng",
			DocVersion: 0,
			FmtVersion: 0,
		},
		func(myKey jdocstest.DocKey) error {
			// For non-existent docs, the response should be NOT_FOUND along with an
			// empty document. This case is handled correctly in ReadAndVerifyDoc().
			s.ReadAndVerifyDoc(crudTestClientType, myKey)
			return nil
		})
	assert.Nil(t, rneErr)

	// Create the set of documents to read
	s.WriteDocSet(1 /*docVersion*/, 1 /*fmtVersion*/)

	// Read request with zero items
	rresp, rerr := s.api.ReadDocs(crudTestClientType, &jdocspb.ReadDocsReq{
		Account: jdocstest.AccountName(1),
		Thing:   jdocstest.ThingName(1),
		Items:   []*jdocspb.ReadDocsReq_Item{},
	})
	assert.Nil(t, rerr)
	assert.Equal(t, 0, len(rresp.Items))

	// Read all documents in the Account table for one account (single table)
	rreq := jdocspb.ReadDocsReq{
		Account: jdocstest.AccountName(1),
		Thing:   jdocstest.ThingName(1),
		Items:   make([]*jdocspb.ReadDocsReq_Item, 0),
	}
	for _, docSpec := range s.jdocSpecList.Specs {
		docType := jdocsdb.TableDocType(docSpec.TableName)
		if docType == jdocsdb.DocTypeAccount {
			rreq.Items = append(rreq.Items, &jdocspb.ReadDocsReq_Item{
				DocName:      docSpec.DocName,
				MyDocVersion: 0, // read latest
			})
		}
	}
	rresp, rerr = s.api.ReadDocs(crudTestClientType, &rreq)
	assert.Nil(t, rerr)
	assert.Equal(t, len(rreq.Items), len(rresp.Items))
	for i := 0; i < len(rresp.Items); i++ {
		expDoc, derr := jdocstest.NewJdoc(jdocstest.DocKey{
			Account:        rreq.Account,
			Thing:          rreq.Thing,
			DocName:        rreq.Items[i].DocName,
			DocType:        jdocsdb.DocTypeAccount,
			DocVersion:     1,
			FmtVersion:     1,
			NumDataStrings: crudTestNumDataStrings,
		})
		assert.Nil(t, derr)
		expItem := &jdocspb.ReadDocsResp_Item{
			Status: jdocspb.ReadDocsResp_CHANGED,
			Doc: &jdocspb.Jdoc{
				DocVersion:     expDoc.DocVersion,
				FmtVersion:     expDoc.FmtVersion,
				ClientMetadata: expDoc.ClientMetadata,
				JsonDoc:        expDoc.JsonDoc,
			},
		}
		s.VerifyReadDocsRespItem(expItem, rresp.Items[i])
	}

	// Update the set of documents
	s.WriteDocSet(2 /*docVersion*/, 2 /*fmtVersion*/)

	// Read all documents for one Account/Thing (multiple tables)
	rreq = jdocspb.ReadDocsReq{
		Account: jdocstest.AccountName(1),
		Thing:   jdocstest.ThingName(1),
		Items:   make([]*jdocspb.ReadDocsReq_Item, 0),
	}
	expRespItems := make([]*jdocspb.ReadDocsResp_Item, 0)
	for _, docSpec := range s.jdocSpecList.Specs {
		docType := jdocsdb.TableDocType(docSpec.TableName)
		rreq.Items = append(rreq.Items, &jdocspb.ReadDocsReq_Item{
			DocName:      docSpec.DocName,
			MyDocVersion: 0, // read latest
		})
		expDoc, derr := jdocstest.NewJdoc(jdocstest.DocKey{
			Account:        rreq.Account,
			Thing:          rreq.Thing,
			DocName:        docSpec.DocName,
			DocType:        docType,
			DocVersion:     2,
			FmtVersion:     2,
			NumDataStrings: crudTestNumDataStrings,
		})
		assert.Nil(t, derr)
		expRespItems = append(expRespItems, &jdocspb.ReadDocsResp_Item{
			Status: jdocspb.ReadDocsResp_CHANGED,
			Doc: &jdocspb.Jdoc{
				DocVersion:     expDoc.DocVersion,
				FmtVersion:     expDoc.FmtVersion,
				ClientMetadata: expDoc.ClientMetadata,
				JsonDoc:        expDoc.JsonDoc,
			},
		})
	}
	rresp, rerr = s.api.ReadDocs(crudTestClientType, &rreq)
	assert.Nil(t, rerr)
	assert.Equal(t, len(rreq.Items), len(rresp.Items))
	for i := 0; i < len(rresp.Items); i++ {
		s.VerifyReadDocsRespItem(expRespItems[i], rresp.Items[i])
	}

	// Cleanup
	s.DeleteDocSet()
}

//////////////////////////////////////////////////////////////////////

type readTestVector struct {
	DocName       string
	MyDocVersion  uint64
	ExpStatus     jdocspb.ReadDocsResp_Status
	ExpDocVersion uint64
}

func (s *JdocsAPISuite) TestReadDocs2() {
	t := s.T()

	// Create the set of documents to read
	//
	// There are enough different document names in testJsonSpecList to cover all
	// of the interesting test cases with a single ReadDocs call.
	docList := []string{"test.Acct.0", "test.Acct.1", "test.Thng.0", "test.Thng.1"}
	acct := jdocstest.AccountName(1)
	thng := jdocstest.ThingName(1)
	fmtVersion := uint64(1)

	// Desired docs state:
	// - The two AcctThing docs do not exist
	// - The two Acct      docs at version 2
	// - The two Thng      docs at version 2
	for _, docName := range docList {
		key := jdocstest.DocKey{
			Account:        acct,
			Thing:          thng,
			DocName:        docName,
			DocType:        DocTypeFromDocName(docName),
			DocVersion:     1,
			FmtVersion:     fmtVersion,
			NumDataStrings: crudTestNumDataStrings,
		}
		s.WriteDocExpectSuccess(key)
		key.DocVersion++
		s.WriteDocExpectSuccess(key)
	}

	// Build up the ReadDocsReq and (expected) ReadDocsResp
	readItems := []readTestVector{
		//             DocName                 MyDocVr ExpStatus                            ExpDocVr
		readTestVector{"test.Acct.0" /******/, 0 /**/, jdocspb.ReadDocsResp_CHANGED /****/, 2},
		readTestVector{"test.Acct.1" /******/, 1 /**/, jdocspb.ReadDocsResp_CHANGED /****/, 2},
		readTestVector{"test.Acct.1" /******/, 2 /**/, jdocspb.ReadDocsResp_UNCHANGED /**/, 2},
		readTestVector{"test.AcctThng.0" /**/, 0 /**/, jdocspb.ReadDocsResp_NOT_FOUND /**/, 0},
		readTestVector{"test.AcctThng.1" /**/, 1 /**/, jdocspb.ReadDocsResp_NOT_FOUND /**/, 0},
		readTestVector{"test.AcctThng.0" /**/, 2 /**/, jdocspb.ReadDocsResp_NOT_FOUND /**/, 0},
		readTestVector{"test.AcctThng.1" /**/, 3 /**/, jdocspb.ReadDocsResp_NOT_FOUND /**/, 0},
		readTestVector{"test.Thng.0" /******/, 2 /**/, jdocspb.ReadDocsResp_UNCHANGED /**/, 2},
		readTestVector{"test.Thng.1" /******/, 3 /**/, jdocspb.ReadDocsResp_CHANGED /****/, 2},
		readTestVector{"test.Thng.0" /******/, 9 /**/, jdocspb.ReadDocsResp_CHANGED /****/, 2},
	}
	rreq := jdocspb.ReadDocsReq{
		Account: jdocstest.AccountName(1),
		Thing:   jdocstest.ThingName(1),
		Items:   make([]*jdocspb.ReadDocsReq_Item, len(readItems)),
	}
	expRespItems := make([]*jdocspb.ReadDocsResp_Item, len(readItems))
	for i, item := range readItems {
		rreq.Items[i] = &jdocspb.ReadDocsReq_Item{
			DocName:      item.DocName,
			MyDocVersion: item.MyDocVersion,
		}
		expDoc, derr := jdocstest.NewJdoc(jdocstest.DocKey{
			Account:        acct,
			Thing:          thng,
			DocName:        item.DocName,
			DocType:        DocTypeFromDocName(item.DocName),
			DocVersion:     item.ExpDocVersion,
			FmtVersion:     fmtVersion,
			NumDataStrings: crudTestNumDataStrings,
		})
		assert.Nil(t, derr)
		if item.ExpStatus == jdocspb.ReadDocsResp_UNCHANGED {
			expDoc.JsonDoc = ""
		}
		expRespItems[i] = &jdocspb.ReadDocsResp_Item{
			Status: item.ExpStatus,
			Doc: &jdocspb.Jdoc{
				DocVersion:     expDoc.DocVersion,
				FmtVersion:     expDoc.FmtVersion,
				ClientMetadata: expDoc.ClientMetadata,
				JsonDoc:        expDoc.JsonDoc,
			},
		}
	}

	// Read + verify
	rresp, rerr := s.api.ReadDocs(crudTestClientType, &rreq)
	assert.Nil(t, rerr)
	if len(rreq.Items) == len(rresp.Items) {
		for i := 0; i < len(rresp.Items); i++ {
			s.VerifyReadDocsRespItem(expRespItems[i], rresp.Items[i])
		}
	} else {
		assert.Failf(t, "ReadDocs resp length mismatch.", "Exp=%d, got=%d", len(rreq.Items), len(rresp.Items))
	}

	// Cleanup
	for _, docName := range docList {
		_, derr := s.api.DeleteDoc(crudTestClientType, &jdocspb.DeleteDocReq{
			Account: acct,
			Thing:   thng,
			DocName: docName,
		})
		assert.Nil(t, derr)
	}
}

//////////////////////////////////////////////////////////////////////

type errorTestVector struct {
	Account        string
	Thing          string
	DocName        string
	ClientMetadata string
	JsonDoc        string
	ExpStatus      codes.Code
	ExpAPICode     string
}

func (s *JdocsAPISuite) TestWriteErrors() {
	t := s.T()

	goodAcct := jdocstest.AccountName(1)
	goodThng := jdocstest.ThingName(1)
	goodDocName := "test.AcctThng.0"

	testVectors := []errorTestVector{
		// TODO: Test case for JSON depth exceeds max
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{", // mal-formed
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadJsonCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "", // empty
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadJsonCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        "",
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadDocNameCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName + "_",
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadDocNameCode,
		},
		errorTestVector{
			Account:        "",
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadAccountCode,
		},
		errorTestVector{
			Account:        strings.Repeat("a", MaxAccountLen+1),
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadAccountCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          "",
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadThingCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          strings.Repeat("t", MaxThingLen+1),
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.BadThingCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: strings.Repeat("d", MaxClientMetadataLen+1),
			JsonDoc:        "{ }",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.ClientMetadataTooBigCode,
		},
		errorTestVector{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName,
			ClientMetadata: "",
			JsonDoc:        "{ \"VeryLongField\": \"" + strings.Repeat("f", MaxJsonDocLen) + "\"}",
			ExpStatus:      codes.InvalidArgument,
			ExpAPICode:     jdocserr.JsonDocTooBigCode,
		},
	}

	// Try all error vectors two times:
	//   1. Against a document that does not exist
	//   2. Against a document that does     exist
	for docVersion := uint64(1); docVersion <= 2; docVersion++ {
		for i, vec := range testVectors {
			_, werr := s.api.WriteDoc(crudTestClientType, &jdocspb.WriteDocReq{
				Account: vec.Account,
				Thing:   vec.Thing,
				DocName: vec.DocName,
				Doc: &jdocspb.Jdoc{
					DocVersion:     docVersion,
					FmtVersion:     1,
					ClientMetadata: vec.ClientMetadata,
					JsonDoc:        vec.JsonDoc,
				},
			})
			if werr == nil {
				t.Errorf("For test vector [%d], expected WriteDoc error, got nil", i)
			} else {
				// cast to jdocserr.Error type, to check specific fields
				statusCode, apiCode := jdocserr.CodesFromError(werr)
				assert.Equal(t, vec.ExpStatus, statusCode)
				assert.Equal(t, vec.ExpAPICode, apiCode)
			}
		}

		// Create or Update the document, for next iteration of the loop
		key := jdocstest.DocKey{
			Account:        goodAcct,
			Thing:          goodThng,
			DocName:        goodDocName,
			DocType:        DocTypeFromDocName(goodDocName),
			DocVersion:     docVersion,
			FmtVersion:     1,
			NumDataStrings: crudTestNumDataStrings,
		}
		s.WriteDocExpectSuccess(key)
	}

	// Cleanup
	_, derr := s.api.DeleteDoc(crudTestClientType, &jdocspb.DeleteDocReq{
		Account: goodAcct,
		Thing:   goodThng,
		DocName: goodDocName,
	})
	assert.Nil(t, derr)
}
