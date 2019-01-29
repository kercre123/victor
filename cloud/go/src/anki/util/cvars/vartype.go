package cvars

type VarType uint8

const (
	Int VarType = iota
	String
	IntSet
)

func (v VarType) DataType() VarType {
	switch v {
	case IntSet:
		return Int
	}
	return v
}

func isTypeAcceptable(vtype VarType, desired VarType) bool {
	if vtype == desired {
		return true
	}

	if vtype.DataType() == desired {
		return true
	}

	return false
}
