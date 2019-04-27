package grpcsvc

import (
	"fmt"
	"os"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/svc"
	"google.golang.org/grpc/grpclog"
)

var (
	registry      = metricsutil.NewRegistry("grpc.grpclog")
	countWarnings = metricsutil.GetCounter("warnings", registry)
	countErrors   = metricsutil.GetCounter("errors", registry)
)

// LogGrpcToAnkiLog sets up gRPC to log to the Anki log subsystem,
// which will then go to stdout or splnk as per the Anki log
// configuration.
func LogGrpcToAnkiLog() {
	grpclog.SetLoggerV2(&AnkiGrpcLogger{})
}

// LogGrpcToStdout sets up gRPC to log to stdout. This should
// generally only be done for testing.
func LogGrpcToStdout() {
	grpclog.SetLoggerV2(grpclog.NewLoggerV2(os.Stdout, os.Stdout, os.Stderr))
}

// AnkiGrpcLogger is an implementation of the grpclog.LoggerV2 interface
// that redirects gRPC's internal logging to Anki's log subsystem, for
// outputting to Splunk via kinesis.
type AnkiGrpcLogger struct{}

func (l *AnkiGrpcLogger) logInfo(msg string) {
	alog.Info{
		"action":    "grpclog",
		"grpclevel": "INFO",
		"msg":       msg,
	}.Log()
}

func (l *AnkiGrpcLogger) logWarning(msg string) {
	countWarnings.Inc(1)
	alog.Warn{
		"action":    "grpclog",
		"grpclevel": "WARN",
		"msg":       msg,
	}.Log()
}

func (l *AnkiGrpcLogger) logError(msg string) {
	countErrors.Inc(1)
	alog.Error{
		"action":    "grpclog",
		"grpclevel": "ERROR",
		"msg":       msg,
	}.Log()
}

func (l *AnkiGrpcLogger) logFatal(msg string) {
	alog.Error{
		"action":    "grpclog",
		"grpclevel": "FATAL",
		"msg":       msg,
	}.Log()
	svc.Fail("GRPC FATAL: %s", msg)
}

func (l *AnkiGrpcLogger) Info(args ...interface{}) {
	l.logInfo(fmt.Sprint(args...))
}

func (l *AnkiGrpcLogger) Infof(format string, args ...interface{}) {
	l.logInfo(fmt.Sprintf(format, args...))
}

func (l *AnkiGrpcLogger) Infoln(args ...interface{}) {
	l.logInfo(fmt.Sprintln(args...))
}

func (l *AnkiGrpcLogger) Warning(args ...interface{}) {
	l.logWarning(fmt.Sprint(args...))
}

func (l *AnkiGrpcLogger) Warningf(format string, args ...interface{}) {
	l.logWarning(fmt.Sprintf(format, args...))
}

func (l *AnkiGrpcLogger) Warningln(args ...interface{}) {
	l.logWarning(fmt.Sprintln(args...))
}

func (l *AnkiGrpcLogger) Error(args ...interface{}) {
	l.logError(fmt.Sprint(args...))
}

func (l *AnkiGrpcLogger) Errorf(format string, args ...interface{}) {
	l.logError(fmt.Sprintf(format, args...))
}

func (l *AnkiGrpcLogger) Errorln(args ...interface{}) {
	l.logError(fmt.Sprintln(args...))
}

func (l *AnkiGrpcLogger) Fatal(args ...interface{}) {
	l.logFatal(fmt.Sprint(args...))
}

func (l *AnkiGrpcLogger) Fatalf(format string, args ...interface{}) {
	l.logFatal(fmt.Sprintf(format, args...))
}

func (l *AnkiGrpcLogger) Fatalln(args ...interface{}) {
	l.logFatal(fmt.Sprintln(args...))
}

func (l *AnkiGrpcLogger) V(level int) bool {
	return true
}
