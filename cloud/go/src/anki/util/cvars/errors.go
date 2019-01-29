package cvars

import "errors"

var NotExist = errors.New("var by given name does not exist")
var WrongType = errors.New("var by given name does not match the given type")
