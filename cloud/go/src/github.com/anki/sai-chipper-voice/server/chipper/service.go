package chipper

import (
	"context"

	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"

	"github.com/anki/houndify-sdk-go/houndify"
	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/auth"
	"github.com/anki/sai-service-framework/grpc/grpcsvc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/anki/sai-chipper-voice/audiostore"
	"github.com/anki/sai-chipper-voice/client/chipperfn"
	"github.com/anki/sai-chipper-voice/conversation/microsoft"
	"github.com/anki/sai-chipper-voice/hstore"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/requestrouter"
)

const ServiceName = "sai_chipper"

type service struct {
	*svc.CompositeServiceObject

	conf serviceConfig

	server *Server

	dasClient *DasClient

	// chipperfnClient is the client to the gRPC service that chipper-fn
	chipperfnClient chipperfn.Client

	// hstoreClient is the client for Hypothesis Store
	hstoreClient *hstore.Client

	// request routing
	awsConfigManager *awsutil.ConfigManager
	requestRouter    *requestrouter.RequestRouter
}

type serviceConfig struct {
	serverAddr  *string
	portNum     *int
	profilePort *int

	// flag to indicate if we should log ASR transcript
	logTranscript *bool

	// flag to indicate if we want to save the raw audio
	saveAudio *bool

	cert *string
	key  *string

	tlsCert []byte
	tlsKey  []byte

	// Intent service configs
	dialogflowEnable      *bool
	dialogflowConfig      DialogflowConfig
	dialogflowGamesConfig DialogflowConfig

	luisEnable      *bool
	microsoftConfig MicrosoftConfig

	lexEnable              *bool
	lexEnableFarFieldModel *bool
	lexConfig              LexConfig
	lexGamesConfig         LexConfig

	// knowledge graph service configs
	houndifyEnable *bool
	houndifyConfig HoundifyConfig

	dasUrl *string

	// blobstore to store audio in dev
	namespace        *string
	audioStoreAppKey *string
	audioStoreUrl    *string

	// hypothesis store enable flag
	hstoreEnable *bool

	// request router
	tablePrefix *string
	env         *string
}

var (
	chipperServiceAuthSpec = []auth.GrpcAuthorizationSpec{
		{
			Method: "/chippergrpc2.ChipperGrpc/TextIntent",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthToken,
				ScopeMask:  auth.UserScopeMask,
			},
		},
		{
			Method: "/chippergrpc2.ChipperGrpc/StreamingIntent",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthToken,
				ScopeMask:  auth.UserScopeMask,
			},
		},
		{
			Method: "/chippergrpc2.ChipperGrpc/StreamingKnowledgeGraph",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthToken,
				ScopeMask:  auth.UserScopeMask,
			},
		},
		{
			Method: "/chippergrpc2.ChipperGrpc/StreamingConnectionCheck",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthToken,
				ScopeMask:  auth.UserScopeMask,
			},
		},
	}
)

func (s *service) CommandSpec() string {
	return "" +
		// Required. Default should be false, so we don't log transcript
		"--log-transcript " +

		// Required. Flag to set whether we save audio to blobstore
		"--save-audio " +

		// Required, enable Dialogflow
		"--dialogflow-enable " +

		// Dialogflow configs
		"[--google-project-name] [--google-application-credentials-raw] " +

		// Required, enable Luis
		"--luis-enable " +

		// Microscoft configs
		"[--microsoft-bing-key] [--microsoft-luis-key] [--microsoft-luis-app-id] " +

		// Required, enable Lex
		"--lex-enable --lex-enable-farfield-model " +
		// Lex configs
		" [--lex-region] [--lex-role-arn] " +
		" [--lex-bot-name] [--lex-bot-alias] " +
		" [--lex-games-bot-name] [--lex-games-bot-alias] " +

		// Required, enable Houndify
		"--houndify-enable " +

		// Optional houndify parameters
		" [--houndify-client-id] [--houndify-client-key] " +

		// audiostore for dev
		"[--blobstore-store-namespace] [--blobstore-store-appkey] [--blobstore-store-target] " +

		// das
		"[--das-url] " +

		// service framework
		s.CompositeServiceObject.CommandSpec() + " " +

		// chipperfn
		s.chipperfnClient.CommandSpec() + " " +

		// hypothesis store
		"--hstore-enable " + s.hstoreClient.CommandSpec() + " " +

		// request router
		// for dynamoDB
		s.awsConfigManager.CommandSpec() + " " +
		"[--chipper-dynamo-prefix] "
}

func (s *service) CommandInitialize(cmd *cli.Cmd) {
	s.CompositeServiceObject.CommandInitialize(cmd)
	s.chipperfnClient.CommandInitialize(cmd)

	s.conf.logTranscript = svc.BoolOpt(cmd, "log-transcript", false, "Enable logging of ASR transcript")
	s.conf.saveAudio = svc.BoolOpt(cmd, "save-audio", false, "Enable saving raw audio to blobstore")

	// google main project
	s.conf.dialogflowEnable = svc.BoolOpt(cmd, "dialogflow-enable", true, "Enable use of Google Dialogflow")
	s.conf.dialogflowConfig.ProjectName = svc.StringOpt(cmd, "google-project-name", "", "Name of Google project")
	s.conf.dialogflowConfig.Creds = svc.StringOpt(cmd, "google-application-credentials-raw", "",
		"Raw credentials for google to be copied to file")
	s.conf.dialogflowConfig.CredEnvVar = "GOOGLE_APPLICATION_CREDENTIALS"

	// google games project
	s.conf.dialogflowGamesConfig.ProjectName = svc.StringOpt(cmd, "google-project-name-games", "", "Name of Google project")
	s.conf.dialogflowGamesConfig.Creds = svc.StringOpt(cmd, "google-application-credentials-raw-games", "",
		"Raw credentials for google to be copied to file")
	s.conf.dialogflowGamesConfig.CredEnvVar = "GOOGLE_APPLICATION_CREDENTIALS_GAMES"

	// microsoft
	s.conf.luisEnable = svc.BoolOpt(cmd, "luis-enable", false, "Enable use of Microsoft Luis")
	s.conf.microsoftConfig.BingKey = svc.StringOpt(cmd, "microsoft-bing-key", "", "Microsoft Bing Speech API key")
	s.conf.microsoftConfig.LuisKey = svc.StringOpt(cmd, "microsoft-luis-key", "", "Microsoft LUIS key")
	s.conf.microsoftConfig.LuisAppId = svc.StringOpt(cmd, "microsoft-luis-app-id", "", "Microsoft LUIS App Id")

	// lex
	// voice command
	s.conf.lexEnable = svc.BoolOpt(cmd, "lex-enable", false, "Enable use of Amazon Lex")
	s.conf.lexEnableFarFieldModel = svc.BoolOpt(cmd, "lex-enable-farfield-model", false, "Enable far-field model for Lex ASR")
	s.conf.lexConfig.BotName = svc.StringOpt(cmd, "lex-bot-name", "", "Amazon Lex botname")
	s.conf.lexConfig.BotAlias = svc.StringOpt(cmd, "lex-bot-alias", "", "Amazon Lex bot alias")
	s.conf.lexConfig.Region = svc.StringOpt(cmd, "lex-region", "", "Amazon Lex region (probably us-east-1)")
	s.conf.lexConfig.AssumeRoleArn = svc.StringOpt(cmd, "lex-role-arn", "", "assume role arn to use lex in cloudai account")

	// games
	s.conf.lexGamesConfig.BotName = svc.StringOpt(cmd, "lex-games-bot-name", "", "Amazon Lex games botname")
	s.conf.lexGamesConfig.BotAlias = svc.StringOpt(cmd, "lex-games-bot-alias", "", "Amazon Lex games bot alias")
	s.conf.lexGamesConfig.Region = svc.StringOpt(cmd, "lex-region", "", "Amazon Lex region (probably us-east-1)")
	s.conf.lexGamesConfig.AssumeRoleArn = svc.StringOpt(cmd, "lex-role-arn", "", "assume role arn to use lex in cloudai account")

	// houndify
	s.conf.houndifyEnable = svc.BoolOpt(cmd, "houndify-enable", false, "Enable use of Houndify")
	s.conf.houndifyConfig.ClientId = svc.StringOpt(cmd, "houndify-client-id", "", "Houndify Client Id")
	s.conf.houndifyConfig.ClientKey = svc.StringOpt(cmd, "houndify-client-key", "", "Houndify Client Key")

	// anki services
	s.conf.dasUrl = svc.StringOpt(cmd, "das-url", "", "das queue url")
	s.conf.namespace = svc.StringOpt(cmd, "blobstore-store-namespace", "", "blobstore namespace")
	s.conf.audioStoreAppKey = svc.StringOpt(cmd, "blobstore-store-appkey", "", "blobstore app key")
	s.conf.audioStoreUrl = svc.StringOpt(cmd, "blobstore-store-target", "", "blobstore url")

	// hypothesis store
	s.conf.hstoreEnable = svc.BoolOpt(cmd, "hstore-enable", false, "Enable use of Hypothesis Store")
	s.hstoreClient.CommandInitialize(cmd)

	// request routing
	s.awsConfigManager.CommandInitialize(cmd)
	s.conf.tablePrefix = svc.StringOpt(cmd, "chipper-dynamo-prefix", "dev", "Dynamodb table name prefix")
}

func (s *service) Start(ctx context.Context) error {
	alog.Info{
		"action": "chipper_start",
		"status": alog.StatusStart,
	}.Log()

	alog.Debug{"configs": s.conf}.Log()

	// set up chipper server with all the clients

	// setup AudioStore client if necessary (dev only)
	var audioStoreClient *audiostore.Client = nil
	var err error = nil
	if *s.conf.saveAudio && *s.conf.audioStoreAppKey != "" && *s.conf.audioStoreUrl != "" && *s.conf.namespace != "" {
		audioStoreClient, err = audiostore.NewClient(ServiceName,
			*s.conf.namespace,
			*s.conf.audioStoreAppKey,
			*s.conf.audioStoreUrl,
		)

		if err != nil {
			alog.Warn{"action": "create_audiostore_client",
				"status": alog.StatusFail,
				"error":  err,
				"msg":    "No audio will be stored",
			}.Log()
		}
	}

	// DAS client
	s.dasClient = nil
	if *s.conf.dasUrl != "" {
		s.dasClient, err = NewDasClient("cloud", ServiceName, *s.conf.dasUrl)
		if err != nil {
			alog.Error{"action": "create_das_client", "status": alog.StatusFail, "error": err}.Log()
			cli.Exit(1)
		}
	}

	// Google Dialogflow
	if *s.conf.dialogflowEnable {
		err := InitDialogflow(s.conf.dialogflowConfig, s.conf.dialogflowGamesConfig)
		if err != nil {
			alog.Error{"action": "init_dialogflow", "status": alog.StatusFail, "error": err}.Log()
			cli.Exit(1)
		}
	} else {
		alog.Error{"action": "init_dialogflow", "status": alog.StatusError, "msg": "Dialogflow MUST BE ENABLED!!!"}.Log()
	}

	// Microsoft Luis
	var luisClient *microsoft.LuisClient = nil
	if *s.conf.luisEnable {
		luisClient = microsoft.NewLuisClient(
			*s.conf.microsoftConfig.LuisKey,
			*s.conf.microsoftConfig.LuisAppId,
		)
	}

	// Amazon Lex
	var lexClient *LexClient = nil
	if *s.conf.lexEnable && *s.conf.lexConfig.BotAlias != "" && *s.conf.lexConfig.BotName != "" {
		lexClient = NewLexClient(s.conf.lexConfig, s.conf.lexGamesConfig, false, *s.conf.lexEnableFarFieldModel, ServiceName)
	}

	// Houndify
	var houndifyClient *houndify.Client = nil
	if *s.conf.houndifyEnable && *s.conf.houndifyConfig.ClientId != "" && *s.conf.houndifyConfig.ClientKey != "" {
		houndifyClient = NewHoundifyClient(s.conf.houndifyConfig)
	}

	// Start chipper-fn client
	err = s.chipperfnClient.Start(ctx)
	if err != nil {
		alog.Warn{"action": "chipperfn_client_start", "status": alog.StatusFail, "error": err}.Log()
		s.chipperfnClient = nil
	}

	// Init and start hypothesis store client
	if *s.conf.hstoreEnable && *s.hstoreClient.Config.StreamName != "" {
		firehose := hstore.NewFirehose(s.hstoreClient.Config)
		metricsutil.AddAWSMonitoringHandlers(firehose.Client)
		s.hstoreClient.Init(firehose)
		s.hstoreClient.Start()
	} else {
		s.hstoreClient = nil
	}

	// Request routing
	if *s.conf.tablePrefix != "" {

		// Get AWS configs for route store
		cfg, err := s.awsConfigManager.Config(endpoints.DynamodbServiceID).NewConfig()
		if err != nil {
			alog.Warn{
				"action": "create_request_router",
				"status": alog.StatusError,
				"error":  err,
				"msg":    "Error getting AWS config",
			}.Log()
		}

		s.requestRouter = requestrouter.New(*s.conf.tablePrefix, nil, cfg)

		// Check if tables exist if we have a route store
		alog.Debug{"action": "checking_tables"}.Log()
		if s.requestRouter.Store != nil {
			err := s.requestRouter.Store.CheckTables()
			if err != nil {
				alog.Error{
					"action":       "check_request_router_tables",
					"status":       alog.StatusError,
					"error":        err,
					"msg":          "Tables for routing requests not found, set route-store to nil",
					"table_prefix": *s.conf.tablePrefix,
				}.Log()
				s.requestRouter.Store = nil
			}
		}

		// Add some default versions to the router, which should work even when route store is nil
		if s.requestRouter != nil {
			versions := requestrouter.CreateDefaultVersions(
				*s.conf.dialogflowConfig.ProjectName,
				s.conf.dialogflowConfig.CredEnvVar,
				*s.conf.dialogflowGamesConfig.ProjectName,
				s.conf.dialogflowGamesConfig.CredEnvVar,
				*s.conf.lexConfig.BotName,
				*s.conf.lexGamesConfig.BotName,
			)
			s.requestRouter.Init(*s.conf.dialogflowEnable, *s.conf.lexEnable, versions...)
			s.requestRouter.Start()

			// wait for request router to be ready
			<-s.requestRouter.Ready()
			alog.Info{"action": "request_router_started", "status": alog.StatusOK}.Log()
		}
	}

	s.server.withServerOpts(
		WithDialogflow(s.conf.dialogflowConfig, s.conf.dialogflowGamesConfig),
		WithMicrosoft(luisClient, s.conf.microsoftConfig),
		WithLex(lexClient),
		WithHoundify(houndifyClient),
		WithDas(s.dasClient),
		WithAudioStore(audioStoreClient),
		WithChipperFnClient(s.chipperfnClient),
		WithHypothesisStoreClient(s.hstoreClient),
		WithRequestRouter(s.requestRouter),
		WithLogTranscript(s.conf.logTranscript),
	)

	return s.CompositeServiceObject.Start(ctx)
}

func (s *service) Stop() error {
	if s.dasClient != nil {
		s.dasClient.Stop()
	}
	if s.chipperfnClient != nil {
		s.chipperfnClient.Stop()
	}
	if s.hstoreClient != nil {
		s.hstoreClient.Stop()
	}
	if s.requestRouter != nil {
		s.requestRouter.Stop()
	}
	return s.CompositeServiceObject.Stop()
}

//
// statushandler.StatusChecker interface to return service health
// status. This implementation is always healthy.
//

func (s *service) IsHealthy() error {
	// TODO: maybe send a text intent
	return nil
}

func NewService() svc.ServiceObject {
	s := &service{
		CompositeServiceObject: svc.NewServiceObject("chipper", svc.WithProfileServer()),
		awsConfigManager:       awsutil.NewConfigManager(endpoints.DynamodbServiceID),
		chipperfnClient:        chipperfn.NewClient(),
		hstoreClient:           hstore.New(&hstore.Config{}),
		requestRouter:          nil,
	}
	s.Add(svc.NewStatusMonitorComponent(svc.WithStatusChecker(s)))

	s.server = NewServer()
	s.Add(
		grpcsvc.New(
			grpcsvc.WithInterceptor(
				auth.NewGrpcAuthorizer(auth.WithAuthorizationSpecs(chipperServiceAuthSpec)),
			),

			// Add the register function to attach the chipper implementation to the gRPC server object.
			grpcsvc.WithRegisterFn(func(gs *grpc.Server) {
				pb.RegisterChipperGrpcServer(gs, s.server)

			})))
	return s

}
