package jdocssvc

import (
	"context"
	"errors"
	"io/ioutil"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

type service struct {
	*svc.CompositeServiceObject

	awsConfigManager *awsutil.ConfigManager

	// tablePrefix is a string prefix to apply to the dynamodb table names
	tablePrefix *string

	docSpecsFile *string

	jdocsServer *Server
}

//
// CommandInitializer interface implementation, for configuration
// options
//

func (s *service) CommandSpec() string {
	return s.CompositeServiceObject.CommandSpec() + " " +
		s.awsConfigManager.CommandSpec() + " " +
		"[--jdocs-dynamo-prefix] [--doc-specs-file]"
}

func (s *service) CommandInitialize(cmd *cli.Cmd) {
	s.CompositeServiceObject.CommandInitialize(cmd)
	s.awsConfigManager.CommandInitialize(cmd)
	s.tablePrefix = svc.StringOpt(cmd, "jdocs-dynamo-prefix", "", "Prefix for DynamoDB table names")
	s.docSpecsFile = svc.StringOpt(cmd, "doc-specs-file", "", "JSON-format file containing all document specs (overrides default specs)")
}

//
// statushandler.StatusChecker interface to return service health
// status. This implementation is always healthy.
//

func (s *service) IsHealthy() error {
	return nil
}

//
// Constructor - constructs the service object with the gRPC service
// component, hooking up the configuration information and configuring
// the gRPC service implementation.
//

func NewService() svc.ServiceObject {
	s := &service{
		CompositeServiceObject: svc.NewServiceObject("jdocs", svc.WithProfileServer()),
		awsConfigManager:       awsutil.NewConfigManager(endpoints.DynamodbServiceID),
		jdocsServer:            NewServer(),
	}
	s.Add(svc.NewStatusMonitorComponent(svc.WithStatusChecker(s)))
	s.Add(
		grpcsvc.New(
			//grpcsvc.WithInterceptor(interceptors.NewAuthFactory(interceptors.WithServerAuthenticator(s.authorize))),
			// Add the register function to attach the Jdocs service
			// implementation to the gRPC server object.
			grpcsvc.WithRegisterFn(func(gs *grpc.Server) {
				jdocspb.RegisterJdocsServer(gs, s.jdocsServer)
			})))
	return s
}

//
// ServiceObject interface implementation, for Start/Stop
// lifecycle. Nothing to do in this implementation, but this is the
// place to hook additional startup/shutdown logic in.
//

func (s *service) Start(ctx context.Context) error {
	if s.tablePrefix == nil || *s.tablePrefix == "" {
		return errors.New("JDOCS_DYNAMO_PREFIX must be set")
	}

	docSpecsJson := defaultDocSpecs
	if s.docSpecsFile != nil && *s.docSpecsFile != "" {
		if jsonBytes, rerr := ioutil.ReadFile(*s.docSpecsFile); rerr != nil {
			return rerr
		} else {
			docSpecsJson = string(jsonBytes)
		}
	}

	if cfg, err := s.awsConfigManager.Config(endpoints.DynamodbServiceID).NewConfig(); err != nil {
		return err
	} else if aerr := s.jdocsServer.InitAPI(docSpecsJson, *s.tablePrefix, cfg); aerr != nil {
		return aerr
	} else if terr := s.jdocsServer.CheckTables(); terr != nil {
		return terr
	}

	alog.Info{
		"action": "jdocs_start",
		"status": alog.StatusStart,
	}.Log()

	if err := s.CompositeServiceObject.Start(ctx); err != nil {
		return err
	}
	return nil
}

func (s *service) Stop() error {
	return s.CompositeServiceObject.Stop()
}
