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
	AddCmdSpec = " " +
		// TODO: May or may not need this
		"--chipper-dynamo-prefix " +

		// Required. Robot firmware version. e.g. "0.12"
		"--firmware-version " +

		// Required. Intent service to add. Valid choices: dialogflow, lex, ms
		"--intent-service " +

		// Required. Language. e.g. "en-us", "en-gb", "fr-fr"
		"--service-lang " +

		// Required. Robot-mode
		"--robot-mode " +

		// Optional
		// dialogflow project name. e.g. "victor-dev-en-us"
		"[--dialogflow-project-name] " +

		// dialogflow published environment name. e.g. "dialogflow-dev-0_12"
		"[--dialogflow-project-version] " +

		// secrets key associated with project name
		"[--dialogflow-creds-key] " +

		// lex bot name. e.g. victor-dev-en-us
		"[--lex-botname] " +

		// lex version name. e.g. lex_dev_a_bc (for version 0.12)
		"[--lex-bot-version] " +

		// enable this version, boolean
		"[--enable-version] "

	GetCmdSpec = " " +
		// Required
		"--chipper-dynamo-prefix " +

		// Required, service name
		"--service-name " +

		// Required, lang
		"--service-lang " +

		// Required. Robot-mode
		"--robot-mode " +

		// Required, sort key. Robot firmware version. e.g. "0.12"
		"--firmware-version "
)

func AddFwVersion() svc.ServiceConfig {
	return svc.WithCommand("add-fw-version", "Add a new firmware to intent version to table", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager(endpoints.DynamodbServiceID)
		awsconfigs.CommandInitialize(cmd)

		cmd.Spec = awsconfigs.CommandSpec() + AddCmdSpec

		// Required
		prefix := svc.StringOpt(cmd, "chipper-dynamo-prefix", "", "Prefix for DynamoDB table names")
		fwVersion := svc.StringOpt(cmd, "firmware-version", "", "Released firmware version")
		intentSvc := svc.StringOpt(cmd, "intent-service", "", "Intent-service to add [dialogflow, lex, ms]")
		lang := svc.StringOpt(cmd, "service-lang", "", "language of intent project")
		mode := svc.StringOpt(cmd, "robot-mode", "", "Robot mode [vc, game]")

		// Optional, but either Lex or Dialogflow needs to be populated
		dialogflowProjName := svc.StringOpt(cmd, "dialogflow-project-name", "", "Dialogflow project name for this version")
		dialogflowProjVersion := svc.StringOpt(cmd, "dialogflow-project-version", "", "Dialogflow project version to use")
		dialogflowCredsKey := svc.StringOpt(cmd, "dialogflow-creds-key", "", "Dialogflow credentials key in Secrets Manager")
		lexBotName := svc.StringOpt(cmd, "lex-botname", "", "Lex bot name")
		lexBotAlias := svc.StringOpt(cmd, "lex-bot-version", "", "Lex bot alias")
		enableVersion := svc.BoolOpt(cmd, "enable-version", true, "enable this version")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "add_fw_version",
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
					"action": "add_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing Dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Create a version mapping
			svcHash, err := requestrouter.GetServiceHashKey(*intentSvc, *lang, *mode)
			if err != nil {
				alog.Error{
					"action": "add_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error creating service hash key",
				}.Log()
				svc.Exit(-3)
			}

			intentVersion := requestrouter.NewIntentServiceVersion(
				requestrouter.WithIntentService(svcHash.PbService),
				requestrouter.WithProjectName(dialogflowProjName),
				requestrouter.WithDFVersion(dialogflowProjVersion),
				requestrouter.WithDFCredsKey(dialogflowCredsKey),
				requestrouter.WithLexBot(lexBotName, lexBotAlias),
			)

			err = intentVersion.Validate()
			if err != nil {
				alog.Error{
					"action": "add_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error creating intent-version",
				}.Log()
				svc.Exit(-4)
			}

			newVersion := &requestrouter.FirmwareIntentVersion{
				Service:       svcHash.Key,
				FwVersion:     *fwVersion,
				Lang:          svcHash.LangString,
				Mode:          svcHash.ModeString,
				IntentVersion: *intentVersion,
				Enable:        *enableVersion,
			}

			// Add version to Dynamodb
			err = store.StoreVersion(context.Background(), newVersion)
			if err != nil {
				alog.Error{
					"action": "add_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error adding new firmware-intent version map",
				}.Log()
				svc.Exit(-5)
			}

			// Print status
			ver := intentVersion.DFVersion
			if ver == "" {
				ver = intentVersion.LexBotAlias
			}

			alog.Info{
				"action":           "add_fw_version",
				"status":           alog.StatusOK,
				"firmware_version": *fwVersion,
				"service":          svcHash,
				"intent_version":   ver,
			}.Log()

		}
	})

}

func GetFwVersion() svc.ServiceConfig {
	return svc.WithCommand("get-fw-version", "Get an intent version from the table", func(cmd *cli.Cmd) {
		awsconfigs := awsutil.NewConfigManager("dynamodb")
		awsconfigs.CommandInitialize(cmd)

		cmd.Spec = awsconfigs.CommandSpec() + GetCmdSpec

		// Required
		prefix := svc.StringOpt(cmd, "chipper-dynamo-prefix", "", "Prefix for DynamoDB table names")
		svcName := svc.StringOpt(cmd, "service-name", "", "service name to query [df, lex, ms]")
		svcLang := svc.StringOpt(cmd, "service-lang", "", "service lang to query")
		mode := svc.StringOpt(cmd, "robot-mode", "", "Robot mode [vc, game]")
		fwVersion := svc.StringOpt(cmd, "firmware-version", "", "Released firmware version")

		cmd.Action = func() {

			// Get the dynamodb config
			cfg, err := awsconfigs.Config("dynamodb").NewConfig()
			if err != nil {
				alog.Error{
					"action": "get_fw_version",
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
					"action": "get_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error initializing Dynamo connection",
				}.Log()
				svc.Exit(-2)
			}

			// Get a version
			svcHash, err := requestrouter.GetServiceHashKey(*svcName, *svcLang, *mode)
			if err != nil {
				alog.Error{
					"action": "get_fw_version",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Fail to create service hash key",
					"svc":    *svcName,
				}.Log()
				svc.Exit(-3)
			}

			res, err := store.GetLastEnabledVersion(context.Background(), svcHash.Key, *fwVersion)
			if err != nil {
				alog.Error{
					"action": "get_fw_version",
					"status": alog.StatusFail,
					"error":  err,
					"msg":    "Fail to get firmware-intent version map",
					"svc":    svcHash.Key,
					"fw":     fwVersion,
				}.Log()
				svc.Exit(-4)
			}

			// Print result
			alog.Info{
				"action":  "get_fw_version",
				"status":  alog.StatusOK,
				"version": res,
			}.Log()
		}
	})

}
