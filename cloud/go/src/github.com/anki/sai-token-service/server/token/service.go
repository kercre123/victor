package token

import (
	"context"
	"crypto/x509"
	"errors"

	"github.com/anki/sai-awsutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/auth"
	"github.com/anki/sai-service-framework/grpc/grpcsvc"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/certstore"
	"github.com/anki/sai-token-service/db"
	"github.com/anki/sai-token-service/proto/tokenpb"
	"github.com/anki/sai-token-service/stsgen"
	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

type service struct {
	*svc.CompositeServiceObject

	awsconfigs *awsutil.ConfigManager
	store      db.TokenStore
	certStore  certstore.RobotCertificateStore

	// stsGen is a generator for STS tokens.
	stsGen stsgen.Generator

	// signer loads the signing certificate (same as the validator)
	// and its private key, for signing outgoing tokens.
	signer *CertificateTokenSigner

	// ankiCert is the loaded public certificate for the Robot PKI
	// certificate authority.
	ankiCert *x509.Certificate

	// tablePrefix is a string prefix to apply to the dynamodb table
	// names, if the dynamodb backing store is used.
	tablePrefix *string

	// backingStore is either "dynamodb", or "memory", setting which
	// backing store implementation is used. "memory" should only be
	// used for testing.
	backingStore *string

	// certStoreConfig encapsulates the settings for the robot
	// certificate storage
	certStoreConfig certstore.RobotCertificateStoreConfig

	// stsGenConfig encapsulates the settinsg for the STS token generator
	stsGenConfig stsgen.StsGeneratorConfig

	// clientStore is a client for the Jdocs service which manages
	// storing hashed client tokens.
	clientStore ClientTokenStore
}

var (
	tokenServiceAuthSpec = []auth.GrpcAuthorizationSpec{
		{
			Method: "/tokenpb.Token/AssociatePrimaryUser",
			Auth: auth.AuthorizationSpec{
				AuthMethod:    auth.AuthAccounts,
				SessionStatus: auth.RequireSession,
				ScopeMask:     auth.UserScopeMask,
			},
		},
		{
			Method: "/tokenpb.Token/ReassociatePrimaryUser",
			Auth: auth.AuthorizationSpec{
				AuthMethod:         auth.AuthBoth,
				SessionStatus:      auth.RequireSession,
				ScopeMask:          auth.UserScopeMask,
				AllowExpiredTokens: true,
				AllowRevokedTokens: true,
			},
		},
		{
			Method: "/tokenpb.Token/AssociateSecondaryClient",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthToken,
				ScopeMask:  auth.UserScopeMask,
			},
		},
		{
			Method: "/tokenpb.Token/DisassociatePrimaryUser",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthEither,
				ScopeMask:  auth.UserSupportOrAdminScopeMask,
			},
		},
		{
			Method: "/tokenpb.Token/RefreshToken",
			Auth: auth.AuthorizationSpec{
				AuthMethod:         auth.AuthToken,
				ScopeMask:          auth.UserScopeMask,
				AllowExpiredTokens: true,
			},
		},
		{
			Method: "/tokenpb.Token/ListRevokedTokens",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthAccounts,
				ScopeMask:  auth.ServiceScopeMask,
			},
		},
		{
			Method: "/tokenpb.Token/RevokeCertificate",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthAccounts,
				ScopeMask:  auth.AdminScopeMask,
			},
		},
		{
			Method: "/tokenpb.Token/RevokeTokens",
			Auth: auth.AuthorizationSpec{
				AuthMethod: auth.AuthAccounts,
				ScopeMask:  auth.ServiceScopeMask,
			},
		},
	}
)

func NewService() svc.ServiceObject {
	s := &service{
		CompositeServiceObject: svc.NewServiceObject("token", svc.WithProfileServer()),
		awsconfigs: awsutil.NewConfigManager(
			endpoints.DynamodbServiceID,
			endpoints.S3ServiceID,
			endpoints.StsServiceID),
		signer:      NewCertificateTokenSigner(),
		clientStore: NewClientTokenStore(),
	}
	s.Add(s.clientStore)
	s.Add(svc.NewStatusMonitorComponent(svc.WithStatusChecker(s)))
	s.Add(
		grpcsvc.New(
			grpcsvc.WithRegisterFn(func(gs *grpc.Server) {
				tokenpb.RegisterTokenServer(gs, &Server{
					Store:       s.store,
					CertStore:   s.certStore,
					Signer:      s.signer,
					StsGen:      s.stsGen,
					ClientStore: s.clientStore,
				})
			}),

			// Add the authorizer, to enforce access controls based on
			// the credentials extracted by the accounts and token
			// authenticators, which are part of the standard
			// interceptor set.
			grpcsvc.WithInterceptor(
				auth.NewGrpcAuthorizer(auth.WithAuthorizationSpecs(tokenServiceAuthSpec)))))

	return s
}

func (s *service) IsHealthy() error {
	if s.store == nil {
		// service has not been started yet
		return nil
	}
	return s.store.IsHealthy()
}

func (s *service) CommandSpec() string {
	return s.CompositeServiceObject.CommandSpec() + " " +
		s.signer.CommandSpec() + " " +
		s.awsconfigs.CommandSpec() + " " +
		"[--token-service-dynamo-prefix] [--token-service-backing-store] " +
		s.certStoreConfig.CommandSpec() +
		s.stsGenConfig.CommandSpec()
}

func (s *service) CommandInitialize(cmd *cli.Cmd) {
	s.CompositeServiceObject.CommandInitialize(cmd)
	s.awsconfigs.CommandInitialize(cmd)
	s.signer.CommandInitialize(cmd)
	s.certStoreConfig.CommandInitialize(cmd)
	s.stsGenConfig.CommandInitialize(cmd)
	s.tablePrefix = svc.StringOpt(cmd, "token-service-dynamo-prefix", "", "Prefix for DynamoDB table names")
	s.backingStore = svc.StringOpt(cmd, "token-service-backing-store", "memory", "Backing store for token storage. One of 'dynamodb' or 'memory'")
}

func (s *service) Start(ctx context.Context) error {
	if s.backingStore == nil || *s.backingStore == "" {
		return errors.New("TOKEN_SERVICE_BACKING_STORE not set")
	}

	alog.Info{
		"action":        "start",
		"backing_store": *s.backingStore,
	}.Log()

	switch *s.backingStore {
	case "dynamodb":
		if s.tablePrefix == nil || *s.tablePrefix == "" {
			return errors.New("TOKEN_SERVICE_DYNAMO_PREFIX must be set if backing store is 'dynamodb'")
		}
		if cfg, err := s.awsconfigs.Config(endpoints.DynamodbServiceID).NewConfig(); err != nil {
			return err
		} else {
			if store, err := db.NewDynamoTokenStore(*s.tablePrefix, cfg); err != nil {
				return err
			} else {
				s.store = store
			}
		}
	case "memory":
		s.store = db.NewMemoryTokenStore()
	}

	s.store = db.NewCachingTokenStore(s.store)

	if s3Cfg, err := s.awsconfigs.Config(endpoints.S3ServiceID).NewConfig(); err != nil {
		return err
	} else {
		certStore, err := s.certStoreConfig.NewStore(s3Cfg)
		if err != nil {
			return err
		}
		s.certStore = certStore
	}

	if stsCfg, err := s.awsconfigs.Config(endpoints.StsServiceID).NewConfig(); err != nil {
		return err
	} else {
		stsGen, err := s.stsGenConfig.NewSTSGenerator(stsCfg)
		if err != nil {
			return err
		}
		s.stsGen = stsGen // will be nil if disabled
	}

	if err := s.signer.Start(ctx); err != nil {
		return err
	}

	if err := s.CompositeServiceObject.Start(ctx); err != nil {
		return err
	}
	return nil
}

func (s *service) Stop() error {
	return s.CompositeServiceObject.Stop()
}
