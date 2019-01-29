package cvars

import (
	"errors"
	"strconv"
)

func AddInt(name string, setter IntSetter, accessor Accessor) {
	vars[name] = &CVar{
		Int,
		setter,
		accessor,
		nil,
	}
}

func AddString(name string, setter StringSetter, accessor Accessor) {
	vars[name] = &CVar{
		String,
		setter,
		accessor,
		nil,
	}
}

func AddIntSet(name string, setter IntSetter, accessor Accessor, options []Option) {
	vars[name] = &CVar{
		IntSet,
		setter,
		accessor,
		append([]Option{}, options...), // don't store passed-in reference
	}
}

func SetInt(name string, value int) (interface{}, error) {
	cv, err := getVarForNameAndType(name, Int)
	if err != nil {
		return nil, err
	}

	return cv.setInt(value), nil
}

func SetString(name string, value string) (interface{}, error) {
	cv, err := getVarForNameAndType(name, String)
	if err != nil {
		return nil, err
	}

	return cv.setString(value), nil
}

func (cv *CVar) Set(value string) (interface{}, error) {
	switch cv.VType.DataType() {
	case Int:
		num, err := strconv.ParseInt(value, 10, 32)
		if err != nil {
			return nil, err
		}
		return cv.setInt(int(num)), nil
	case String:
		return cv.setString(value), nil
	}
	return nil, errors.New("unknown type")
}

func GetAllVars() map[string]CVar {
	ret := make(map[string]CVar)
	for k, v := range vars {
		ret[k] = *v
	}
	return ret
}
