package main

import (
	"fmt"

	"github.com/anki/sai-go-util/log"
)

func logIfNoError(err error, userName, action, format string, a ...interface{}) {
	if err != nil {
		alog.Error{
			"action":         action,
			"test_user_name": userName,
			"status":         "error",
			"error":          err,
		}.Log()
	} else {
		alog.Info{
			"action":         action,
			"test_user_name": userName,
			"status":         "ok",
			"message":        fmt.Sprintf(format, a...),
		}.Log()
	}
}
