// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package provides validators for a variety of types and can applied to
// members of a struct.
package validate

import (
	"database/sql"
	"fmt"
	"net/http"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"github.com/anki/sai-go-util/jsonutil"
)

const (
	NotEqual         = "field_not_equal"
	BadFieldLength   = "bad_field_length"
	BadCharCount     = "bad_char_count"
	FieldIsNil       = "is_nil"
	FieldNotNil      = "is_not_nil"
	NotOneOf         = "not_one_of"
	InvalidEmailAddr = "invalid_email_address"
	FieldErrorsCode  = "field_errors"
	InvalidType      = "invalid_field_type"
	NotIntBetween    = "not_int_between"
	NotStringBetween = "not_string_between"
	MapKeySet        = "map_key_is_set"
	MapKeyNotSet     = "map_key_not_set"
	TimeParseError   = "time_parse_error"
	TimeNotAfter     = "time_not_after"
	TimeNotBefore    = "time_not_before"
	NotMatchAny      = "not_match_any"
	NoRegexpMatch    = "pattern_mismatch"
)

// A FieldErorr represents one error with one field.
type FieldError struct {
	FieldName string      `json:"field"`
	Code      string      `json:"code"`
	Message   string      `json:"message"`
	Value     interface{} `json:"-"`
}

func (fe FieldError) Error() string {
	return fe.Message
}

// TODO add JsonError method

// FieldErrors hold a slice of errors for each field name.
type FieldErrors map[string][]*FieldError

func (fe FieldErrors) Error() string {
	fieldNames := make([]string, 0, len(fe))
	for k, _ := range fe {
		fieldNames = append(fieldNames, k)
	}
	//return fmt.Sprintf("%d fields with errors: %s", len(fe), strings.Join(fieldNames, ", "))
	return fmt.Sprintf("%d fields with errors: %s", len(fe), fe.ErrorsByField())
}

func (fe FieldErrors) ErrorsByField() string {
	// field1:err1;err2, field2:err1;err2
	var buf []byte
	var j int
	for fieldName, errs := range fe {
		if j > 0 {
			buf = append(buf, ' ')
		}
		buf = append(buf, []byte(fieldName)...)
		buf = append(buf, ':')
		for i, err := range errs {
			if i > 0 {
				buf = append(buf, ';')
			}
			buf = append(buf, []byte(err.Code)...)
		}
		j++
	}
	return string(buf)
}

func (fe FieldErrors) JsonError() jsonutil.JsonErrorResponse {
	return jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       FieldErrorsCode,
		Message:    fe.Error(),
		Metadata: map[string]interface{}{
			"bad_fields": fe,
		},
	}
}

// The FieldValidator allows for implementors to be used as a validator with the Field method.
type FieldValidator interface {
	Validate(fieldName string, value interface{}) []*FieldError
}

// A Validator runs tests against a number of fields.
type Validator struct {
	FieldErrors FieldErrors
	errorCount  int
}

// New returns a new Validator.
func New() *Validator {
	return &Validator{
		FieldErrors: make(FieldErrors),
		errorCount:  0,
	}
}

// Validate returns all FieldErrors that have been discovered so far.
func (v *Validator) Validate() FieldErrors {
	if v.errorCount == 0 {
		return nil
	}
	return v.FieldErrors
}

// Field applies one or more validators to a single field.
func (v *Validator) Field(fieldName string, value interface{}, validators ...FieldValidator) {
	for _, fv := range validators {
		if ferrs := fv.Validate(fieldName, value); len(ferrs) != 0 {
			v.errorCount += len(ferrs)
			for _, ferr := range ferrs {
				v.FieldErrors[ferr.FieldName] = append(v.FieldErrors[ferr.FieldName], ferr)
			}
		}
	}
}

// convert null types to their pointer equivalent
func normalizeSqlTypes(in interface{}) (result interface{}) {
	switch v := in.(type) {
	case sql.NullString:
		if v.Valid {
			return v.String
		}
		var r *string
		return r
	case sql.NullBool:
		if v.Valid {
			return v.Bool
		}
		var r *bool
		return r
	case sql.NullInt64:
		if v.Valid {
			return v.Int64
		}
		var r *int64
		return r
	case sql.NullFloat64:
		if v.Valid {
			return v.Float64
		}
		var r *float64
		return r
	}
	return in
}

type Equal struct {
	Val interface{}
}

func (v Equal) Validate(fieldName string, value interface{}) []*FieldError {
	if v.Val != value {
		return []*FieldError{{FieldName: fieldName, Code: NotEqual, Message: "Field does not equal supplied value", Value: value}}
	}
	return nil
}

// LengthBetween validates that the length of the given value is between a given range.
//
// The value must be a string, *string, []byte, []int or []rune else the validator will panic.
//
// Notes:
// For strings this measures the number of bytes in the string, not the number of characters.
// nil pointers are counted as a zero length string; use the IsNotNil validator if nil should be invalid.
type LengthBetween struct {
	Min int
	Max int
}

func (v LengthBetween) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	var length int
	switch v := value.(type) {
	case string:
		length = len(v)
	case []byte:
		length = len(v)
	case []int:
		length = len(v)
	case []int64:
		length = len(v)
	case []float64:
		length = len(v)
	case []rune:
		length = len(v)
	case *string:
		if v != nil {
			length = len(*v)
		}
	default:
		panic(fmt.Sprintf("Invalid field type %T passed to LengthBetween", v))
	}
	if length < v.Min || length > v.Max {
		return []*FieldError{{FieldName: fieldName, Code: BadFieldLength, Message: "Invalid field length", Value: value}}
	}
	return nil
}

// CharCountBetween validates that a utf8 string has a character count within a given range.
//
// the value must be either a string, *string or a []byte, else the validator will panic.
// NOTE: nil pointers are counted as a zero length string; use the IsNotNil validator if nil should be invalid.
type CharCountBetween struct {
	Min int
	Max int
}

func toStringPtr(caller string, value interface{}) (str *string) {
	switch v := value.(type) {
	case string:
		str = &v
	case *string:
		str = v
	case []byte:
		s := string(v)
		str = &s
	default:
		panic(fmt.Sprintf("Invalid field type %T passed to %s", v, caller))
	}
	return str
}

func (v CharCountBetween) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	var str []byte
	switch v := value.(type) {
	case string:
		str = []byte(v)
	case *string:
		if v != nil { // nil strings are counted as zero length
			str = []byte(*v)
		}
	case []byte:
		str = v
	default:
		panic(fmt.Sprintf("Invalid field type %T passed to CharCountBetween", v))
	}

	length := utf8.RuneCount(str)
	if length < v.Min || length > v.Max {
		return []*FieldError{{FieldName: fieldName, Code: BadCharCount, Message: "Invalid character count", Value: value}}
	}
	return nil
}

// IsNotNil validates that a field is not set to nil.
//
// the field *must* be a pointer, chan, func, interface, map, or slice value else the validator will panic.
type IsNotNil struct{}

func (v IsNotNil) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	if reflect.ValueOf(value).IsNil() {
		return []*FieldError{{FieldName: fieldName, Code: FieldIsNil, Message: "Field is nil", Value: value}}
	}
	return nil
}

// IsNil validates that a field is set to nil.
//
// the field *must* be a pointer, chan, func, interface, map, or slice value else the validator will panic.
type IsNil struct{}

func (v IsNil) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	if !reflect.ValueOf(value).IsNil() {
		return []*FieldError{{FieldName: fieldName, Code: FieldNotNil, Message: "Field is not nil", Value: value}}
	}
	return nil
}

type OneOf struct {
	Options []interface{}
}

func (v OneOf) Validate(fieldName string, value interface{}) []*FieldError {
	for _, okval := range v.Options {
		if okval == value {
			return nil
		}
	}
	return []*FieldError{{FieldName: fieldName, Code: NotOneOf, Message: "Field is not set to a valid value", Value: value}}
}

// OneOfString validates that a field matches one of a set of strings.
type OneOfString struct {
	Options []string
}

func (v OneOfString) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	strval := toStringPtr("OneOfString", value)
	for _, okval := range v.Options {
		//if reflect.DeepEqual(okval, value) {
		if okval == *strval {
			return nil
		}
	}
	return []*FieldError{{FieldName: fieldName, Code: NotOneOf, Message: "Field is not set to a valid value", Value: value}}
}

// Verify an email address.
//
// Expects a string or *string for input.
// This does a crude verification.  It allows any ASCII character for the local portion
// of the address, and optionally allows UTF8 characters for the domain portion.
type Email struct {
	AllowUTF8Domain bool
}

func (v Email) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)

	addr := toStringPtr("Email", value)
	if addr == nil {
		return []*FieldError{{FieldName: fieldName, Code: InvalidEmailAddr, Message: "Email address is empty", Value: value}}
	}
	idx := strings.LastIndexByte(*addr, '@')
	// must have an @ and at least one character before and after it
	if idx < 1 || idx == len(*addr)-1 {
		return []*FieldError{{FieldName: fieldName, Code: InvalidEmailAddr, Message: "Email address is invalid", Value: value}}
	}
	local, domain := (*addr)[:idx], (*addr)[idx+1:]
	if !isAscii(local, false) {
		return []*FieldError{{FieldName: fieldName, Code: InvalidEmailAddr, Message: "Email address contains non ASCII characters", Value: value}}
	}
	if !v.AllowUTF8Domain && !isAscii(domain, false) {
		return []*FieldError{{FieldName: fieldName, Code: InvalidEmailAddr, Message: "Email address contains non ASCII characters", Value: value}}
	}
	return nil
}

type MatchRegexp struct {
	Regexp *regexp.Regexp
}

func (v MatchRegexp) Validate(fieldName string, value interface{}) []*FieldError {
	strval := toStringPtr("MatchRegexp", value)
	if !v.Regexp.MatchString(*strval) {
		return []*FieldError{{FieldName: fieldName, Code: NoRegexpMatch, Message: "Field did not match supplied pattern", Value: value}}
	}
	return nil
}

type IntBetween struct {
	Min int64
	Max int64
}

func (v IntBetween) Validate(fieldName string, value interface{}) []*FieldError {
	var val64 int64
	switch val := value.(type) {
	case int:
		val64 = int64(val)
	case int32:
		val64 = int64(val)
	case int64:
		val64 = int64(val)
	case uint:
		val64 = int64(val)
	case uint64:
		val64 = int64(val)
	default:
		return []*FieldError{{FieldName: fieldName, Code: InvalidType, Message: "Field was not an int", Value: value}}
	}
	if val64 < v.Min || val64 > v.Max {
		return []*FieldError{{FieldName: fieldName, Code: NotIntBetween, Message: "Field value out of range", Value: value}}
	}
	return nil
}

type StringBetween struct {
	Min string
	Max string
}

func (v StringBetween) Validate(fieldName string, value interface{}) []*FieldError {
	strval := toStringPtr("StringBetween", value)
	if strval == nil {
		return []*FieldError{{FieldName: fieldName, Code: NotStringBetween, Message: "Value is nil", Value: value}}
	}
	if *strval < v.Min || *strval > v.Max {
		return []*FieldError{{FieldName: fieldName, Code: NotStringBetween, Message: "Field value out of range", Value: value}}
	}
	return nil
}

type ParsedTimeAfter struct {
	Format string
	Time   time.Time
}

func (v ParsedTimeAfter) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	strval := toStringPtr("ParsedTimeAfter", value)
	t, err := time.Parse(v.Format, *strval)
	if err != nil {
		return []*FieldError{{FieldName: fieldName, Code: TimeParseError, Message: "Failed to parse time: " + err.Error(), Value: value}}
	}
	if t.Before(v.Time) {
		return []*FieldError{{FieldName: fieldName, Code: TimeNotAfter, Message: "Field time is before test value", Value: value}}
	}
	return nil
}

type ParsedTimeBetween struct {
	Format   string
	Earliest time.Time
	Latest   time.Time
}

func (v ParsedTimeBetween) Validate(fieldName string, value interface{}) []*FieldError {
	value = normalizeSqlTypes(value)
	strval := toStringPtr("ParsedTimeBetween", value)
	t, err := time.Parse(v.Format, *strval)
	if err != nil {
		return []*FieldError{{FieldName: fieldName, Code: TimeParseError, Message: "Failed to parse time: " + err.Error(), Value: value}}
	}
	if t.Before(v.Earliest) {
		return []*FieldError{{FieldName: fieldName, Code: TimeNotAfter, Message: "Field time is before test value", Value: value}}
	}
	if t.After(v.Latest) {
		return []*FieldError{{FieldName: fieldName, Code: TimeNotBefore, Message: "Field time is after test value", Value: value}}
	}
	return nil
}

type AggregateFieldValidator interface {
	ValidateAggregate(fieldName string, value interface{}) []*FieldError
}

type NotSet struct{}

func (fv NotSet) Validate(fieldName string, value interface{}) []*FieldError {
	return []*FieldError{{FieldName: fieldName, Code: MapKeySet, Message: "Map key is set", Value: value}}
}

var notSet = NotSet{}

// Map applies validators to keys within a map
type Map map[string]interface{}

func (mv Map) Validate(fieldName string, subject interface{}) (errors []*FieldError) {
	// assert value is a map type
	sbj := reflect.ValueOf(subject)
	sbjType := sbj.Type()
	if sbjType.Kind() != reflect.Map {
		return []*FieldError{{FieldName: fieldName, Code: InvalidType,
			Message: "Map validator expected map type, received " + sbjType.Kind().String()}}
	}
	ktype := sbjType.Key()
	if ktype.Kind() != reflect.String {
		return []*FieldError{{FieldName: fieldName, Code: InvalidType,
			Message: "Map validator expected map with string keys, received keys of type " + ktype.Kind().String()}}
	}
	// get validators for each key
	for k, fv := range mv {
		// fv may be one of:
		// a field validator
		// an aggregate field validator
		// a list of field validators
		// the MapKeyNotSet sentinal
		newFieldName := k
		if len(fieldName) > 0 {
			newFieldName = fieldName + "." + k
		}
		subjval := sbj.MapIndex(reflect.ValueOf(k))
		if !subjval.IsValid() {
			// field not set, unless the MapKeyNotSet sentinal is set, then generate an error
			if fv != notSet {
				errors = append(errors, &FieldError{FieldName: newFieldName, Code: MapKeyNotSet, Message: "Map key not set"})
			}
			continue
		}
		subjint := subjval.Interface()
		switch f := fv.(type) {
		case FieldValidator:
			if errs := f.Validate(newFieldName, subjint); len(errs) != 0 {
				errors = append(errors, errs...)
			}
		case []FieldValidator:
			for _, subfv := range f {
				if errs := subfv.Validate(newFieldName, subjint); len(errs) != 0 {
					errors = append(errors, errs...)
				}
			}
		default:
			panic(fmt.Sprintf("Map expects values of either FieldValidator or []FieldValidator, not %T", fv))
		}
	}
	return errors
	// iterate over keys and apply sub validators
}

// Slice applies validators to every element of a slice
type Slice []FieldValidator

func (sv Slice) Validate(fieldName string, subject interface{}) (errors []*FieldError) {
	// assert value is a slice type
	sbj := reflect.ValueOf(subject)
	sbjType := sbj.Type()
	if sbjType.Kind() != reflect.Slice {
		return []*FieldError{{FieldName: fieldName, Code: InvalidType,
			Message: "Slice validator expected slice type, received " + sbjType.Kind().String()}}
	}
	for i := 0; i < sbj.Len(); i++ {
		newFieldName := strconv.Itoa(i + 1)
		if fieldName != "" {
			newFieldName = fieldName + "." + newFieldName
		}
		el := sbj.Index(i).Interface()
		// apply each validator to the entry
		for _, fv := range sv {
			if errs := fv.Validate(newFieldName, el); len(errs) != 0 {
				errors = append(errors, errs...)
			}
		}
	}
	return errors
}

// MatchAny is a validator that succeeds if any of the validators passed to it succeeds
type MatchAny []FieldValidator

func (m MatchAny) Validate(fieldName string, value interface{}) (errors []*FieldError) {
	for _, v := range m {
		if errs := v.Validate(fieldName, value); errs != nil {
			errors = append(errors, errs...)
		} else {
			// validator completed without error; ignore any other errors
			return nil
		}
	}
	// Collect errors into a single error
	codes := make([]string, len(errors))
	for i, err := range errors {
		codes[i] = err.Code
	}
	return []*FieldError{{FieldName: fieldName, Code: NotMatchAny, Message: strings.Join(codes, ",")}}
}

func isAscii(s string, allowSpace bool) bool {
	var first, last byte = 0x20, 0x7e
	if !allowSpace {
		first = 0x21
	}
	for i := range s {
		if ch := s[i]; ch < first || ch > last {
			return false
		}
	}
	return true
}
