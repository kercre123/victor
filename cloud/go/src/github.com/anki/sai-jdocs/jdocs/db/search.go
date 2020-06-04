package jdocsdb

// search.go
//
// Search documents in JDOCS database, using query/scan/etc.

import (
	"fmt"
	"github.com/anki/sai-awsutil/dynamoutil"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

// FindDocsAssociatedWithAccount finds all documents with a particular DocName
// that are associated with the specified Account. For Account document types,
// this will return at most one document. For AccountThing document types, >1
// document may be returned, one for each Thing. Note that key.Thing argument
// value is ignored.
func (jdb *JdocsDB) FindDocsAssociatedWithAccount(key JdocKey) ([]JdocKey, error) {
	if _, kerr := jdb.dbKeyItem(key); kerr != nil {
		return []JdocKey{}, kerr
	}
	if key.DocType == DocTypeThing {
		return []JdocKey{}, fmt.Errorf("FindDocsAssociatedWithAccount not valid for DocName=%s with DocType=%s", key.DocName, key.DocType)
	}

	result := make([]JdocKey, 0)

	// Perform the query and accumulate the results
	err := jdb.db.GetTable(key.TableName).QueryPages(
		// Function called with every Query Output page:
		func(qo *dynamodb.QueryOutput, isLastPage bool) bool {
			for _, item := range qo.Items {
				if dynamoutil.Item(item).GetStringValue(attrDocName) == key.DocName {
					key := JdocKey{
						TableName: key.TableName,
						Account:   key.Account,
						Thing:     dynamoutil.Item(item).GetStringValue(attrThing),
						DocType:   key.DocType,
						DocName:   key.DocName,
					}
					result = append(result, key)
				}
			}
			return true // continue iterating through pages
		},
		// Query config:
		dynamoutil.WithSimpleKey(key.Account),
	)
	if err != nil {
		return []JdocKey{}, err
	}
	return result, nil
}
