// +build !debug

package debug

func Printf(format string, a ...interface{}) (n int, err error) {
	return 0, nil
}

func Println(a ...interface{}) (n int, err error) {
	return 0, nil
}

func Prettyf(format string, a ...interface{}) (n int, err error) {
	return 0, nil
}

func Prettyln(a ...interface{}) (n int, err error) {
	return 0, nil
}
