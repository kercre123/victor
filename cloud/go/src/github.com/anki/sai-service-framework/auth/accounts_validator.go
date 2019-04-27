package auth

import (
	"context"

	"github.com/anki/sai-go-accounts/client/accounts"
	"github.com/anki/sai-service-framework/rest"
	"github.com/jawher/mow.cli"
)

// SessionValidator is a simplified interface for validating an appkey
// and optional user session, returning everything the accounts system
// knows about them.
type SessionValidator interface {
	ValidateSession(ctx context.Context, appkey, session string) (*AccountsCredentials, error)
}

// AccountsValidator is an implementation of SessionValidator which
// communicates with a running accounts service via the REST API.
type AccountsValidator struct {
	clientConfig rest.APIClientConfig
	client       *accounts.AccountsClient
}

type AccountsValidatorOpt func(a *AccountsValidator)

// WithAccountsClient configurs an AccountsValidator instance with a
// pre-existing accounts service client. Primarily for testing.
func WithAccountsClient(client *accounts.AccountsClient) AccountsValidatorOpt {
	return func(a *AccountsValidator) {
		a.client = client
	}
}

func NewAccountsValidator(opts ...AccountsValidatorOpt) *AccountsValidator {
	validator := &AccountsValidator{
		clientConfig: rest.APIClientConfig{
			Prefix: "accounts",
		},
	}
	for _, opt := range opts {
		opt(validator)
	}
	return validator
}

func (v *AccountsValidator) CommandSpec() string {
	return v.clientConfig.CommandSpec()
}

func (v *AccountsValidator) CommandInitialize(cmd *cli.Cmd) {
	v.clientConfig.CommandInitialize(cmd)
}

func (v *AccountsValidator) Start(ctx context.Context) error {
	if apiclient, err := v.clientConfig.NewClient(); err != nil {
		return err
	} else {
		v.client = &accounts.AccountsClient{Client: apiclient}
	}
	return nil
}

func (v *AccountsValidator) Stop() error {
	return nil
}

func (v *AccountsValidator) Kill() error {
	return v.Stop()
}

func (v *AccountsValidator) ValidateSession(ctx context.Context, appkey, session string) (*AccountsCredentials, error) {
	if v.client == nil {
		panic("AccountsValidator never initialized")
	}
	// TODO: pass ctx to ValidateSession2() when SAIP-1655 is implemented
	if _, response, err := v.client.ValidateSession2(appkey, session); err != nil {
		return nil, err
	} else {
		return &AccountsCredentials{
			AppKey:      response.ApiKey,
			UserSession: response.UserSession,
		}, nil
	}
}

type validatorContextKey int

const validatorKey validatorContextKey = iota

func WithSessionValidatorContext(ctx context.Context, s SessionValidator) context.Context {
	return context.WithValue(ctx, validatorKey, s)
}

func GetSessionValidator(ctx context.Context) SessionValidator {
	if value, ok := ctx.Value(validatorKey).(SessionValidator); ok {
		return value
	} else {
		return nil
	}
}
