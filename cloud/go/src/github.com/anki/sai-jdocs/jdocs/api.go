package jdocs

// api.go
//
// Implement the service-level API commands.

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/jdocs/db"  // package jdocsdb
	"github.com/anki/sai-jdocs/jdocs/err" // package jdocserr
	"github.com/anki/sai-jdocs/proto/jdocspb"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/dynamodb"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const (
	MaxAccountLen         = 64         // can be increased, if needed
	MaxThingLen           = 64         // can be increased, if needed
	MaxClientMetadataLen  = 32         // can be increased, if needed
	MaxJsonDocLen         = 200 * 1024 // 200KB (DynamoDB item size limit is 400KB)
	MasJsonDocDepthLevels = 16         // DynamoDB has a document depth limit of 32
)

//////////////////////////////////////////////////////////////////////

type API struct {
	jdocSpecs intJdocSpecSet
	jdb       *jdocsdb.JdocsDB
}

func NewAPI() *API {
	return &API{
		jdocSpecs: make(intJdocSpecSet),
		jdb:       nil,
	}
}

func (a *API) Init(docSpecsJson, tablePrefix string, cfg *aws.Config) error {
	var docSpecList JdocSpecList
	if jerr := json.Unmarshal([]byte(docSpecsJson), &docSpecList); jerr != nil {
		return fmt.Errorf("Cannot parse JSON for JDOCS document specs: %q", jerr)
	}
	if iSpecSet, err := docSpecList.toIntJdocSpecSet(); err != nil {
		return err
	} else {
		a.jdocSpecs = *iSpecSet
	}

	// Figure out the tables that are needed
	tables := make(map[string]bool)
	for _, spec := range a.jdocSpecs {
		tables[spec.TableName] = true
	}
	jdb, err := jdocsdb.NewJdocsDB(&tables, tablePrefix, cfg)
	a.jdb = jdb
	return err
}

func (a *API) CheckTables() error {
	return a.jdb.CheckTables()
}

//////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////

func jdocKeyString(k *jdocsdb.JdocKey) string {
	// for error messages (TableName is excluded)
	return fmt.Sprintf("Account=%s Thing=%s DocType=%s DocName=%s", k.Account, k.Thing, k.DocType, k.DocName)
}

func (a *API) makeJdocKey(account, thing, docName string) (*jdocsdb.JdocKey, error) {
	spec, ok := a.jdocSpecs[docName]
	if !ok {
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadDocNameCode, "DocName=%s not supported by service", docName)
	}
	key := jdocsdb.JdocKey{
		TableName: spec.TableName,
		Account:   account,
		Thing:     thing,
		DocType:   spec.DocType,
		DocName:   docName,
	}

	if l := len(account); l > MaxAccountLen {
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadAccountCode, "%s: Account is too long", jdocKeyString(&key))
	}
	if l := len(thing); l > MaxThingLen {
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadThingCode, "%s: Thing is too long", jdocKeyString(&key))
	}
	// TODO: Check that account,thing only have allowed characters?

	// make sure DocType is valid, and account/thing values are consistent with it
	switch spec.DocType {
	case jdocsdb.DocTypeAccount:
		if account == "" {
			return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadAccountCode, "%s: Account cannot be empty string", jdocKeyString(&key))
		}
	case jdocsdb.DocTypeThing:
		if thing == "" {
			return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadThingCode, "%s: Thing cannot be empty string", jdocKeyString(&key))
		}
	case jdocsdb.DocTypeAccountThing:
		if account == "" {
			return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadAccountCode, "%s: Account cannot be empty string", jdocKeyString(&key))
		}
		if thing == "" {
			return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadThingCode, "%s: Thing cannot be empty string", jdocKeyString(&key))
		}
	default:
		// DocType values are checked in toIntJdocSpecSet(), which is called by NewAPI()
		// => should NEVER get here
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadDocTypeCode, "%s", jdocKeyString(&key))
	}

	return &key, nil
}

//////////////////////////////////////////////////////////////////////

func (a *API) Echo(in *jdocspb.EchoReq) (*jdocspb.EchoResp, error) {
	return &jdocspb.EchoResp{Data: in.Data}, nil
}

func (a *API) WriteDoc(clientType ClientType, wreq *jdocspb.WriteDocReq) (*jdocspb.WriteDocResp, error) {
	// TODO: Authenticate

	// Enforce API limits
	if l := len(wreq.Doc.ClientMetadata); l > MaxClientMetadataLen {
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.ClientMetadataTooBigCode, "Len=%d exceeds Max=%d", l, MaxClientMetadataLen)
	}
	newDocLen := len(wreq.Doc.JsonDoc)
	if newDocLen > MaxJsonDocLen {
		return nil, jdocserr.Errorf(codes.InvalidArgument, jdocserr.JsonDocTooBigCode, "Len=%d exceeds Max=%d", newDocLen, MaxJsonDocLen)
	}

	key, kerr := a.makeJdocKey(wreq.Account, wreq.Thing, wreq.DocName)
	if kerr != nil {
		return nil, kerr
	}
	newDoc := jdocsdb.Jdoc{
		DocVersion:     wreq.Doc.DocVersion,
		FmtVersion:     wreq.Doc.FmtVersion,
		ClientMetadata: wreq.Doc.ClientMetadata,
		JsonDoc:        wreq.Doc.JsonDoc,
	}

	// Log a warning if client attempts to write a document that exceeds the max
	// length in the spec (regardless of whether the write is successful). This is
	// intended to detect client bugs and changes in usage patterns that don't fit
	// JDOCS use model well, and hence could leave to undesirable performance or
	// excessive AWS costs.
	if expLen := a.jdocSpecs[key.DocName].ExpectedMaxLen; newDocLen > expLen {
		alog.Warn{
			"action":           "write_doc",
			"status":           "json_doc_exceeds_expected_len",
			"spec_exp_max_len": expLen,
			"got_len":          newDocLen,
			"json_doc":         newDoc.JsonDoc, // Helpful, or not needed?
		}.Log()
	}

	var dberr error
	if newDoc.DocVersion == 0 {
		// Create attempt
		if !a.jdocSpecs[wreq.DocName].AllowCreate[clientType] {
			return nil, jdocserr.Errorf(codes.PermissionDenied, jdocserr.CreateDocNotAllowedCode, "ClientType=%s DocName=%s", clientType, wreq.DocName)
		}
		if newDoc.FmtVersion == 0 {
			return &jdocspb.WriteDocResp{
				Status:           jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION,
				LatestDocVersion: 0, // XXX
			}, nil // soft error
		}
		newDoc.DocVersion = 1
		dberr = a.jdb.CreateDoc(*key, newDoc)
		if dberr == nil {
			return &jdocspb.WriteDocResp{
				Status:           jdocspb.WriteDocResp_ACCEPTED,
				LatestDocVersion: newDoc.DocVersion,
			}, nil
		}
	} else {
		// Update attempt
		if !a.jdocSpecs[wreq.DocName].AllowUpdate[clientType] {
			return nil, jdocserr.Errorf(codes.PermissionDenied, jdocserr.UpdateDocNotAllowedCode, "ClientType=%s DocName=%s", clientType, wreq.DocName)
		}
		newDoc.DocVersion++
		dberr = a.jdb.UpdateDoc(*key, newDoc)
		if dberr == nil {
			return &jdocspb.WriteDocResp{
				Status:           jdocspb.WriteDocResp_ACCEPTED,
				LatestDocVersion: newDoc.DocVersion,
			}, nil
		}
	}

	// (dberr != nil) .. need to find out why and return a useful error code. This
	// requires reading the current version of the document (if it exists).
	if aerr, ok := dberr.(awserr.Error); ok && (aerr.Code() == dynamodb.ErrCodeConditionalCheckFailedException) {
		curDoc, rerr := a.jdb.ReadDoc(*key, true /*stronglyConsistent*/)
		if rerr != nil {
			return nil, jdocserr.Errorf(codes.Internal, jdocserr.DBReadErrorCode, "%s", rerr)
		}
		if newDoc.FmtVersion < curDoc.FmtVersion {
			// ConditionalCheckFailedException = FmtVersion match
			return &jdocspb.WriteDocResp{
				Status:           jdocspb.WriteDocResp_REJECTED_BAD_FMT_VERSION,
				LatestDocVersion: curDoc.DocVersion,
			}, nil // soft error
		} else {
			// ConditionalCheckFailedException = DocVersion match, or attribute_not_exists()
			return &jdocspb.WriteDocResp{
				Status:           jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION,
				LatestDocVersion: curDoc.DocVersion,
			}, nil // soft error
		}
	}
	if _, ok := status.FromError(dberr); ok {
		return nil, dberr
	}
	return nil, jdocserr.Errorf(codes.Internal, jdocserr.DBCreateErrorCode, "%s", dberr)
}

func (a *API) ReadDocs(clientType ClientType, rreq *jdocspb.ReadDocsReq) (*jdocspb.ReadDocsResp, error) {
	// TODO: Authenticate

	// Documents may be spread across multiple DB tables; need separate into
	// individual lists of documents from each table, for efficient reads.
	docsInTable := make(map[string]*jdocsdb.ReadDocsInput) // tableName --> set of docNames
	oneDocName := make(map[string]string)                  // tableName --> one docName in that table (XXX)
	for _, reqItem := range rreq.Items {
		// any validation error for account/thing/docName causes an error for the
		// whole read request
		key, kerr := a.makeJdocKey(rreq.Account, rreq.Thing, reqItem.DocName)
		if kerr != nil {
			return nil, kerr
		}
		if _, ok := docsInTable[key.TableName]; !ok {
			// don't try to cram items into an un-initialized map
			newSet := make(jdocsdb.ReadDocsInput)
			docsInTable[key.TableName] = &newSet
		}
		set := docsInTable[key.TableName]
		(*set)[reqItem.DocName] = true
		oneDocName[key.TableName] = reqItem.DocName
	}

	// Read the documents from each table
	allReadDocs := make(jdocsdb.ReadDocsOutput)
	for tableName, docSet := range docsInTable {
		// By design, all docs in the set are in the same table and have the same DocType
		key, _ := a.makeJdocKey(rreq.Account, rreq.Thing, oneDocName[tableName])
		if err := a.jdb.ReadDocs(*key, docSet, &allReadDocs); err != nil {
			return nil, jdocserr.Errorf(codes.Internal, jdocserr.DBReadErrorCode, "%s", err)
		}
	}

	// Populate the response (same order as request)
	resp := jdocspb.ReadDocsResp{
		Items: make([]*jdocspb.ReadDocsResp_Item, len(rreq.Items), len(rreq.Items)),
	}
	for i, reqItem := range rreq.Items {
		nd := allReadDocs[reqItem.DocName]
		newDoc := jdocspb.Jdoc{
			DocVersion:     nd.DocVersion,
			FmtVersion:     nd.FmtVersion,
			ClientMetadata: nd.ClientMetadata,
			JsonDoc:        nd.JsonDoc,
		}
		if !a.jdocSpecs[reqItem.DocName].AllowRead[clientType] {
			// Not authorized to read this document!
			resp.Items[i] = &jdocspb.ReadDocsResp_Item{
				Status: jdocspb.ReadDocsResp_PERMISSION_DENIED,
				Doc:    &jdocspb.Jdoc{},
			}
		} else if newDoc.DocVersion == 0 {
			resp.Items[i] = &jdocspb.ReadDocsResp_Item{
				Status: jdocspb.ReadDocsResp_NOT_FOUND,
				Doc:    &newDoc,
			}
		} else if newDoc.DocVersion == reqItem.MyDocVersion {
			resp.Items[i] = &jdocspb.ReadDocsResp_Item{
				Status: jdocspb.ReadDocsResp_UNCHANGED,
				Doc: &jdocspb.Jdoc{
					DocVersion:     newDoc.DocVersion,
					FmtVersion:     newDoc.FmtVersion,
					ClientMetadata: newDoc.ClientMetadata,
					JsonDoc:        "",
				},
			}
		} else {
			// newDoc is newer, or older, than reqItem.MyDocversion
			resp.Items[i] = &jdocspb.ReadDocsResp_Item{
				Status: jdocspb.ReadDocsResp_CHANGED,
				Doc:    &newDoc,
			}
		}
	}

	return &resp, nil
}

func (a *API) DeleteDoc(clientType ClientType, dreq *jdocspb.DeleteDocReq) (*jdocspb.DeleteDocResp, error) {
	// TODO: Authenticate

	key, kerr := a.makeJdocKey(dreq.Account, dreq.Thing, dreq.DocName)
	if kerr != nil {
		return nil, kerr
	}

	if !a.jdocSpecs[dreq.DocName].AllowDelete[clientType] {
		// Not authorized
		return nil, jdocserr.Errorf(codes.PermissionDenied, jdocserr.DeleteDocNotAllowedCode, "ClientType=%s DocName=%s", clientType, dreq.DocName)
	}

	derr := a.jdb.DeleteDoc(*key)
	if derr != nil {
		return nil, jdocserr.Errorf(codes.Internal, jdocserr.DBDeleteErrorCode, "%s", derr)
	}

	return &jdocspb.DeleteDocResp{}, nil
}

//////////////////////////////////////////////////////////////////////
// Future Service API Functions
//////////////////////////////////////////////////////////////////////

func (a *API) FindThingsAccountHasOwned(clientType ClientType, account, product string) ([]string, error) {
	// TODO: Authenticate
	// TODO: Authorize

	// Every time an account takes ownership of a robot, a new AppTokens document
	// gets associated with the Account+Thing.
	findKey, kerr := a.makeJdocKey(account, "IGNORED", product+".AppTokens")
	if kerr != nil {
		return []string{}, kerr
	}
	jdocKeyList, ferr := a.jdb.FindDocsAssociatedWithAccount(*findKey)
	if ferr != nil {
		return []string{}, jdocserr.Errorf(codes.Internal, jdocserr.DBFindErrorCode, "%s", ferr)
	}

	thingsOwned := make([]string, 0)
	for _, jdocKey := range jdocKeyList {
		thingsOwned = append(thingsOwned, jdocKey.Thing)
	}
	return thingsOwned, nil
}

func (a *API) FindThingsAccountOwnsNow(clientType ClientType, account, product string) ([]string, error) {
	thingsHasOwned, err := a.FindThingsAccountHasOwned(clientType, account, product)
	if err != nil {
		return []string{}, err
	}

	// Some of the Things may no longer be owned by this account
	thingsOwnedNow := make([]string, 0)
	ownersDocName := product + ".Owners"
	for _, thing := range thingsHasOwned {
		key, kerr := a.makeJdocKey("IGNORED", thing, ownersDocName)
		if kerr != nil {
			return []string{}, kerr
		}
		// For most accurate result, do a Strongly Consistent read. It is expected
		// this API function will not be called frequently, so the extra cost and
		// higher latency shouldn't matter.
		jdoc, err := a.jdb.ReadDoc(*key, true /*stronglyConsistent*/)
		if err != nil {
			return []string{}, jdocserr.Errorf(codes.Internal, jdocserr.DBReadErrorCode, "%s", err)
		}
		// TODO: Extract the current owner from JSON
		isOwnedNow := (jdoc.DocVersion > 1) // XXX: Fake logic to use jdoc variable
		if isOwnedNow {
			thingsOwnedNow = append(thingsOwnedNow, thing)
		}
	}

	return thingsOwnedNow, nil
}

func (a *API) FindOwnerOfThing(clientType ClientType, thing, product string) (string, error) {
	ownersDocName := product + ".Owners"
	key, kerr := a.makeJdocKey("IGNORED", thing, ownersDocName)
	if kerr != nil {
		return "", kerr
	}
	// For most accurate result, do a Strongly Consistent read. It is expected
	// this API function will not be called frequently, so the extra cost and
	// higher latency shouldn't matter.
	jdoc, err := a.jdb.ReadDoc(*key, true /*stronglyConsistent*/)
	if err != nil {
		return "", jdocserr.Errorf(codes.Internal, jdocserr.DBReadErrorCode, "%s", err)
	}
	// TODO: Extract the current owner from JSON
	owner := jdoc.ClientMetadata // XXX: Fake logic to use jdoc variable
	return owner, nil
}

//////////////////////////////////////////////////////////////////////

// GDPRViewData returns a map of doc-->JsonDoc, for all documents associated
// with the account that have GDPR-sensitive data.
func (a *API) GDPRViewData(clientType ClientType, account string) (map[string]string, error) {
	// TODO: Authenticate
	// TODO: Authorize

	// Make list of document names that have GDPR data
	docKeyList := make([]jdocsdb.JdocKey, 0)
	for docName, docSpec := range a.jdocSpecs {
		if docSpec.HasGDPRData {
			docKeyList = append(docKeyList, jdocsdb.JdocKey{
				TableName: docSpec.TableName,
				Account:   account,
				Thing:     "IGNORED",
				DocType:   docSpec.DocType,
				DocName:   docName,
			})
		}
	}

	// Fetch all of the documents in the list, for all Thing values.
	result := make(map[string]string)
	for _, fkey := range docKeyList {
		keyList, err := a.jdb.FindDocsAssociatedWithAccount(fkey)
		if err != nil {
			// TODO: Log warning
			continue // TODO: Should return an error?
		}
		for _, key := range keyList {
			stronglyConsistent := false // TODO: Is Eventually Consistent read ok?
			jdoc, err := a.jdb.ReadDoc(key, stronglyConsistent)
			if err != nil {
				// TODO: Log warning
				continue // TODO: Should return an error?
			}
			result[key.DocName+"_"+key.Account+"_"+key.Thing] = jdoc.JsonDoc
		}
	}
	return result, nil
}

func (a *API) DeleteAllAccountData(clientType ClientType, account string) error {
	// TODO: Authenticate

	// Identify all documents whose DocType includes Account association.
	// Also, check that delete operation is authorized.
	docKeyList := make([]jdocsdb.JdocKey, 0)
	for docName, docSpec := range a.jdocSpecs {
		if docSpec.DocType != jdocsdb.DocTypeThing {
			docKeyList = append(docKeyList, jdocsdb.JdocKey{
				TableName: docSpec.TableName,
				Account:   account,
				Thing:     "IGNORED",
				DocType:   docSpec.DocType,
				DocName:   docName,
			})
		}
		if !docSpec.AllowDelete[clientType] {
			return jdocserr.Errorf(codes.PermissionDenied, jdocserr.DeleteDocNotAllowedCode, "ClientType=%s DocName=%s", clientType, docName)
		}
	}

	// Delete all of the documents in the list, for all Thing values.
	for _, fkey := range docKeyList {
		keyList, err := a.jdb.FindDocsAssociatedWithAccount(fkey)
		if err != nil {
			// TODO: Log warning
			continue // TODO: Should return an error?
		}
		for _, key := range keyList {
			if err := a.jdb.DeleteDoc(key); err != nil {
				// TODO: Log warning
				continue // TODO: Should return an error?
			}
		}
	}
	return nil
}
