// The debug package provides a few print routines that are no-ops unless
// the "debug" build flag is provided at build time.

// +build debug

package debug

import (
	"fmt"

	"github.com/kr/pretty"
)

func Printf(format string, a ...interface{}) (n int, err error) {
	return fmt.Printf(format, a...)
}

func Println(a ...interface{}) (n int, err error) {
	return fmt.Print(a...)
}

func Prettyf(format string, a ...interface{}) (n int, err error) {
	return pretty.Printf(format, a...)
}

func Prettyln(a ...interface{}) (n int, err error) {
	return pretty.Print(a...)
}
