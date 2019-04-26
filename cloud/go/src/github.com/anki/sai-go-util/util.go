// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// util provides general utility functions.
package util

import (
	"fmt"
	"os"
	"reflect"
	"strings"
)

// SetFromEnv set the value of a target string with an the value of
// environment variable, if set.
//
// Note: This can't currently distinguish between a non-existant environment variable and one
// that just holds an empty string.
func SetFromEnv(target *string, envKey string) {
	if value := os.Getenv(envKey); value != "" {
		*target = value
	}
}

// UpdateStructDelta copies values from a source to target struct based on keys in a changeset.
//
// Key names in the changeSet are looked up against the json tag in the source/target struct
// If it finds a match the the value is copied from source to target.
//
// map and struct values are ignored (ie. it will not descend into sub structures, though
// it will copy slices and arrays)
//
// Struct fields without a json tag are ignored.
// TODO: Unit tests
func UpdateStructDelta(changeSet map[string]interface{}, source, target interface{}) (updated []string) {
	sourcev, targetv := reflect.ValueOf(source), reflect.ValueOf(target)
	vtype := sourcev.Type()
	if vtype != targetv.Type() {
		panic("source and target are different types!")
	}
	if sourcev.Kind() != reflect.Ptr || sourcev.Elem().Kind() != reflect.Struct {
		panic("source and target must be pointers to structs")
	}

	// deref pointer
	sourcev, targetv = sourcev.Elem(), targetv.Elem()

	// dereference pointer
	vtype = sourcev.Type()
	for i := 0; i < vtype.NumField(); i++ {
		f := vtype.Field(i)
		// get base type of field
		t := f.Type
		if t.Kind() == reflect.Ptr {
			t = t.Elem() // deref ptr
		}
		if jsonName := f.Tag.Get("json"); jsonName != "" {
			if _, ok := changeSet[jsonName]; ok {
				targetv.Field(i).Set(sourcev.Field(i))
				updated = append(updated, jsonName)
			}
		}
	}
	return updated
}

func StructFieldByName(s interface{}, keyname string) (interface{}, error) {
	v := reflect.ValueOf(s)
	if v.Kind() != reflect.Struct {
		return nil, fmt.Errorf("expected a struct, got %T", s)
	}
	val := v.FieldByName(keyname)
	if !val.IsValid() {
		return nil, fmt.Errorf("struct had no field named %q", keyname)
	}

	return val.Interface(), nil
}

// StructFieldByJsonField attempts to return a value from a struct by matching
// the name associated with its json tag
func StructFieldByJsonField(s interface{}, fieldname string) (interface{}, error) {
	v := reflect.ValueOf(s)
	if v.Kind() == reflect.Ptr {
		v = v.Elem()
	}
	if v.Kind() != reflect.Struct {
		return nil, fmt.Errorf("expected a struct, got %T", s)
	}

	typ := v.Type()

	for i := 0; i < typ.NumField(); i++ {
		if t := typ.Field(i).Tag.Get("json"); t != "" {
			tv := strings.SplitN(t, ",", 2)
			if tv[0] == fieldname {
				return v.Field(i).Interface(), nil
			}
		}
	}
	return nil, fmt.Errorf("Could not find json field with name %q in struct", fieldname)
}

// Must1 takes the error result from a one value function call and panics if it's not nil
func Must1(err error) {
	if err != nil {
		panic(err.Error())
	}
}

// Must2 takes the value and error from a two value function call and panics if the error is not nil.
//
// eg. func doSomething() (string, error) { ... }
// result := Must2(doSomething())
func Must2(v interface{}, err error) interface{} {
	if err != nil {
		panic(err.Error())
	}
	return v
}

// Must3 takes the values and error from a three return value function call and panics if the error is not nil.
//
// eg. func doSomething() (string, int, error) { ... }
// strval, intval := Must3(doSomething())
func Must3(v1 interface{}, v2 interface{}, err error) (interface{}, interface{}) {
	if err != nil {
		panic(err.Error())
	}
	return v1, v2
}
