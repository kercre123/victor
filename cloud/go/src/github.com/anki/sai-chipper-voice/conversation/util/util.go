package util

import (
	"errors"
	"reflect"
	"strconv"
)

func FlattenInterfaceToStringMap(keyPrefix string, input interface{}) (output map[string]string, err error) {
	output = map[string]string{}
	ref := reflect.ValueOf(input)
	switch ref.Kind() {
	case reflect.Bool:
		stringVersion := strconv.FormatBool(input.(bool))
		output[keyPrefix] = stringVersion
	case reflect.Int:
		stringVersion := strconv.FormatInt(int64(input.(int)), 10)
		output[keyPrefix] = stringVersion
	case reflect.Int8:
		stringVersion := strconv.FormatInt(int64(input.(int8)), 10)
		output[keyPrefix] = stringVersion
	case reflect.Int16:
		stringVersion := strconv.FormatInt(int64(input.(int16)), 10)
		output[keyPrefix] = stringVersion
	case reflect.Int32:
		stringVersion := strconv.FormatInt(int64(input.(int32)), 10)
		output[keyPrefix] = stringVersion
	case reflect.Int64:
		stringVersion := strconv.FormatInt(input.(int64), 10)
		output[keyPrefix] = stringVersion
	case reflect.Uint:
		stringVersion := strconv.FormatUint(uint64(input.(uint)), 0)
		output[keyPrefix] = stringVersion
	case reflect.Uint8:
		stringVersion := strconv.FormatUint(uint64(input.(uint8)), 8)
		output[keyPrefix] = stringVersion
	case reflect.Uint16:
		stringVersion := strconv.FormatUint(uint64(input.(uint16)), 16)
		output[keyPrefix] = stringVersion
	case reflect.Uint32:
		stringVersion := strconv.FormatUint(uint64(input.(uint32)), 32)
		output[keyPrefix] = stringVersion
	case reflect.Uint64:
		stringVersion := strconv.FormatUint(input.(uint64), 64)
		output[keyPrefix] = stringVersion
	case reflect.Float32:
		stringVersion := strconv.FormatFloat(float64(input.(float32)), 'f', -1, 32)
		output[keyPrefix] = stringVersion
	case reflect.Float64:
		stringVersion := strconv.FormatFloat(input.(float64), 'f', -1, 64)
		output[keyPrefix] = stringVersion
	case reflect.Map:
		for _, key := range ref.MapKeys() {
			keyMap, suberr := FlattenInterfaceToStringMap(" ", key.Interface())
			if suberr != nil {
				err = suberr
				return
			}
			// Get a string form of key, this is a hack
			keyString := " "
			for _, keyValue := range keyMap {
				keyString = keyValue
				break
			}
			value := ref.MapIndex(key).Interface()
			mapToMerge, suberr := FlattenInterfaceToStringMap(keyPrefix+"."+keyString, value)
			if suberr != nil {
				err = suberr
				return
			}
			for mergeKey, mergeValue := range mapToMerge {
				output[mergeKey] = mergeValue
			}
		}
	case reflect.Ptr:
		if ref.IsNil() {
			return
		}

		mapToMerge, suberr := FlattenInterfaceToStringMap(keyPrefix, ref.Elem().Interface())
		if suberr != nil {
			err = suberr
			return
		}
		for mergeKey, mergeValue := range mapToMerge {
			output[mergeKey] = mergeValue
		}
	case reflect.Slice:
		for index := 0; index < ref.Len(); index++ {
			item := ref.Index(index).Interface()
			mapToMerge, suberr := FlattenInterfaceToStringMap(keyPrefix+strconv.Itoa(index), item)
			if suberr != nil {
				err = suberr
				return
			}
			for mergeKey, mergeValue := range mapToMerge {
				output[mergeKey] = mergeValue
			}
		}
	case reflect.String:
		output[keyPrefix] = input.(string)
	case reflect.Struct:
		for index := 0; index < ref.NumField(); index++ {
			mapToMerge, suberr := FlattenInterfaceToStringMap(keyPrefix+"."+ref.Type().Field(index).Name, ref.Field(index).Interface())
			if suberr != nil {
				err = suberr
				return
			}
			for mergeKey, mergeValue := range mapToMerge {
				output[mergeKey] = mergeValue
			}
		}
	case reflect.Interface:
		output[keyPrefix] = input.(string)
	case reflect.Invalid:
		return
	default:
		err = errors.New("Trying to flatten unsupported type " + ref.Kind().String())
	}
	return
}
