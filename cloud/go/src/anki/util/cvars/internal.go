package cvars

var vars = make(map[string]*CVar)

func getVarForNameAndType(name string, vtype VarType) (*CVar, error) {
	cv, ok := vars[name]
	if !ok {
		return nil, NotExist
	}
	if !isTypeAcceptable(cv.VType, vtype) {
		return nil, WrongType
	}

	return cv, nil
}

func (cv *CVar) setInt(value int) interface{} {
	return cv.setter.(IntSetter)(value)
}

func (cv *CVar) setString(value string) interface{} {
	return cv.setter.(StringSetter)(value)
}
