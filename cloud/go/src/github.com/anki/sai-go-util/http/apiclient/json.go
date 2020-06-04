package apiclient

import (
	"errors"
)

var (
	NotFound     = errors.New("Entry not found")
	NotString    = errors.New("Entry was not a string")
	NotArray     = errors.New("Entry was not an array")
	OutOfBounds  = errors.New("Array index was out of bounds")
	NotObject    = errors.New("Entry was not an object")
	KeyWrongType = errors.New("Key was not a string or int")
)

// Wrap a json response with some utility methods
type Json map[string]interface{}

// Field fetches a subkey from a field
// eg to access response["session"]["session_token"] safely use
// response.Field("session", "session_token")
// returns nil if the key doesn't exist
func (j Json) Field(subkeys ...string) (interface{}, error) {
	var m interface{} = map[string]interface{}(j)
	var ok bool
	for _, key := range subkeys {
		m, ok = m.(map[string]interface{})[key]
		if !ok {
			return nil, NotFound
		}
	}
	return m, nil
}

// FieldStr returns a subkey from a field as a string
// It will return a NotString error if the leaf isn't a string
// or a NotFound error if the item doesn't exist
func (j Json) FieldStr(subkeys ...string) (string, error) {
	result, err := j.Field(subkeys...)
	if err != nil {
		return "", err
	}
	if result, ok := result.(string); ok {
		return result, nil
	}
	return "", NotString
}

// FieldStrQ returns a subkey from a field as a string.
// It will return the empty string if the field doesn't exist,
// or isn't a string.
func (j Json) FieldStrQ(subkeys ...string) (result string) {
	result, _ = j.FieldStr(subkeys...)
	return result
}

// FieldAryStr returns a subkey from a field as a string and supports array indices
// It will return a NotString error if the leaf isn't a string
// or a NotFound error if the item doesn't exist
func (j Json) FieldArrayStr(subkeys ...interface{}) (string, error) {
	result, err := j.FieldArray(subkeys...)
	if err != nil {
		return "", err
	}
	if result, ok := result.(string); ok {
		return result, nil
	}
	return "", NotString
}

// FieldArray fetches a subkey from a field, and supports array indices
// eg to access response["session"][1]["session_token"] safely use
// response.Field("session", 1, "session_token")
// returns nil if the key doesn't exist
func (j Json) FieldArray(subkeys ...interface{}) (interface{}, error) {
	// top level json array is technically valid, but doesn't seem to be representable in Go json
	// so we always assume the top level is a json object
	var m interface{} = map[string]interface{}(j)
	var ok bool

	for _, key := range subkeys {
		switch v := key.(type) {
		case string:
			// key is a string, so check that we have the expected json object
			_, ok = m.(map[string]interface{})
			if !ok {
				return nil, NotObject
			}

			m, ok = m.(map[string]interface{})[v]
			if !ok {
				return nil, NotFound
			}
		case int:
			// key is an int, so check that we have the expected json array
			_, ok = m.([]interface{})
			if !ok {
				return nil, NotArray
			}

			if len(m.([]interface{})) <= v || v < 0 {
				return nil, OutOfBounds
			}

			m = m.([]interface{})[v]
		default:
			return nil, KeyWrongType
		}
	}

	return m, nil
}
