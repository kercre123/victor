// +build vicos

package log

import "fmt"

/*
#cgo LDFLAGS: -llog
#include <android/log.h>

// Go cannot call variadic C functions directly
static int android_log(int prio, const char* tag, const char* str) {
	return __android_log_print(prio, tag, "%s", str);
}
*/
import "C"

func Println(a ...interface{}) (int, error) {
	str := fmt.Sprintln(a...)
	ret := C.android_log(C.ANDROID_LOG_INFO, C.CString(Tag), C.CString(str))
	return int(ret), nil
}

func Printf(format string, a ...interface{}) (int, error) {
	str := fmt.Sprintf(format, a...)
	ret := C.android_log(C.ANDROID_LOG_INFO, C.CString(Tag), C.CString(str))
	return int(ret), nil
}

func init() {
	logVicos = Println
}
