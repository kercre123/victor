package svc

import (
	"fmt"
	"os"

	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

func Exit(exitCode int) {
	alog.Info{
		"action": "service_exit",
		"code":   exitCode,
	}.Log()
	cli.Exit(exitCode)
}

func Fail(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, format+"\n", a...)
	cli.Exit(100)
}

func ServiceMain(serviceName, serviceDescription string, config ...ServiceConfig) {
	if s, err := NewService(serviceName, serviceDescription, config...); err != nil {
		alog.Error{
			"action": "service_create",
			"status": alog.StatusError,
			"error":  err,
		}.Log()
		Exit(-1)
	} else {
		// Call into the old envconfig package because we have util
		// code that depend on it running (e.g. awsutil.GetConfig()).
		envconfig.DefaultConfig.Flags.Parse(os.Args)
		if err := s.Run(os.Args); err != nil {
			alog.Error{
				"action": "service_run",
				"status": alog.StatusError,
				"error":  err,
			}.Log()
			Exit(-2)
		}
	}
}
