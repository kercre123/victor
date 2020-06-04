package commands

import (
	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	"github.com/anki/sai-chipper-voice/requestrouter"
)

func CreateTables() svc.ServiceConfig {
	return svc.WithCommand("create-tables", "Create the DynamoDB tables", func(cmd *cli.Cmd) {
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
					"action": "create_dynamodb_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting AWS config",
				}.Log()
				svc.Exit(-1)
			}

			// Initialize routing store
			store, err := requestrouter.NewRouteStore(*prefix, cfg)
			if err != nil {
				alog.Error{
					"action": "create_dynamodb_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Create the tables
			err = store.CreateTables()
			if err != nil {
				alog.Error{
					"action": "create-create_dynamodb_tables",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error creating dynamo tables",
				}.Log()
				svc.Exit(-3)
			}

			alog.Info{
				"action": "create_dynamodb_tables",
				"status": alog.StatusOK,
				"prefix": *prefix,
			}.Log()

		}
	})
}
