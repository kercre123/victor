package jdocssvc

import (
	"context"
	"encoding/json"
	"fmt"
	"testing"

	"github.com/anki/sai-go-util/dockerutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/jdocs"
	"github.com/anki/sai-jdocs/jdocs/db" // package jdocsdb
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/test"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
)

//////////////////////////////////////////////////////////////////////

const testJsonSpecList = `{
  "Specs": [
    {
      "DocName":        "vic.Entitlements",
      "TableName":      "Acct-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    ["ClientTypeUnknown"],
      "AllowRead":      ["ClientTypeUnknown"],
      "AllowUpdate":    ["ClientTypeUnknown"],
      "AllowDelete":    ["ClientTypeUnknown"]
    },
    {
      "DocName":        "vic.AppTokens",
      "TableName":      "AcctThng-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    ["ClientTypeUnknown"],
      "AllowRead":      ["ClientTypeUnknown"],
      "AllowUpdate":    ["ClientTypeUnknown"],
      "AllowDelete":    ["ClientTypeUnknown"]
    },
    {
      "DocName":        "vic.UserSettings",
      "TableName":      "Acct-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    ["ClientTypeUnknown", "ClientTypeTokenService"],
      "AllowRead":      ["ClientTypeUnknown", "ClientTypeTokenService"],
      "AllowUpdate":    ["ClientTypeUnknown", "ClientTypeTokenService", "ClientTypeVicRobot"],
      "AllowDelete":    ["ClientTypeUnknown"]
    },
    {
      "DocName":        "vic.RobotSettings",
      "TableName":      "AcctThng-vic",
      "HasGDPRData":    true,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    ["ClientTypeUnknown", "ClientTypeTokenService"],
      "AllowRead":      ["ClientTypeUnknown", "ClientTypeTokenService", "ClientTypeChipperTimerService", "ClientTypeChipperWeatherService"],
      "AllowUpdate":    ["ClientTypeUnknown", "ClientTypeTokenService", "ClientTypeVicRobot"],
      "AllowDelete":    ["ClientTypeUnknown"]
    },
    {
      "DocName":        "vic.RobotOwners",
      "TableName":      "Thng-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    ["ClientTypeUnknown"],
      "AllowRead":      ["ClientTypeUnknown"],
      "AllowUpdate":    ["ClientTypeUnknown"],
      "AllowDelete":    ["ClientTypeUnknown"]
    },
    {
      "DocName":        "vic.HardwareStats",
      "TableName":      "Thng-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 2048,
      "AllowCreate":    ["ClientTypeUnknown"],
      "AllowRead":      ["ClientTypeUnknown"],
      "AllowUpdate":    ["ClientTypeUnknown"],
      "AllowDelete":    ["ClientTypeUnknown"]
    }
  ]
}`

//////////////////////////////////////////////////////////////////////

// TestJdoc is a simple document type where the fields match the keys and
// metadata.
type TestJdoc struct {
	Account    string `json:"Account"`
	Thing      string `json:"Thing"`
	DocName    string `json:"DocName"`
	FmtVersion uint64 `json:"FmtVersion"`
	DocVersion uint64 `json:"DocVersion"`
}

func writeTestJdoc(client jdocspb.JdocsClient, doc TestJdoc) (*jdocspb.WriteDocResp, error) {
	clientMetadata := doc.Account + "_" + doc.Thing + "_" + doc.DocName
	doc.DocVersion++ // XXX: Successful write will cause DocVersion to increment
	jsonDoc, err := json.Marshal(doc)
	if err != nil {
		return &jdocspb.WriteDocResp{}, err
	}
	doc.DocVersion--
	wreq := &jdocspb.WriteDocReq{
		Account: doc.Account,
		Thing:   doc.Thing,
		DocName: doc.DocName,
		Doc: &jdocspb.Jdoc{
			DocVersion:     doc.DocVersion,
			FmtVersion:     doc.FmtVersion,
			ClientMetadata: clientMetadata,
			JsonDoc:        string(jsonDoc),
		},
	}
	return client.WriteDoc(context.Background(), wreq)
}

func verifyTestJdoc(t *testing.T, myDocVersion uint64, exp TestJdoc, rresp *jdocspb.ReadDocsResp_Item) {
	assert.Equal(t, exp.DocVersion, rresp.Doc.DocVersion)
	assert.Equal(t, exp.FmtVersion, rresp.Doc.FmtVersion)
	assert.Equal(t, exp.Account+"_"+exp.Thing+"_"+exp.DocName, rresp.Doc.ClientMetadata)

	if myDocVersion == exp.DocVersion {
		assert.Equal(t, jdocspb.ReadDocsResp_UNCHANGED, rresp.Status)
		assert.Equal(t, "", rresp.Doc.JsonDoc)
	} else {
		assert.Equal(t, jdocspb.ReadDocsResp_CHANGED, rresp.Status)
		var gotDoc TestJdoc
		//fmt.Printf("rresp.Doc.JsonDoc = %s\n", rresp.Doc.JsonDoc)
		if jerr := json.Unmarshal([]byte(rresp.Doc.JsonDoc), &gotDoc); jerr != nil {
			assert.Nil(t, jerr)
		}
		assert.Equal(t, exp, gotDoc)
	}
}

func readVerifyTestJdoc(t *testing.T, client jdocspb.JdocsClient, myDocVersion uint64, exp TestJdoc) {
	rreq := &jdocspb.ReadDocsReq{
		Account: exp.Account,
		Thing:   exp.Thing,
		Items: []*jdocspb.ReadDocsReq_Item{
			&jdocspb.ReadDocsReq_Item{
				DocName:      exp.DocName,
				MyDocVersion: myDocVersion,
			},
		},
	}
	rresp, err := client.ReadDocs(context.Background(), rreq)
	if err == nil {
		assert.Equal(t, 1, len(rresp.Items))
		if len(rresp.Items) > 0 {
			//fmt.Printf("Account=%s, Thing=%s, DocName=%s JsonDoc:\n%s\n", exp.Account, exp.Thing, exp.DocName, rresp.Items[0].Doc.JsonDoc)
			verifyTestJdoc(t, myDocVersion, exp, rresp.Items[0])
		}
	}
}

func verifyDocNotFound(t *testing.T, client jdocspb.JdocsClient, account, thing, docName string) {
	rreq := &jdocspb.ReadDocsReq{
		Account: account,
		Thing:   thing,
		Items: []*jdocspb.ReadDocsReq_Item{
			&jdocspb.ReadDocsReq_Item{
				DocName:      docName,
				MyDocVersion: 0,
			},
		},
	}
	rresp, err := client.ReadDocs(context.Background(), rreq)
	assert.Nil(t, err)
	if err == nil {
		assert.Equal(t, 1, len(rresp.Items))
		if len(rresp.Items) > 0 {
			assert.Equal(t, jdocspb.ReadDocsResp_NOT_FOUND, rresp.Items[0].Status)
			assert.Equal(t, uint64(0), rresp.Items[0].Doc.DocVersion)
			assert.Equal(t, uint64(0), rresp.Items[0].Doc.FmtVersion)
			assert.Equal(t, "", rresp.Items[0].Doc.ClientMetadata)
			assert.Equal(t, "", rresp.Items[0].Doc.JsonDoc)
		}
	}
}

func writeNewDocVersion(t *testing.T, client jdocspb.JdocsClient, newDoc TestJdoc) {
	wresp, werr := writeTestJdoc(client, newDoc)
	assert.Nil(t, werr)
	if werr == nil {
		assert.Equal(t, jdocspb.WriteDocResp_ACCEPTED, wresp.Status)
		assert.Equal(t, uint64(newDoc.DocVersion+1), wresp.LatestDocVersion)
	}
}

func testBasicReadWriteFunctionality(t *testing.T, client jdocspb.JdocsClient, account, thing, docName string, numDocVersions uint64) {
	// At beginning of test, document should not exist
	verifyDocNotFound(t, client, account, thing, docName)

	// Write multiple versions of the document, then read + verify
	var dv uint64
	for dv = 0; dv < numDocVersions; dv++ {
		newDoc := TestJdoc{
			Account:    account,
			Thing:      thing,
			DocName:    docName,
			FmtVersion: dv + 1,
			DocVersion: dv,
		}
		writeNewDocVersion(t, client, newDoc)
		newDoc.DocVersion++
		readVerifyTestJdoc(t, client, 0, newDoc)
		readVerifyTestJdoc(t, client, 1, newDoc)
	}
	// Try to write documents with invalid DocVersion
	for _, delta := range []int{-1, 1} {
		newDoc := TestJdoc{
			Account:    account,
			Thing:      thing,
			DocName:    docName,
			FmtVersion: dv + 1,
			DocVersion: dv + uint64(delta),
		}
		wresp, werr := writeTestJdoc(client, newDoc)
		assert.Nil(t, werr)
		assert.Equal(t, jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, wresp.Status)
		assert.Equal(t, dv, wresp.LatestDocVersion)
	}

	// Try to write documents with invalid FmtVersion
	newDoc := TestJdoc{
		Account:    account,
		Thing:      thing,
		DocName:    docName,
		FmtVersion: numDocVersions - 1,
		DocVersion: dv,
	}
	wresp, werr := writeTestJdoc(client, newDoc)
	assert.Nil(t, werr)
	assert.Equal(t, jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION, wresp.Status)
	assert.Equal(t, dv, wresp.LatestDocVersion)
}

//////////////////////////////////////////////////////////////////////

type JdocsServerSuite struct {
	grpctest.Suite
	dynamoSuite  dockerutil.DynamoSuite
	jdocSpecList *jdocs.JdocSpecList
	jdb          *jdocsdb.JdocsDB
	jdocsServer  *Server
}

// SetupSuite() is called once, prior to any test cases being run. If
// the RegisterFn is set here, then a new gRPC service instance will
// be automatically started before each test case.
//
// Alternatively, if an individual test  case needs a different server
// setup (e.g. with different parameters),  then the test case can set
// RegisterFn and explicitly call s.Setup().
func (s *JdocsServerSuite) SetupSuite() {
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
	var err error
	s.jdb, err = jdocsdb.NewJdocsDB(&tables, tablePrefix, s.dynamoSuite.Config)
	if err != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", err))
	}
	err = s.jdb.CreateTables()
	if err != nil {
		assert.FailNow(s.T(), fmt.Sprintf("%v", err))
	}

	s.jdocsServer = NewServer()
	s.RegisterFn = func(gs *grpc.Server) {
		jdocspb.RegisterJdocsServer(gs, s.jdocsServer)
	}

	err = s.jdocsServer.InitAPI(testJsonSpecList, tablePrefix, s.dynamoSuite.Config)
	assert.Nil(s.T(), err)

	alog.SetLevelByString("WARN")
}

func (s *JdocsServerSuite) TearDownSuite() {
	s.dynamoSuite.TearDownSuite()
}

// Because "go test" doesn't know anything about the testify/suite
// package, we need one file-level test case that instantiates and
// runs the suite.
func TestJdocsServerSuite(t *testing.T) {
	suite.Run(t, new(JdocsServerSuite))
}

//////////////////////////////////////////////////////////////////////

func (s *JdocsServerSuite) TestBasic() {
	client := jdocspb.NewJdocsClient(s.Suite.ClientConn)
	numAccounts := 3
	numThings := 2
	numDocVersions := uint64(3)

	// Test basic read/write functionality on individual documents.
	for _, docSpec := range s.jdocSpecList.Specs {
		docType := jdocsdb.TableDocType(docSpec.TableName)
		switch docType {
		case jdocsdb.DocTypeAccount:
			for accountNum := 0; accountNum < numAccounts; accountNum++ {
				account := fmt.Sprintf("user%d", accountNum)
				thing := ""
				testBasicReadWriteFunctionality(s.T(), client, account, thing, docSpec.DocName, numDocVersions)
			}
		case jdocsdb.DocTypeThing:
			for thingNum := 0; thingNum < numThings; thingNum++ {
				account := ""
				thing := fmt.Sprintf("thing%d", thingNum)
				testBasicReadWriteFunctionality(s.T(), client, account, thing, docSpec.DocName, numDocVersions)
			}
		case jdocsdb.DocTypeAccountThing:
			for accountNum := 0; accountNum < numThings; accountNum++ {
				for thingNum := 0; thingNum < numThings; thingNum++ {
					account := fmt.Sprintf("user%d", accountNum)
					thing := fmt.Sprintf("thing%d", thingNum)
					testBasicReadWriteFunctionality(s.T(), client, account, thing, docSpec.DocName, numDocVersions)
				}
			}
		}
	}

	// Now, read groups of documents and make sure all documents exist as
	// expected, with distinct JSON document values.
	readItems := make([]*jdocspb.ReadDocsReq_Item, 0)
	for _, docSpec := range s.jdocSpecList.Specs {
		readItems = append(readItems, &jdocspb.ReadDocsReq_Item{
			DocName:      docSpec.DocName,
			MyDocVersion: 0,
		})
	}

	for accountNum := 0; accountNum < numThings; accountNum++ {
		for thingNum := 0; thingNum < numThings; thingNum++ {
			account := fmt.Sprintf("user%d", accountNum)
			thing := fmt.Sprintf("thing%d", thingNum)
			rreq := &jdocspb.ReadDocsReq{
				Account: account,
				Thing:   thing,
				Items:   readItems,
			}
			rresp, err := client.ReadDocs(context.Background(), rreq)
			if err == nil {
				assert.Equal(s.T(), len(rreq.Items), len(rresp.Items))
				for i, ritem := range rresp.Items {
					expDoc := TestJdoc{
						Account:    account,
						Thing:      thing,
						DocName:    rreq.Items[i].DocName,
						FmtVersion: numDocVersions,
						DocVersion: numDocVersions,
					}
					docType := jdocsdb.TableDocType(s.jdocSpecList.Specs[i].TableName)
					switch docType {
					case jdocsdb.DocTypeAccount:
						expDoc.Thing = ""
					case jdocsdb.DocTypeThing:
						expDoc.Account = ""
					}
					verifyTestJdoc(s.T(), rreq.Items[i].MyDocVersion, expDoc, ritem)
				}
			}
		}
	}
}
