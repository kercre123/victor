package commands

import (
	"io/ioutil"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/server/jdocs" // package jdocssvc
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
)

func CheckTables() svc.ServiceConfig {
	return svc.WithCommand("check-tables", "Validate Doc Specs and check that all DynamoDB tables exist", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager("dynamodb")
		awsconfigs.CommandInitialize(cmd)
		cmd.Spec = awsconfigs.CommandSpec() + " " +
			// TODO (refactor): this is duplicated from service.go
			"--jdocs-dynamo-prefix"
		tablePrefix := svc.StringOpt(cmd, "jdocs-dynamo-prefix", "", "Prefix for DynamoDB table names")
		docSpecsFile := svc.StringOpt(cmd, "doc-specs-file", "", "JSON-format file containing all document specs")

		jdocsServer := jdocssvc.NewServer( /*TODO*/ )

		cmd.Action = func() {
			if cfg, err := awsconfigs.Config("dynamodb").NewConfig(); err != nil {
				alog.Error{
					"action": "check-tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error configuring AWS",
				}.Log()
				svc.Exit(-1)
			} else if specsJson, rerr := ioutil.ReadFile(*docSpecsFile); rerr != nil {
				alog.Error{
					"action": "check-tables",
					"status": alog.StatusError,
					"error":  rerr,
					"msg":    "Error checking reading JSON documents specification file",
				}.Log()
				svc.Exit(-2)
			} else if derr := jdocsServer.InitAPI(string(specsJson), *tablePrefix, cfg); derr != nil {
				alog.Error{
					"action": "check-tables",
					"status": alog.StatusError,
					"error":  derr,
					"msg":    "Error checking initializing dynamo tables",
				}.Log()
				svc.Exit(-3)
			}

			// Check the tables
			err := jdocsServer.CheckTables()
			if err != nil {
				alog.Error{
					"action": "check-tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error checking dynamo tables",
				}.Log()
				svc.Exit(-3)
			}

			alog.Info{
				"action": "check-tables",
				"status": alog.StatusOK,
				"prefix": *tablePrefix,
			}.Log()
		}

	})
}
