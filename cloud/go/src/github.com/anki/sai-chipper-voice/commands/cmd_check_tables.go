package commands

import (
	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	"github.com/anki/sai-chipper-voice/requestrouter"
)

func CheckTables() svc.ServiceConfig {
	return svc.WithCommand("check-tables", "Check the DynamoDB tables", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager(endpoints.DynamodbServiceID)
		awsconfigs.CommandInitialize(cmd)
		cmd.Spec = awsconfigs.CommandSpec() + " " +
			// TODO (refactor): this is duplicated from service.go
			"--chipper-dynamo-prefix"

		prefix := svc.StringOpt(cmd, "chipper-dynamo-prefix", "", "Prefix for DynamoDB table names")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "check_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting AWS config",
				}.Log()
				svc.Exit(-1)
			}

			// Initialize the route store
			store, err := requestrouter.NewRouteStore(*prefix, cfg)
			if err != nil {
				alog.Error{
					"action": "check_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Create the tables
			err = store.CheckTables()
			if err != nil {
				alog.Error{
					"action": "check_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error checking dynamo tables",
				}.Log()
				svc.Exit(-3)
			}

			alog.Info{
				"action": "check_tables",
				"status": alog.StatusOK,
				"prefix": *prefix,
			}.Log()

		}
	})
}
