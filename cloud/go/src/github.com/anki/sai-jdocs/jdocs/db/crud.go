package jdocsdb

// crud.go
//
// Implements basic Create/Update/Read/Delete ("CRUD") operations for a Jdoc
// stored in a DynamoDB table.
//
// - Handles mapping between DynamoDB attributes and Jdoc type.
//
// - Converts JSON document string to/from DynamoDB document type.
//
// - Each function call operates on a single DynamoBB table.
//
// - Read can fetch multiple rows from a table => read multiple docs at once,
//   for most efficient RCU (Read Capacity Unit) usage.
//
// - Create/Update/Delete affect a single document only.
//
// - Things that must be done externally:
//   - Authentication
//   - Authorization
//   - Determining the DB table name
//   - Determining the Account/Thing/AccountThing association for the document

import (
	"fmt"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/dynamodb"
	"google.golang.org/grpc/codes"

	"github.com/anki/sai-awsutil/dynamoutil"
	"github.com/anki/sai-jdocs/jdocs/err" // package jdocserr
)

// CreateDoc creates a new document. If the document already exists, it is an
// error.
func (jdb *JdocsDB) CreateDoc(key JdocKey, doc Jdoc) error {
	if jerr := doc.ValidateJSON(); jerr != nil {
		return jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadJsonCode, "%s", jerr)
	}
	item, kerr := jdb.dbKeyItem(key)
	if kerr != nil {
		return kerr
	}
	now := time.Now()
	item.SetTimeValue(attrCreated, now)
	item.SetTimeValue(attrUpdated, now)
	item.SetStringValue(attrDocName, key.DocName)
	if key.DocType != DocTypeAccount {
		// don't include this attribute for non-Thing documents
		item.SetStringValue(attrThing, key.Thing)
	}
	item.SetUintValue(attrDocVersion, doc.DocVersion)
	item.SetUintValue(attrFmtVersion, doc.FmtVersion)
	item.SetStringValue(attrClientMetadata, doc.ClientMetadata)
	item.SetStringValue(attrJsonDoc, doc.JsonDoc)
	return jdb.db.GetTable(key.TableName).SimplePutItemNew(item)
}

// ReadDoc reads a single document. An empty document with DocVersion==0 is
// returned if the document does not exist. The Read can be either Eventually
// Consistent or Strongly Consistent.
func (jdb *JdocsDB) ReadDoc(key JdocKey, stronglyConsistent bool) (*Jdoc, error) {
	item, kerr := jdb.dbKeyItem(key)
	if kerr != nil {
		return &Jdoc{}, kerr
	}
	gii := dynamodb.GetItemInput{ConsistentRead: aws.Bool(stronglyConsistent)}
	if gotItem, gerr := jdb.db.GetTable(key.TableName).GetItem(item, &gii); gerr != nil {
		return &Jdoc{}, gerr
	} else if len(gotItem) == 0 {
		// document does not exist
		return &Jdoc{}, nil
	} else {
		dui := dynamoutil.Item(gotItem)
		return &Jdoc{
			DocVersion:     dui.GetUintValue(attrDocVersion),
			FmtVersion:     dui.GetUintValue(attrFmtVersion),
			ClientMetadata: dui.GetStringValue(attrClientMetadata),
			JsonDoc:        dui.GetStringValue(attrJsonDoc),
		}, nil
	}
}

// ReadDocs reads and returns 0+ documents from one table, and adds the results
// into the supplied map (DocName-->*Jdoc). All Read operations use Eventual
// Consistency. For documents that do not exist, nothing will get put into the
// result, and it is not an error.
func (jdb *JdocsDB) ReadDocs(key JdocKey, docsToRead *ReadDocsInput, outDocs *ReadDocsOutput) error {
	if _, kerr := jdb.dbKeyItem(key); kerr != nil {
		return kerr
	}

	// For the special case of reading only one document, don't waste RCUs with a
	// Query that reads >1 item.
	if len(*docsToRead) == 1 {
		// pull the doc name out of the map (single iteration)
		for docName, _ := range *docsToRead {
			key.DocName = docName
		}
		doc, err := jdb.ReadDoc(key, false /*stronglyConsistent*/)
		if err != nil {
			return err
		}
		(*outDocs)[key.DocName] = doc
		return nil
	}

	// Multiple GetItem() calls would consume multiple Read Capacity Units.
	// Instead, can be more efficient using careful item Query.
	//
	// https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/CapacityUnitCalculations.html
	//   "Query: reads multiple items that have the same partition key value. All
	//    of the items returned are treated as a single read operation, where
	//    DynamoDB computes the total size of all items and then rounds up to the
	//    next 4 KB boundary. For example, suppose your query returns 10 items
	//    whose combined size is 40.8 KB. DynamoDB rounds the item size for the
	//    operation to 44 KB. If a query returns 1500 items of 64 bytes each, the
	//    cumulative size is 96 KB."
	// The exact query command depends on the document association.
	var queryKey, docnamePrefix string
	switch key.DocType {
	case DocTypeAccount:
		queryKey = key.Account
		docnamePrefix = key.Account + "_"
	case DocTypeThing:
		queryKey = key.Thing
		docnamePrefix = key.Thing + "_"
	case DocTypeAccountThing:
		queryKey = key.Account
		docnamePrefix = key.Account + "_" + key.Thing + "_"
	default: // it is required and assumed that DocType is valid
		return fmt.Errorf("DocType=%s not supported", key.DocType)
	}
	//fmt.Printf("tableName=%s, docnamePrefix=%s\n", key.TableName, docnamePrefix)

	// Assign a default "not found" value for every requested document
	for docName, _ := range *docsToRead {
		(*outDocs)[docName] = &Jdoc{}
	}

	// Perform the query and accumulate the results
	err := jdb.db.GetTable(key.TableName).QueryPages(
		// Function called with every Query Output page:
		func(qo *dynamodb.QueryOutput, isLastPage bool) bool {
			for _, item := range qo.Items {
				dui := dynamoutil.Item(item)
				docName := dui.GetStringValue(attrDocName)
				if _, ok := (*docsToRead)[docName]; ok { // don't return unrequested docs!
					(*outDocs)[docName] = &Jdoc{
						DocVersion:     dui.GetUintValue(attrDocVersion),
						FmtVersion:     dui.GetUintValue(attrFmtVersion),
						ClientMetadata: dui.GetStringValue(attrClientMetadata),
						JsonDoc:        dui.GetStringValue(attrJsonDoc),
					}
				}
			}
			return true // continue iterating through pages
		},
		// Query Config:
		dynamoutil.WithCompoundKey(
			queryKey,
			dynamoutil.SortKeyExpression{
				Type:   dynamoutil.BeginsWith,
				Value1: &dynamodb.AttributeValue{S: aws.String(docnamePrefix)},
			}),
	)
	return err
}

// UpdateDoc updates the value of an existing document. The update is
// conditional: Item must already exist, and DynamoDB DocVersion and FmtVerison
// values must be compatible with new document.
//
// XXX(gwenz): Ideally, UpdateDoc() would be dumb and would blindly perform the
// requested update. However, we want the write to be conditional (to catch
// design errors), and we also want to keep the package interface simple.
func (jdb JdocsDB) UpdateDoc(key JdocKey, new Jdoc) error {
	if jerr := new.ValidateJSON(); jerr != nil {
		return jdocserr.Errorf(codes.InvalidArgument, jdocserr.BadJsonCode, "%s", jerr)
	}

	item, kerr := jdb.dbKeyItem(key)
	if kerr != nil {
		return kerr
	}
	item.SetTimeValue(attrUpdated, time.Now())
	item.SetStringValue(attrDocName, key.DocName)
	if key.DocType != DocTypeAccount {
		// don't include this attribute for non-Thing documents
		item.SetStringValue(attrThing, key.Thing)
	}
	item.SetUintValue(attrDocVersion, new.DocVersion)
	item.SetUintValue(attrFmtVersion, new.FmtVersion)
	item.SetStringValue(attrClientMetadata, new.ClientMetadata)
	item.SetStringValue(attrJsonDoc, new.JsonDoc)

	pii := dynamodb.PutItemInput{
		// Use ExpressionAttributeNames to avoid conflict with DynamoDB reserved words
		ConditionExpression: aws.String("attribute_exists(#k) AND (#d = :dv) AND (#f <= :fv)"),
		ExpressionAttributeNames: map[string]*string{
			"#k": aws.String(jdb.db.GetTable(key.TableName).PrimaryKey.GetKeyName()),
			"#d": aws.String(attrDocVersion),
			"#f": aws.String(attrFmtVersion),
		},
		ExpressionAttributeValues: map[string]*dynamodb.AttributeValue{
			":dv": &dynamodb.AttributeValue{N: aws.String(fmt.Sprintf("%d", new.DocVersion-1))},
			":fv": &dynamodb.AttributeValue{N: aws.String(fmt.Sprintf("%d", new.FmtVersion))},
		},
	}
	return jdb.db.GetTable(key.TableName).PutItem(item, &pii)
}

// DeleteDoc deletes a document. If the document does not exist, it is not an
// error.
func (jdb *JdocsDB) DeleteDoc(key JdocKey) error {
	item, kerr := jdb.dbKeyItem(key)
	if kerr != nil {
		return kerr
	}
	return jdb.db.GetTable(key.TableName).SimpleDeleteItem(item)
}
