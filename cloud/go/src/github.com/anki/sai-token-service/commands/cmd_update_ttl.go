package commands

import (
	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/db"
	"github.com/jawher/mow.cli"
)

// TODO: reduce boilerplate between this command and CreateTable()

func UpdateTTL() svc.ServiceConfig {
	return svc.WithCommand("update-ttl", "Update the DynamoDB tables to set the TTL", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager("dynamodb")
		awsconfigs.CommandInitialize(cmd)
		cmd.Spec = awsconfigs.CommandSpec() + " " +
			// TODO (refactor): this is duplicated from service.go
			"--token-service-dynamo-prefix"
		prefix := svc.StringOpt(cmd, "token-service-dynamo-prefix", "", "Prefix for DynamoDB table names")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "update-ttl",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting AWS config",
				}.Log()
				svc.Exit(-1)
			}

			// Initialize the token store
			store, err := db.NewDynamoTokenStore(*prefix, cfg)
			if err != nil {
				alog.Error{
					"action": "update-ttl",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Create the tables
			err = store.UpdateTTL()
			if err != nil {
				alog.Error{
					"action": "update-ttl",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error updating dynamo tables TTL",
				}.Log()
				svc.Exit(-3)
			}

			alog.Info{
				"action": "update-ttl",
				"status": alog.StatusOK,
				"prefix": *prefix,
			}.Log()

		}
	})
}
