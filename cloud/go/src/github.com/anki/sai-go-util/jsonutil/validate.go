package jsonutil

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strings"
)

// BadFieldsError is returned by ValidateKeys to reflect all key names
// that were used in a JSON document, but not defined in the supplied whitelist.
type BadFieldsError []string

// Error complies with the Error interface.
func (e BadFieldsError) Error() string {
	return fmt.Sprintf("Illegal fields in JSON data: %s", strings.Join(e, ","))
}

// Fields returns the field names that were used and not defined in the whitelist.
func (e BadFieldsError) Fields() []string {
	return []string(e)
}

func (e BadFieldsError) JsonError() JsonErrorResponse {
	return JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "invalid_fields",
		Message:    e.Error(),
		Metadata: map[string]interface{}{
			"fields": strings.Join(e, ","),
		},
	}
}

type WhiteField struct {
	Name     string
	Children []WhiteField
}

func parseError(err error) JsonError {
	return JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "json_parse_error",
		Message:    err.Error(),
	}
}

func validate(data map[string]interface{}, fieldlist []WhiteField, errPrefix string, badfields *[]string) (valid map[string]interface{}) {
	var (
		fieldData interface{}
		found     bool
	)
	valid = make(map[string]interface{})
	for _, field := range fieldlist {
		if fieldData, found = data[field.Name]; !found {
			// whitelisted field is not used
			continue
		}
		// Whitelisted field is used, check to see if the whitelist wants us
		// to check keys in child objects of this entry
		valid[field.Name] = true
		if Children := field.Children; Children != nil {
			if childData, ok := fieldData.(map[string]interface{}); ok {
				// child is an object
				validChildren := validate(childData, field.Children, errPrefix+field.Name+".", badfields)
				valid[field.Name] = validChildren
			} else if childArr, ok := fieldData.([]interface{}); ok {
				// child is an array; child if array elements are objects to validate too
				validList := []interface{}{}
				for i, childObj := range childArr {
					if childObj, ok := childObj.(map[string]interface{}); ok {
						prefix := fmt.Sprintf("%s%s.%d.", errPrefix, field.Name, i)
						validChildren := validate(childObj, field.Children, prefix, badfields)
						validList = append(validList, validChildren)
					}
				}
				valid[field.Name] = validList
			}
		}
		// done with this entry; remove it.
		delete(data, field.Name)
	}

	// anything left in data is not defined in the whitelist
	for fieldName, _ := range data {
		*badfields = append(*badfields, errPrefix+fieldName)
	}
	return valid
}

// ValidateKeys scans through a JSON object and recursively checks the key
// names used against a whitelist.
//
// Returns a map with keys that were supplied and are valid
func ValidateKeys(j []byte, whitelist []WhiteField) (valid map[string]interface{}, err JsonError) {
	if err := json.Unmarshal(j, &valid); err != nil {
		return nil, parseError(err)
	}

	var badfields []string
	valid = validate(valid, whitelist, "", &badfields)
	if len(badfields) == 0 {
		return valid, nil
	}

	return nil, BadFieldsError(badfields)
}

// NewWhitelist is a utility function to generate a whitelist to use with
// ValidateKeys.
//
// It accepts either/or strings and Whitefield arguments, transforming string
// arguments into WhiteField{name, nil}
func NewWhitelist(fields ...interface{}) []WhiteField {
	result := make([]WhiteField, len(fields))
	for i, field := range fields {
		switch f := field.(type) {
		case string:
			result[i] = WhiteField{f, nil}
		case WhiteField:
			result[i] = f
		default:
			panic(fmt.Sprintf("Unexpected type passed to newWhitelist - must be string or WhiteField: %T", field))
		}
	}
	return result
}

// Decode a JSON string, validating its keys against a whitelist before loading it into the
// supplied object.
func DecodeAndValidateJson(data []byte, whitelist []WhiteField, v interface{}) (valid map[string]interface{}, err JsonError) {
	if valid, err = ValidateKeys(data, whitelist); err != nil {
		return valid, err
	}
	if err := json.Unmarshal(data, v); err != nil {
		return valid, parseError(err)
	}
	return valid, nil
}
