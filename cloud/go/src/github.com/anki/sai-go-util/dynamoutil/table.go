package dynamoutil

import (
	"errors"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

const (
	// DefaultRead & DefaultWrite specify the provisioned throughput
	// capacity of each DynamoDB table
	DefaultRead                = 10
	DefaultWrite               = 10
	DefaultAttributeNameKey    = "key"
	DefaultAttributeNameSubkey = "subkey"
	AttributeTypeString        = "S"
	AttributeTypeNumber        = "N"
	AttributeTypeBinary        = "B"
	KeyTypeHash                = "HASH"
	KeyTypeRange               = "RANGE"
)

var (
	Tables []TableSpec
)

// RegisterTable registers a single table definition, to be acted on
// by CreateTables and DeleteTables.
func RegisterTable(t TableSpec) {
	Tables = append(Tables, t)
}

// RegisterTables registers a list of table definitions.
func RegisterTables(t []TableSpec) {
	Tables = append(Tables, t...)
}

// ----------------------------------------------------------------------
// Attributes
// ----------------------------------------------------------------------

// AttributeSpec is a simple tuple pairing an attribute name and its
// type specifier.
type AttributeSpec struct {
	Name string `json:"name"`
	Type string `json:"type,omitempty"`
}

// GetType returns the attribute's data type, defaulting to string if
// no type has been specified.
func (a AttributeSpec) GetType() string {
	if a.Type == "" {
		return AttributeTypeString
	}
	return a.Type
}

// ToAttributeDefinition converts an AttributeSpec to an AWS SDK
// representation of the same attribute.
func (a AttributeSpec) ToAttributeDefinition() *dynamodb.AttributeDefinition {
	return &dynamodb.AttributeDefinition{
		AttributeName: aws.String(a.Name),
		AttributeType: aws.String(a.GetType()),
	}
}

// ----------------------------------------------------------------------
// Keys
// ----------------------------------------------------------------------

// KeySpec represents the data we need to define the primary key for a
// table, local index, or global index.
type KeySpec struct {
	KeyName    string `json:"key_name" yaml:"key_name"`
	SubkeyName string `json:"subkey_name,omitempty" yaml:"subkey_name,omitempty"`
	SubkeyType string `json:"subkey_type,omitempty" yaml:"subkey_type,omitempty"`
}

// GetKeyName returns the TableSpec's key attribute name, or the
// default key attribute name if an override has not been set.
func (k KeySpec) GetKeyName() string {
	if k.KeyName == "" {
		return DefaultAttributeNameKey
	}
	return k.KeyName
}

// GetSubkeyName returns the TableSpec's subkey attribute name, or the
// default subkey attribute name if an override has not been set.
func (k KeySpec) GetSubkeyName() string {
	if k.SubkeyName == "" {
		return DefaultAttributeNameSubkey
	}
	return k.SubkeyName
}

// ToAttributeDefinitions returns an AWS SDK representation of the
// table attributes for this key suitable for use in a CreateTable or
// other call.
func (k KeySpec) ToAttributeDefinitions() []*dynamodb.AttributeDefinition {
	attrs := []*dynamodb.AttributeDefinition{
		{
			AttributeName: aws.String(k.GetKeyName()),
			AttributeType: aws.String(AttributeTypeString),
		},
	}
	if k.SubkeyType != "" {
		attrs = append(
			attrs,
			&dynamodb.AttributeDefinition{
				AttributeName: aws.String(k.GetSubkeyName()),
				AttributeType: aws.String(k.SubkeyType),
			},
		)
	}
	return attrs
}

// ToKeySchema returns an AWS SDK representation of the key suitable
// for use in a CreateTable or other call.
func (k KeySpec) ToKeySchema() []*dynamodb.KeySchemaElement {
	schema := []*dynamodb.KeySchemaElement{
		{
			AttributeName: aws.String(k.GetKeyName()),
			KeyType:       aws.String(KeyTypeHash),
		},
	}
	if k.SubkeyType != "" {
		schema = append(
			schema,
			&dynamodb.KeySchemaElement{
				AttributeName: aws.String(k.GetSubkeyName()),
				KeyType:       aws.String(KeyTypeRange),
			},
		)
	}
	return schema
}

// ToKeyValue returns an AWS SDK representation of a key value suitable for use
// in DeleteItem calls.
func (k KeySpec) ToKeyValue(hashValue string, subkeyValue interface{}) (map[string]*dynamodb.AttributeValue, error) {
	result := map[string]*dynamodb.AttributeValue{
		k.KeyName: MakeStringValue(hashValue),
	}

	if k.SubkeyType != "" {
		switch k.SubkeyType {
		case AttributeTypeNumber:
			switch v := subkeyValue.(type) {
			case float64:
				result[k.SubkeyName] = MakeFloatValue(v)
			case int64:
				result[k.SubkeyName] = MakeIntValue(v)
			case int:
				result[k.SubkeyName] = MakeIntValue(int64(v))
			default:
				return nil, errors.New("invalid value type for numeric subkey; must be float64, int or int64")
			}

		case AttributeTypeString:
			if v, ok := subkeyValue.(string); ok {
				result[k.SubkeyName] = MakeStringValue(v)
			} else {
				return nil, errors.New("invalid value type for string subkey")
			}

		case AttributeTypeBinary:
			if v, ok := subkeyValue.([]byte); ok {
				result[k.SubkeyName] = MakeBinaryValue(v)
			} else {
				return nil, errors.New("invalid value type for binary subkey")
			}

		default:
			panic(fmt.Sprintf("unhandled subkey type %s", k.SubkeyType))
		}
	}

	return result, nil
}

// ----------------------------------------------------------------------
// Indexes
// ----------------------------------------------------------------------

// IndexSpec represents the data we need to define a local or global
// seondary index.
type IndexSpec struct {
	Name             string   `json:"name"`
	Key              KeySpec  `json:"key"`
	AdditionalFields []string `json:"additional_fields,omitempty" yaml:"additional_fields,omitempty"`
}

// getProjection returns the AWS SDK Project structure required by
// local and global secondary index definitions.
func (i IndexSpec) getProjection() *dynamodb.Projection {
	p := &dynamodb.Projection{
		ProjectionType: aws.String("KEYS_ONLY"),
	}
	if len(i.AdditionalFields) > 0 {
		p.ProjectionType = aws.String("INCLUDE")
		for _, f := range i.AdditionalFields {
			p.NonKeyAttributes = append(p.NonKeyAttributes, aws.String(f))
		}
	}
	return p
}

// ToLocalSecondaryIndex converts an IndexSpec to an AWS SDK
// LocalSecondaryIndex struct needed for a CreateTable call.
func (i IndexSpec) ToLocalSecondaryIndex() *dynamodb.LocalSecondaryIndex {
	idx := &dynamodb.LocalSecondaryIndex{
		IndexName:  aws.String(i.Name),
		Projection: i.getProjection(),
		KeySchema:  i.Key.ToKeySchema(),
	}
	return idx
}

// ToGlobalSecondaryIndex converts an IndexSpec to an AWS SDK
// GlobalSecondaryIndex struct needed for a CreateTable call.
func (i IndexSpec) ToGlobalSecondaryIndex() *dynamodb.GlobalSecondaryIndex {
	idx := &dynamodb.GlobalSecondaryIndex{
		IndexName:  aws.String(i.Name),
		Projection: i.getProjection(),
		KeySchema:  i.Key.ToKeySchema(),
		ProvisionedThroughput: &dynamodb.ProvisionedThroughput{
			ReadCapacityUnits:  aws.Int64(DefaultRead),
			WriteCapacityUnits: aws.Int64(DefaultWrite),
		},
	}
	return idx
}

// ----------------------------------------------------------------------
// Tables
// ----------------------------------------------------------------------

// TableSpec defines a few very basic properties of a single Dynamo
// table, which drive the rest of our dynamo infrastructure.
type TableSpec struct {
	Name          string          `json:"name"`
	PrimaryKey    KeySpec         `json:"primary_key" yaml:"primary_key"`
	Attributes    []AttributeSpec `json:"attributes"`
	LocalIndexes  []IndexSpec     `json:"local_indexes,omitempty" yaml:"local_indexes,omitempty"`
	GlobalIndexes []IndexSpec     `json:"global_indexes,omitempty" yaml:"global_indexes,omitempty"`
}

// GetTableName returns the TableSpec's table name with the globally
// configured table prefix applied.
func (s TableSpec) GetTableName() string {
	return TablePrefix + s.Name
}

// GetKeyName returns the name of the primary key for the table.
func (s TableSpec) GetKeyName() string {
	return s.PrimaryKey.GetKeyName()
}

func (s TableSpec) KeyForQueryInput(q *dynamodb.QueryInput) (k KeySpec, err error) {
	if q.IndexName != nil && *q.IndexName != "" {
		if idx, err := s.GetIndex(*q.IndexName); err != nil {
			return k, err
		} else {
			return idx.Key, nil
		}
	}
	return s.PrimaryKey, nil
}

// GetIndex resolves an index name to the IndexSpec object describing
// that index.
func (s TableSpec) GetIndex(name string) (IndexSpec, error) {
	for _, idx := range s.LocalIndexes {
		if idx.Name == name {
			return idx, nil
		}
	}
	for _, idx := range s.GlobalIndexes {
		if idx.Name == name {
			return idx, nil
		}
	}
	return IndexSpec{}, fmt.Errorf("Index '%s' not found", name)
}

// NewCreateTableInput converts a TableSpec to a
// dynamodb.CreateTableInput, suitable for passing to the SDK
// CreateTable() API call to provision a table in Dynamo.
//
// By default, all our tables use the attributes "key" and "subkey"
// for the hash key and range key, respectively, although that can be
// overridden.
func (s TableSpec) NewCreateTableInput() *dynamodb.CreateTableInput {
	cti := dynamodb.CreateTableInput{
		TableName:            aws.String(s.GetTableName()),
		AttributeDefinitions: s.PrimaryKey.ToAttributeDefinitions(),
		KeySchema:            s.PrimaryKey.ToKeySchema(),
		ProvisionedThroughput: &dynamodb.ProvisionedThroughput{
			ReadCapacityUnits:  aws.Int64(DefaultRead),
			WriteCapacityUnits: aws.Int64(DefaultWrite),
		},
	}
	if len(s.Attributes) > 0 {
		for _, a := range s.Attributes {
			cti.AttributeDefinitions = append(cti.AttributeDefinitions, a.ToAttributeDefinition())
		}
	}
	if len(s.LocalIndexes) > 0 {
		for _, i := range s.LocalIndexes {
			cti.LocalSecondaryIndexes = append(cti.LocalSecondaryIndexes, i.ToLocalSecondaryIndex())
		}
	}
	if len(s.GlobalIndexes) > 0 {
		for _, i := range s.GlobalIndexes {
			cti.GlobalSecondaryIndexes = append(cti.GlobalSecondaryIndexes, i.ToGlobalSecondaryIndex())
		}
	}
	return &cti
}

// NewDeleteTableInput converts a TableSpec to a
// dynamodb.DeleteTableInput, suitable for passing to the SDK
// DeleteTable() API call to remove a table from Dynamo.
func (s TableSpec) NewDeleteTableInput() *dynamodb.DeleteTableInput {
	dti := dynamodb.DeleteTableInput{
		TableName: aws.String(s.GetTableName()),
	}
	return &dti
}

// NewDescribeTableInput returns an operation input structure for the
// DescribeTable operation. The structure will still require filling
// in the attributes to be stored.
func (s TableSpec) NewDescribeTableInput() *dynamodb.DescribeTableInput {
	return &dynamodb.DescribeTableInput{
		TableName: aws.String(s.GetTableName()),
	}
}

// NewGetItemInput returns an operation input structure for the GetItem
// operation.
func (s TableSpec) NewGetItemInput() *dynamodb.GetItemInput {
	return &dynamodb.GetItemInput{
		TableName: aws.String(s.GetTableName()),
	}
}

// NewDeleteItemInput returns an operation input structure for the DeleteItem
// operation.
func (s TableSpec) NewDeleteItemInput() *dynamodb.DeleteItemInput {
	return &dynamodb.DeleteItemInput{
		TableName: aws.String(s.GetTableName()),
	}
}

// NewPutItemInput returns an operation input structure for the PutItem
// operation.
func (s TableSpec) NewPutItemInput() *dynamodb.PutItemInput {
	return &dynamodb.PutItemInput{
		TableName: aws.String(s.GetTableName()),
	}
}

// NewPutItemIfNewInput returns an operation input structure for the
// PutItem operation conditionalized to insert only if the key does
// not exist already.
func (s TableSpec) NewPutItemIfNewInput() *dynamodb.PutItemInput {
	return &dynamodb.PutItemInput{
		TableName:           aws.String(s.GetTableName()),
		ConditionExpression: aws.String("attribute_not_exists(#k)"),
		ExpressionAttributeNames: map[string]*string{
			"#k": aws.String(s.PrimaryKey.GetKeyName()), // "key" is a reserved word
		},
	}
}

// NewBatchWriteItemInput returns an operation input structure for the
// BatchWriteItem operation.
//
// It configures the RequestItems map using the supplied requests and
// sets the map key name to the name of the table.
func (s TableSpec) NewBatchWriteItemInput(requestItems []*dynamodb.WriteRequest) *dynamodb.BatchWriteItemInput {
	return &dynamodb.BatchWriteItemInput{
		RequestItems: map[string][]*dynamodb.WriteRequest{
			s.GetTableName(): requestItems,
		},
	}
}

// ----------------------------------------------------------------------
// Query
// ----------------------------------------------------------------------

type SortKeyExpressionOp string

const (
	Equal              SortKeyExpressionOp = "="
	LessThan           SortKeyExpressionOp = "<"
	LessThanOrEqual    SortKeyExpressionOp = "<="
	GreaterThan        SortKeyExpressionOp = ">"
	GreaterThanOrEqual SortKeyExpressionOp = ">="
	Between            SortKeyExpressionOp = "between"
	BeginsWith         SortKeyExpressionOp = "begins_with"
)

// A SortKeyExpression is used with WithCompoundKey to specify how a sort
// key should be matched for a query.
type SortKeyExpression struct {
	Type   SortKeyExpressionOp // Operator to use for the query
	Value1 string              // Value to apply to the operator
	Value2 string              // Second value used with the Between operator
}

// AsExpression renders the value as a string expression that can be
// used with the DynamoDB API.
func (sk SortKeyExpression) AsExpression() (string, error) {
	switch sk.Type {
	case Equal, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual:
		return fmt.Sprintf("#sortkey %s :val1", sk.Type), nil

	case Between:
		return "#sortkey BETWEEN :val1 AND :val2", nil

	case BeginsWith:
		return "begins_with(#sortkey , :val1)", nil

	default:
		return "", errors.New("illegal sort key op")
	}
}

// UpdateQueryInput updates an existing dynamodb.QueryInput and populates
// its KeyConditionExpression, ExpressionAttributeNames and  ExpressionAttributeValues
// fields based on the settings of the SortKeyExpression.
func (sk SortKeyExpression) UpdateQueryInput(keyName string, q *dynamodb.QueryInput) error {
	var exp string

	q.ExpressionAttributeNames["#sortkey"] = aws.String(keyName)
	q.ExpressionAttributeValues[":val1"] = MakeStringValue(sk.Value1)

	switch sk.Type {
	case Equal, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual:
		exp = fmt.Sprintf("#sortkey %s :val1", sk.Type)

	case Between:
		exp = "#sortkey BETWEEN :val1 AND :val2"
		q.ExpressionAttributeValues[":val2"] = MakeStringValue(sk.Value2)

	case BeginsWith:
		exp = "begins_with(#sortkey , :val1)"

	default:
		return errors.New("illegal sort key op")
	}

	q.KeyConditionExpression = aws.String(*q.KeyConditionExpression + " AND " + exp)

	return nil
}

// A QueryConfig is a configuration function that can be used with
// TableSpec.Query() or TableSpec.QueryInput() to set the parameters
// of the query.
type QueryConfig func(TableSpec, *dynamodb.QueryInput) error

// WithLimit limits the number of items returned by a query.
func WithLimit(limit int64) QueryConfig {
	return func(t TableSpec, q *dynamodb.QueryInput) error {
		q.Limit = aws.Int64(limit)
		return nil
	}
}

// WithIndex specifies an index to query on.
func WithIndex(index string) QueryConfig {
	return func(t TableSpec, q *dynamodb.QueryInput) error {
		if _, err := t.GetIndex(index); err != nil {
			return err
		}
		q.IndexName = aws.String(index)
		return nil
	}
}

// WithSimpleKey will set up the query with a simple key condition on
// the table's primary key equal to the specified value. If an index
// has already been set with WithIndex(), the key condition
// will apply to the index instead of the primary table.
func WithSimpleKey(key string) QueryConfig {
	return withKey(key, nil)
}

// WithCompoundKey will setup the query with both a partition key and
// a sort key expression to match on.
//
// If an index  has already been set with WithIndex(), the key condition
// will apply to the index instead of the primary table.
func WithCompoundKey(key string, skexp SortKeyExpression) QueryConfig {
	return withKey(key, &skexp)
}

func withKey(key string, skexp *SortKeyExpression) QueryConfig {
	return func(t TableSpec, q *dynamodb.QueryInput) error {
		k, err := t.KeyForQueryInput(q)
		if err != nil {
			return err
		}
		keyName := k.GetKeyName()
		q.KeyConditionExpression = aws.String("#key = :val")
		q.ExpressionAttributeNames = map[string]*string{
			"#key": aws.String(keyName),
		}
		q.ExpressionAttributeValues = map[string]*dynamodb.AttributeValue{
			":val": MakeStringValue(key),
		}

		if skexp != nil {
			skexp.UpdateQueryInput(k.SubkeyName, q)
		}
		return nil
	}
}

// WithStartKey sets an ExclusiveStartKey property on the query, which
// specifies a specific row to begin the query from. The set of
// attribute values is typically obtained from a previous query to
// implement paging of results.
func WithStartKey(key map[string]*dynamodb.AttributeValue) QueryConfig {
	return func(t TableSpec, q *dynamodb.QueryInput) error {
		q.ExclusiveStartKey = key
		return nil
	}
}

// SortAscending specifies that the results should be sorted in
// ascending key order.
func SortAscending(t TableSpec, q *dynamodb.QueryInput) error {
	q.ScanIndexForward = aws.Bool(true)
	return nil
}

// SortDescending specifies that the results should be sorted in
// descending key order.
func SortDescending(t TableSpec, q *dynamodb.QueryInput) error {
	q.ScanIndexForward = aws.Bool(false)
	return nil
}

// QueryInput produces an AWS SDK dynamodb.QueryInput structure
// suitable for passing to the ExecuteQuery() function, which can be
// configured by zero or more QueryConfig calls.
func (s TableSpec) QueryInput(config ...QueryConfig) (*dynamodb.QueryInput, error) {
	in := &dynamodb.QueryInput{
		TableName: aws.String(s.GetTableName()),
	}
	for _, cfg := range config {
		if err := cfg(s, in); err != nil {
			return nil, err
		}
	}
	return in, nil
}

// Query immediately executes a query operation.
func (s TableSpec) Query(config ...QueryConfig) (*dynamodb.QueryOutput, error) {
	if in, err := s.QueryInput(config...); err != nil {
		return nil, err
	} else {
		return DynamoDB.Query(in)
	}
}

// ExecuteQuery execute a previously constructed query.
func (s TableSpec) ExecuteQuery(q *dynamodb.QueryInput) (*dynamodb.QueryOutput, error) {
	return DynamoDB.Query(q)
}

// ----------------------------------------------------------------------
// Scan
// ----------------------------------------------------------------------

// A ScanConfig is a configuration function that can be used with
// TableSpec.Scan() or TableSpec.ScanIndexForward() to set the parameters
// of the query.
type ScanConfig func(TableSpec, *dynamodb.ScanInput) error

// WithScanLimit limits the number of items returned by a scan.
func WithScanLimit(limit int64) ScanConfig {
	return func(t TableSpec, q *dynamodb.ScanInput) error {
		q.Limit = aws.Int64(limit)
		return nil
	}
}

// WithScanIndex specifies an index to scan instead of the primary
// table.
func WithScanIndex(index string) ScanConfig {
	return func(t TableSpec, q *dynamodb.ScanInput) error {
		if _, err := t.GetIndex(index); err != nil {
			return err
		}
		q.IndexName = aws.String(index)
		return nil
	}
}

// WithScanStartKey sets an ExclusiveStartKey property on the scan,
// which specifies a specific row to begin the scan from. The set of
// attribute values is typically obtained from a previous scan to
// implement paging of results.
func WithScanStartKey(key map[string]*dynamodb.AttributeValue) ScanConfig {
	return func(t TableSpec, q *dynamodb.ScanInput) error {
		q.ExclusiveStartKey = key
		return nil
	}
}

// ScanInput produces an AWS SDK dynamodb.ScanInput structure suitable
// for passing to the ExecuteScan() function, which can be configured
// by zero or more ScanConfig calls.
func (s TableSpec) ScanInput(config ...ScanConfig) (*dynamodb.ScanInput, error) {
	in := &dynamodb.ScanInput{
		TableName: aws.String(s.GetTableName()),
	}
	for _, cfg := range config {
		if err := cfg(s, in); err != nil {
			return nil, err
		}
	}
	return in, nil
}

// Scan immediately execute a scan operation.
func (s TableSpec) Scan(config ...ScanConfig) (*dynamodb.ScanOutput, error) {
	if in, err := s.ScanInput(config...); err != nil {
		return nil, err
	} else {
		return DynamoDB.Scan(in)
	}
}

// ExecuteScan executes a previously constructed scan.
func (s TableSpec) ExecuteScan(q *dynamodb.ScanInput) (*dynamodb.ScanOutput, error) {
	return DynamoDB.Scan(q)
}
