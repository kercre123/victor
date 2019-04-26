package iniconf

import (
	"errors"
	"fmt"
	"reflect"
)

var (
	ErrInvalidType      = errors.New("target must be a pointer")
	ErrInvalidFieldType = errors.New("target must be a string, int or bool")
)

// Reader provides read access to a section
// It is implemented by both Iniconf and ConfChain
type reader interface {
	HasSection(sectionName string) bool
	EntryString(sectionName, entryName string) (string, error)
	EntryInt(sectionName, entryName string) (int64, error)
	EntryBool(sectionName, entryName string) (bool, error)
}

func readSection(source reader, section string, v interface{}) error {
	rpv := reflect.ValueOf(v)
	if rpv.Kind() != reflect.Ptr || rpv.IsNil() {
		return ErrInvalidType
	}
	rv := rpv.Elem()
	if rv.Kind() != reflect.Struct {
		return ErrInvalidType
	}

	return visitStruct(rv, true, func(tag string, fv reflect.Value, kind reflect.Kind) error {
		if !fv.CanSet() {
			return nil
		}
		if kind == reflect.Ptr {
			if fv.IsNil() {
				fv.Set(reflect.New(fv.Type().Elem()))
			}
			fv = fv.Elem()
			kind = fv.Kind()
		}

		switch kind {
		case reflect.String:
			ev, err := source.EntryString(section, tag)
			if err != nil {
				return nil
			}
			fv.SetString(ev)

		case reflect.Bool:
			ev, err := source.EntryBool(section, tag)
			if err != nil {
				return nil
			}
			fv.SetBool(ev)

		case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64,
			reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64, reflect.Uintptr:
			ev, err := source.EntryInt(section, tag)
			if err != nil {
				return nil
			}
			fv.SetInt(ev)

		case reflect.Interface:
			// TODO: guess types instead of assuming string
			ev, err := source.EntryString(section, tag)
			if err != nil {
				return nil
			}
			fv.Set(reflect.ValueOf(ev))

		default:
			return ErrInvalidFieldType
		}
		return nil
	})
}

func loadSection(c *IniConf, sectionName string, v interface{}) error {
	rv := reflect.ValueOf(v)
	if rv.Kind() == reflect.Ptr {
		rv = rv.Elem()
	}
	if rv.Kind() != reflect.Struct {
		return errors.New("WriteIntoSection value must be a struct or a struct pointer")
	}
	return visitStruct(rv, false, func(tag string, fv reflect.Value, kind reflect.Kind) error {
		if kind == reflect.Ptr || kind == reflect.Interface {
			if fv.IsNil() {
				return nil
			}
			fv = fv.Elem()
			kind = fv.Kind()
		}

		switch kind {
		case reflect.String, reflect.Bool,
			reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64,
			reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64, reflect.Uintptr:
			return c.SetEntry(sectionName, tag, fv.Interface())

		default:
			return ErrInvalidFieldType
		}
		return nil
	})
}

func visitStruct(rv reflect.Value, initNil bool, handler func(tag string, fv reflect.Value, kind reflect.Kind) error) error {
	if rv.Kind() != reflect.Struct {
		panic(fmt.Sprintf("expected struct, got %v", rv.Kind()))
	}
	typ := rv.Type()
	for i := 0; i < typ.NumField(); i++ {
		f := typ.Field(i)
		fv := rv.Field(i)
		if f.PkgPath != "" && !f.Anonymous {
			continue // skip unexported fields
		}

		kind := fv.Kind()
		if sv := structVal(fv, initNil); sv != nil {
			if err := visitStruct(*sv, initNil, handler); err != nil {
				return err
			}
			continue
		}

		tag := f.Tag.Get("iniconf")
		if tag == "" {
			continue
		}

		if err := handler(tag, fv, kind); err != nil {
			return err
		}
	}
	return nil
}

func structVal(rv reflect.Value, initNil bool) *reflect.Value {
	if rv.Kind() == reflect.Struct {
		return &rv
	}
	if rv.Kind() != reflect.Ptr {
		return nil
	}
	if rv.IsNil() {
		if initNil && rv.CanSet() {
			nv := reflect.New(rv.Type().Elem())
			rv.Set(nv)
			rv = nv.Elem()
		} else {
			return nil
		}
	} else {
		rv = rv.Elem()
	}
	if rv.Kind() == reflect.Struct {
		return &rv
	}
	return nil
}
