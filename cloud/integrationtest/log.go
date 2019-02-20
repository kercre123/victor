package main

import (
	"fmt"
	"time"

	"github.com/anki/sai-go-util/log"
)

func logError(err error, taskID int, requestor, action string, extraFields map[string]interface{}, format string, a ...interface{}) {
	errorLog := alog.Error{
		"task_id":   taskID,
		"action":    action,
		"requestor": requestor,
		"status":    "error",
		"error":     err,
		"message":   fmt.Sprintf(format, a...),
	}

	for extraKey, extraValue := range extraFields {
		errorLog[extraKey] = extraValue
	}

	errorLog.Log()
}

func logInfo(taskID int, requestor, action string, extraFields map[string]interface{}, format string, a ...interface{}) {
	infoLog := alog.Info{
		"task_id":   taskID,
		"action":    action,
		"requestor": requestor,
		"status":    "ok",
		"message":   fmt.Sprintf(format, a...),
	}

	for extraKey, extraValue := range extraFields {
		infoLog[extraKey] = extraValue
	}

	infoLog.Log()
}

func logInfoIfNoError(err error, taskID int, requestor, action, subAction, format string, a ...interface{}) {
	extraFields := map[string]interface{}{"sub_action": subAction}
	if err != nil {
		logError(err, taskID, requestor, action, extraFields, format, a...)
	} else {
		logInfo(taskID, requestor, action, extraFields, format, a...)
	}
}

func logDebugIfNoError(err error, taskID int, requestor, action, subAction, format string, a ...interface{}) {
	if err != nil {
		logError(err, taskID, requestor, action, map[string]interface{}{"sub_action": subAction}, format, a...)
	} else {
		alog.Debug{
			"task_id":    taskID,
			"action":     action,
			"sub_action": subAction,
			"requestor":  requestor,
			"status":     "ok",
			"message":    fmt.Sprintf(format, a...),
		}.Log()
	}
}

func logLatency(err error, taskID int, requestor, action string, latency time.Duration) {
	extraFields := map[string]interface{}{"latency": latency.Nanoseconds(), "latency_str": latency}
	if err != nil {
		logError(err, taskID, requestor, action, extraFields, "")
	} else {
		logInfo(taskID, requestor, action, extraFields, "")
	}
}
