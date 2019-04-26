package db

import (
	"errors"
	"net/textproto"
	"strings"

	"github.com/anki/sai-blobstore/apierrors"
	"github.com/anki/sai-go-util/dynamoutil"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

const (
	TableBaseName = "metadata"

	AttributeId                 = "id"
	AttributeMetadataKey        = "mdkey"
	AttributeMetadataValue      = "mdvalue"
	AttributeMetadataValueLower = "mdvalue-lc"

	IndexMetadataKey = "by-mdkey"
)

var (
	ErrInvalidKey = errors.New("invalid key")
)

func init() {
	dynamoutil.RegisterTable(table)
}

var (
	/*
		Metadata is stored in dynamo with one row per metadata key-value
		There are columns for the blob id name, the key name, the key value and the
		lower cased key value (used for comparison searches to make it case in-sensitive)
	*/

	table = dynamoutil.TableSpec{
		Name: TableBaseName,

		PrimaryKey: dynamoutil.KeySpec{
			KeyName:    AttributeId,
			SubkeyName: AttributeMetadataKey,
			SubkeyType: dynamoutil.AttributeTypeString,
		},

		// Additional attributes that are used in secondary indexes
		// need to be defined at table creation time. Attributes that
		// are just projected, but not part of the key, do not need to
		// be pre-defined.
		Attributes: []dynamoutil.AttributeSpec{
			{Name: AttributeMetadataValueLower},
		},

		GlobalIndexes: []dynamoutil.IndexSpec{
			{
				Name: IndexMetadataKey,
				Key: dynamoutil.KeySpec{
					KeyName:    AttributeMetadataKey,
					SubkeyName: AttributeMetadataValueLower,
					SubkeyType: dynamoutil.AttributeTypeString,
				},
				AdditionalFields: []string{
					AttributeId,
					AttributeMetadataValue,
				},
			},
		},
	}
)

type Record struct {
	Id            string `json:"id"`
	MetadataKey   string `json:"mdkey"`
	MetadataValue string `json:"mdvalue"`
}

func TableName() string {
	return table.GetTableName()
}

func SaveBlobMetadata(id string, metadata map[string]string) error {
	requests := make([]*dynamodb.WriteRequest, len(metadata))
	i := 0
	for k, v := range metadata {
		k = normalizeKey(k)
		requests[i] = &dynamodb.WriteRequest{
			PutRequest: &dynamodb.PutRequest{
				Item: map[string]*dynamodb.AttributeValue{
					AttributeId:                 dynamoutil.MakeStringValue(id),
					AttributeMetadataKey:        dynamoutil.MakeStringValue(k),
					AttributeMetadataValue:      dynamoutil.MakeStringValue(v),
					AttributeMetadataValueLower: dynamoutil.MakeStringValue(strings.ToLower(v)),
				},
			},
		}
		i++
	}
	ipt := table.NewBatchWriteItemInput(requests)
	return dynamoutil.BatchWriteItems(ipt)
}

// ByID returns all metadata for the given blob id
func ByID(id string) (metadata map[string]string, err error) {
	if id == "" {
		return nil, apierrors.BlobIDNotFound
	}

	resp, err := table.Query(dynamoutil.WithSimpleKey(id))
	if err != nil {
		return nil, err
	}

	if len(resp.Items) == 0 {
		return nil, apierrors.BlobIDNotFound
	}

	metadata = make(map[string]string)
	for _, item := range resp.Items {
		key := dynamoutil.GetStringValue(item[AttributeMetadataKey])
		value := dynamoutil.GetStringValue(item[AttributeMetadataValue])
		metadata[key] = value
	}
	return metadata, nil
}

type SearchMode string

const (
	SearchEqual      SearchMode = "eq"
	SearchGT         SearchMode = "gt"
	SearchGTE        SearchMode = "gte"
	SearchLT         SearchMode = "lt"
	SearchLTE        SearchMode = "lte"
	SearchBetween    SearchMode = "between"
	SearchBeginsWith SearchMode = "prefix"
)

var modeToExpressionOp = map[SearchMode]dynamoutil.SortKeyExpressionOp{
	SearchEqual:      dynamoutil.Equal,
	SearchGT:         dynamoutil.GreaterThan,
	SearchGTE:        dynamoutil.GreaterThanOrEqual,
	SearchLT:         dynamoutil.LessThan,
	SearchLTE:        dynamoutil.LessThanOrEqual,
	SearchBetween:    dynamoutil.Between,
	SearchBeginsWith: dynamoutil.BeginsWith,
}

// ByMetadataKey finds all blobs matching the specified metadata key
// using a case-insensitive value search
func ByMetadataKey(key, value1, value2 string, searchtype SearchMode) (results []Record, err error) {
	var exp dynamoutil.SortKeyExpression

	// normalize the key
	key = normalizeKey(key)
	value1 = strings.ToLower(value1)
	value2 = strings.ToLower(value2)

	switch searchtype {
	case SearchEqual, SearchGT, SearchGTE, SearchLT, SearchLTE, SearchBeginsWith:
		exp = dynamoutil.SortKeyExpression{
			Type:   modeToExpressionOp[searchtype],
			Value1: value1,
		}
	case SearchBetween:
		exp = dynamoutil.SortKeyExpression{
			Type:   modeToExpressionOp[searchtype],
			Value1: value1,
			Value2: value2,
		}

	default:
		return nil, apierrors.BadSearchMode
	}

	resp, err := table.Query(
		dynamoutil.WithIndex(IndexMetadataKey),
		dynamoutil.WithCompoundKey(key, exp))

	if err != nil {
		return nil, err
	}

	results = make([]Record, len(resp.Items))
	for i, item := range resp.Items {
		results[i] = Record{
			Id:            dynamoutil.GetStringValue(item[AttributeId]),
			MetadataKey:   dynamoutil.GetStringValue(item[AttributeMetadataKey]),
			MetadataValue: dynamoutil.GetStringValue(item[AttributeMetadataValue]),
		}
	}
	return results, nil
}

func normalizeKey(key string) string {
	return textproto.CanonicalMIMEHeaderKey(key)
}
