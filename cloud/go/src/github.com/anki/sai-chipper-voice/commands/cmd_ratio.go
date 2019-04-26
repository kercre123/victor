package commands

import (
	"context"

	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	"github.com/anki/sai-chipper-voice/requestrouter"
)

const (
	AddRatioCmdSpec = " " +
		// TODO: May or may not need this
		// Required.
		"--chipper-dynamo-prefix " +
		"--dialogflow-ratio " +
		"--lex-ratio " +
		"[--ms-ratio] " +
		"--created-by "

	GetRatioCmdSpec = " --chipper-dynamo-prefix [--num-ratio] "
)

func AddRatio() svc.ServiceConfig {
	return svc.WithCommand("add-ratio", "Add a new distribution ratio", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager(endpoints.DynamodbServiceID)
		awsconfigs.CommandInitialize(cmd)

		cmd.Spec = awsconfigs.CommandSpec() + AddRatioCmdSpec

		// Required
		prefix := svc.StringOpt(cmd, "chipper-dynamo-prefix", "", "Prefix for DynamoDB table names")
		creator := svc.StringOpt(cmd, "created-by", "", "Email of person/ system running this command")
		dfRatio := svc.IntOpt(cmd, "dialogflow-ratio", 0, "Dialogflow ratio")
		lexRatio := svc.IntOpt(cmd, "lex-ratio", 0, "Lex ratio")
		msRatio := svc.IntOpt(cmd, "ms-ratio", 0, "Microsoft ratio (optional)")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "add_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting AWS Dynamo config",
				}.Log()
				svc.Exit(-1)
			}

			// Initialize the routing store
			store, err := requestrouter.NewRouteStore(*prefix, cfg)
			if err != nil {
				alog.Error{
					"action": "add_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing Dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// create a Ratio
			ratio, err := requestrouter.NewRatio(*dfRatio, *lexRatio, *msRatio, *creator)
			if err != nil {
				alog.Error{
					"action": "add_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error creating ratio",
				}.Log()
				svc.Exit(-3)
			}

			// Add ratio to Dynamodb
			err = store.StoreRatio(context.Background(), ratio)
			if err != nil {
				alog.Error{
					"action": "add_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error saving ratio to dynamodb",
				}.Log()
				svc.Exit(-4)
			}

			// Print status

			alog.Info{
				"action":     "add_ratio",
				"status":     alog.StatusOK,
				"ratio":      ratio.Value,
				"created_by": *creator,
			}.Log()

		}
	})
}

func GetRatio() svc.ServiceConfig {
	return svc.WithCommand("get-ratios", "Get last N saved ratios from dynamodb", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager("dynamodb")
		awsconfigs.CommandInitialize(cmd)

		cmd.Spec = awsconfigs.CommandSpec() + GetRatioCmdSpec

		// Required
		prefix := svc.StringOpt(cmd, "chipper-dynamo-prefix", "", "Prefix for DynamoDB table names")
		numRatio := svc.IntOpt(cmd, "num-ratio", 1, "Number of ratios to get")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "get_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting AWS Dynamo config",
				}.Log()
				svc.Exit(-1)
			}

			// Initialize the routing store
			store, err := requestrouter.NewRouteStore(*prefix, cfg)
			if err != nil {
				alog.Error{
					"action": "get_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing Dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Get ratios to Dynamodb
			ratios, err := store.GetRecentRatios(context.Background(), *numRatio)
			if err != nil {
				alog.Error{
					"action": "get_ratio",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error getting ratios to dynamodb",
				}.Log()
				svc.Exit(-4)
			}

			// Print results
			for _, r := range ratios {
				alog.Info{
					"action": "get_ratio",
					"status": alog.StatusOK,
					"ratio":  r,
				}.Log()
			}
		}
	})
}
