// +build !linux

package log

import "fmt"

func Println(a ...interface{}) (int, error) {
	return fmt.Println(a...)
}

func Printf(format string, a ...interface{}) (int, error) {
	return fmt.Printf(format, a...)
}
