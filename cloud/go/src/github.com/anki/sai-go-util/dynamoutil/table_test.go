package dynamoutil

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// This just tests the simple stuff that we can execute without
// running against Dynamo. Because the dynamoutil/dynamotest package
// imports dynamoutil, we can't run test code that uses dynamotest
// here because of circular imports.

func TestAttributeSpec_GetType(t *testing.T) {
	assert.Equal(t, AttributeTypeString, AttributeSpec{}.GetType())
	assert.Equal(t, AttributeTypeNumber, AttributeSpec{Type: AttributeTypeNumber}.GetType())
}

func TestKeySpec_Names(t *testing.T) {
	assert.Equal(t, DefaultAttributeNameKey, KeySpec{}.GetKeyName())
	assert.Equal(t, "Id", KeySpec{KeyName: "Id"}.GetKeyName())
	assert.Equal(t, DefaultAttributeNameSubkey, KeySpec{}.GetSubkeyName())
	assert.Equal(t, "Name", KeySpec{SubkeyName: "Name"}.GetSubkeyName())
}

func TestTableSpec_RegisterTable(t *testing.T) {
	assert.Equal(t, 0, len(Tables))
	RegisterTable(TableSpec{})
	assert.Equal(t, 1, len(Tables))
	RegisterTables([]TableSpec{
		TableSpec{},
		TableSpec{},
	})
	assert.Equal(t, 3, len(Tables))
}

func TestTableSpec_TablePrefix(t *testing.T) {
	assert.Equal(t, "MyTable", TableSpec{Name: "MyTable"}.GetTableName())
	TablePrefix = "dev-"
	defer func() { TablePrefix = "" }()
	assert.Equal(t, "dev-MyTable", TableSpec{Name: "MyTable"}.GetTableName())
}

func TestTableSpec_GetIndex(t *testing.T) {
	ts := TableSpec{
		LocalIndexes: []IndexSpec{
			{Name: "li1"},
			{Name: "li2"},
		},
		GlobalIndexes: []IndexSpec{
			{Name: "gi1"},
			{Name: "gi2"},
		},
	}
	i, err := ts.GetIndex("li1")
	require.Nil(t, err)
	assert.Equal(t, "li1", i.Name)

	i, err = ts.GetIndex("gi2")
	require.Nil(t, err)
	assert.Equal(t, "gi2", i.Name)

	i, err = ts.GetIndex("whatever")
	require.NotNil(t, err)
}
