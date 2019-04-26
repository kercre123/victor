package validate

import (
	"database/sql"
	"reflect"
	"regexp"
	"sort"
	"strings"
	"testing"
)

var (
	nilString    *string
	nonNilString = "foobar"
)

var validatorTests = []struct {
	validator     FieldValidator
	value         interface{}
	expectedError *FieldError
}{
	//{LengthBetween{1, 2}, "ok", nil},
	{LengthBetween{1, 2}, sql.NullString{String: "ok", Valid: true}, nil},
	{LengthBetween{0, 2}, sql.NullString{String: "", Valid: false}, nil},
	{LengthBetween{1, 2}, sql.NullString{String: "", Valid: false}, &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{0, 2}, nilString, nil},
	{LengthBetween{1, 2}, nilString, &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{1, 3}, "toolong", &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{10, 100}, "tooshort", &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{1, 30}, []byte("okbytes"), nil},
	{LengthBetween{1, 3}, []byte("longbytes"), &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{1, 30}, []int{1, 2}, nil},
	{LengthBetween{1, 3}, []int{1, 2, 3, 4, 5}, &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{LengthBetween{1, 30}, []rune{1, 2}, nil},
	{LengthBetween{1, 3}, []rune{1, 2, 3, 4, 5}, &FieldError{FieldName: "field_name", Code: "bad_field_length", Message: "Invalid field length"}},
	{CharCountBetween{1, 2}, "ok", nil},
	{CharCountBetween{0, 2}, nilString, nil},
	{CharCountBetween{1, 2}, nilString, &FieldError{FieldName: "field_name", Code: "bad_char_count", Message: "Invalid character count"}},
	{CharCountBetween{1, 2}, []byte("ok"), nil},
	{CharCountBetween{1, 9}, "Hello, \u4e16\u754c", nil},                                                                                              // 13 bytes, 9 characters
	{CharCountBetween{1, 8}, "Hello, \u4e16\u754c", &FieldError{FieldName: "field_name", Code: "bad_char_count", Message: "Invalid character count"}}, // 13 bytes, 9 characters
	{IsNil{}, nilString, nil},
	{IsNil{}, &nonNilString, &FieldError{FieldName: "field_name", Code: "is_not_nil", Message: "Field is not nil"}},
	{IsNotNil{}, &nonNilString, nil},
	{IsNotNil{}, nilString, &FieldError{FieldName: "field_name", Code: "is_nil", Message: "Field is nil"}},
	{OneOfString{[]string{"a", "b", "c"}}, "b", nil},
	{OneOfString{[]string{"a", "b", "c"}}, "d", &FieldError{FieldName: "field_name", Code: "not_one_of", Message: "Field is not set to a valid value"}},
	{OneOf{[]interface{}{"a", 2, true}}, 2, nil},
	{OneOf{[]interface{}{"a", 2, true}}, "d", &FieldError{FieldName: "field_name", Code: "not_one_of", Message: "Field is not set to a valid value"}},
	{Email{}, "a@b", nil},
	{Email{}, "a@b@c", nil},
	{Email{}, "b@", &FieldError{FieldName: "field_name", Code: InvalidEmailAddr, Message: "Email address is invalid"}},
	{Email{}, "bob@", &FieldError{FieldName: "field_name", Code: InvalidEmailAddr, Message: "Email address is invalid"}},
	{Email{}, "utf8-ç@b", &FieldError{FieldName: "field_name", Code: InvalidEmailAddr, Message: "Email address contains non ASCII characters"}},
	{Email{}, "utf8domain@ç", &FieldError{FieldName: "field_name", Code: InvalidEmailAddr, Message: "Email address contains non ASCII characters"}},
	{Email{AllowUTF8Domain: true}, "utf8domain@ç", nil},
	{MatchRegexp{Regexp: regexp.MustCompile(`match\d`)}, "match5", nil},
	{MatchRegexp{Regexp: regexp.MustCompile(`match\d`)}, "matchz", &FieldError{FieldName: "field_name", Code: NoRegexpMatch, Message: "Field did not match supplied pattern"}},
	{IntBetween{Min: 10, Max: 20}, 15, nil},
	{IntBetween{Min: 10, Max: 20}, 20, nil},
	{IntBetween{Min: 10, Max: 20}, 10, nil},
	{IntBetween{Min: 10, Max: 20}, 21, &FieldError{FieldName: "field_name", Code: NotIntBetween, Message: "Field value out of range"}},
	{IntBetween{Min: 10, Max: 20}, "foo", &FieldError{FieldName: "field_name", Code: InvalidType, Message: "Field was not an int"}},
	{StringBetween{Min: "aab", Max: "aaf"}, "aab", nil},
	{StringBetween{Min: "aab", Max: "aaf"}, "aac", nil},
	{StringBetween{Min: "aab", Max: "aaf"}, "aaf", nil},
	{StringBetween{Min: "aab", Max: "aaf"}, "aaa", &FieldError{FieldName: "field_name", Code: NotStringBetween, Message: "Field value out of range"}},
	{StringBetween{Min: "aab", Max: "aaf"}, "aag", &FieldError{FieldName: "field_name", Code: NotStringBetween, Message: "Field value out of range"}},
	{StringBetween{Min: "aab", Max: "aaf"}, "", &FieldError{FieldName: "field_name", Code: NotStringBetween, Message: "Field value out of range"}},
	{StringBetween{Min: "aab", Max: "aaf"}, nilString, &FieldError{FieldName: "field_name", Code: NotStringBetween, Message: "Value is nil"}},
}

func TestValidators(t *testing.T) {
	for _, tst := range validatorTests {
		var resultErr *FieldError = nil
		result := tst.validator.Validate("field_name", tst.value)
		if len(result) > 0 {
			resultErr = result[0]
		}
		if len(result) > 1 {
			t.Errorf("Validator %v returned more than one error", tst.validator)
		}
		if resultErr != nil && resultErr.Value == nil {
			t.Errorf("Validator %#v set incorect Value field expectedValue=%#v actual=%#v", tst.validator, tst.value, resultErr.Value)
		}
		// nil out result value for easier comparison
		if resultErr != nil {
			resultErr.Value = nil
		}
		if (tst.expectedError == nil && resultErr != nil) || (tst.expectedError != nil && (resultErr == nil || *tst.expectedError != *resultErr)) {
			t.Errorf("Validator %#v failed for value=%#v expectedError=%#v actualError=%#v", tst.validator, tst.value, tst.expectedError, resultErr)
		}
	}
}

var panicTests = []struct {
	validator     FieldValidator
	value         interface{}
	expectedPanic string
}{
	{LengthBetween{1, 2}, struct{}{}, "Invalid field type struct {} passed to LengthBetween"},
	{CharCountBetween{1, 2}, struct{}{}, "Invalid field type struct {} passed to CharCountBetween"},
}

func TestValidatorPanics(t *testing.T) {
	var pstr string
	var result []*FieldError
	for _, tst := range panicTests {
		pstr = ""
		func() {
			defer func() {
				if r := recover(); r != nil {
					pstr = r.(string)
				}
			}()
			result = tst.validator.Validate("field_name", tst.value)
		}()
		if pstr != tst.expectedPanic {
			t.Errorf("Validator %v failed to panic for value=%#v got result=%#v panic=%#v instead", tst.validator, tst.value, result, pstr)
		}
	}

}

func TestNoError(t *testing.T) {
	v := New()
	v.Field("field1", "ok", LengthBetween{1, 10})
	result := v.Validate()
	if len(result) != 0 {
		t.Errorf("Unexpected error result from Validate: %#v", result)
	}
}

func TestSingleField(t *testing.T) {
	v := New()
	val := "toolong"
	// Run three validators; first and last should fail.
	v.Field("field1", &val, LengthBetween{1, 3}, CharCountBetween{1, 100}, IsNil{})
	result := v.Validate()
	if result.Error() != "1 fields with errors: field1:bad_field_length;is_not_nil" {
		t.Errorf("Validate returned wrong error message %q", result)
	}
	// type FieldErrors map[string][]FieldError
	if len(result) != 1 {
		t.Errorf("Unexpected number of fields in result set count=%d", len(result))
	}
	expectedErrors := []*FieldError{
		{FieldName: "field1", Code: "bad_field_length", Message: "Invalid field length", Value: &val},
		{FieldName: "field1", Code: "is_not_nil", Message: "Field is not nil", Value: &val},
	}
	if !reflect.DeepEqual(result["field1"], expectedErrors) {
		t.Errorf("Did not receive expected error set for field1: %#v", result["field1"])
	}
}

func TestMultipleFields(t *testing.T) {
	v := New()
	val := "toolong"
	// Test three fields.  field1 and field3 have errors
	v.Field("field1", &val, LengthBetween{1, 3})
	v.Field("field2", &val, CharCountBetween{1, 100})
	v.Field("field3", &val, IsNil{})
	result := v.Validate()
	if len(result) != 2 {
		t.Errorf("Unexpected number of fields in result set count=%d", len(result))
	}
	expectedErrors := []*FieldError{
		{FieldName: "field1", Code: "bad_field_length", Message: "Invalid field length", Value: &val},
	}
	if !reflect.DeepEqual(result["field1"], expectedErrors) {
		t.Errorf("Did not receive expected error set for field1: %#v", result["field1"])
	}
	expectedErrors = []*FieldError{
		{FieldName: "field3", Code: "is_not_nil", Message: "Field is not nil", Value: &val},
	}
	if !reflect.DeepEqual(result["field3"], expectedErrors) {
		t.Errorf("Did not receive expected error set for field3: %#v", result["field3"])
	}

}

func TestMap(t *testing.T) {
	v := New()
	value := map[string]interface{}{
		"zone": "string",
		"two":  2,
		"three": map[string]interface{}{
			"four": "test@example.com",
			"five": map[string]interface{}{
				"six": "final string",
			},
		},
	}

	validator := Map{
		"one": LengthBetween{1, 10}, // not set in value
		"two": IntBetween{1, 3},
		"three": Map{
			"four": []FieldValidator{Email{}, LengthBetween{1, 5}}, // three.four length fail
			"five": Map{
				"six": LengthBetween{1, 2}, // three.five.six fail
			},
		},
	}

	expectedErrors := FieldErrors{
		"one": []*FieldError{
			{FieldName: "one", Code: "map_key_not_set", Message: "Map key not set"},
		},
		"three.four": []*FieldError{
			{FieldName: "three.four", Code: "bad_field_length", Message: "Invalid field length", Value: "test@example.com"},
		},
		"three.five.six": []*FieldError{
			{FieldName: "three.five.six", Code: "bad_field_length", Message: "Invalid field length", Value: "final string"},
		},
	}

	v.Field("", value, validator)
	result := v.Validate()

	if !reflect.DeepEqual(result, expectedErrors) {
		t.Errorf("Did not received expected error set: %#v", result.ErrorsByField())
	}
}

func TestSlice(t *testing.T) {
	v := New()
	value := []string{"short", "too long", "short2"}

	validator := Slice{
		LengthBetween{1, 6},
	}

	expectedErrors := FieldErrors{
		"f.2": []*FieldError{{FieldName: "f.2", Code: "bad_field_length", Message: "Invalid field length", Value: "too long"}},
	}

	v.Field("f", value, validator)
	result := v.Validate()

	if !reflect.DeepEqual(result, expectedErrors) {
		t.Errorf("Did not received expected error set: %#v", result)
	}
}

var matchTests = []struct {
	val           string
	expectedCodes []string
}{
	{"ok-due-to-length", nil},                      // doesn't match one of
	{"oneof", nil},                                 // doesn't meet length criteria
	{"tooshort", []string{NotOneOf, BadCharCount}}, // doesn't meet either
}

func TestMatchAny(t *testing.T) {
	for _, test := range matchTests {
		v := New()
		v.Field("f", test.val, &MatchAny{
			OneOfString{[]string{"oneof"}},
			CharCountBetween{10, 50},
		})
		errs := v.Validate()
		if test.expectedCodes == nil {
			if errs != nil {
				t.Errorf("val=%q got errors %#v", test.val, errs)
			}
		} else {
			codes := strings.Split(errs["f"][0].Message, ",")
			sort.Strings(codes)
			sort.Strings(test.expectedCodes)
			if !reflect.DeepEqual(codes, test.expectedCodes) {
				t.Errorf("val=%q expected=%#v actual=%#v",
					test.val, test.expectedCodes, codes)
			}
		}
	}
}
