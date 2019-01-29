package cvars

type CVar struct {
	VType    VarType
	setter   interface{}
	Accessor Accessor
	Options  []Option
}

type IntSetter func(int) interface{}
type StringSetter func(string) interface{}
type Accessor func() interface{}

type Option struct {
	Value string
	Title string
}
