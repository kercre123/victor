package robot

/*

#include "crash_reporter.h"

*/
import "C"

func InstallCrashReporter(procname string) {
	C.InstallCrashReporter(C.CString(procname))
}

func UninstallCrashReporter() {
	C.UninstallCrashReporter()
}
