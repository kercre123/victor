package rest

import (
	"fmt"

	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
)

// APIClientConfig configure the core parameters for initializing a
// REST HTTP client (sai-go-util/http/apiclient/Client).
type APIClientConfig struct {
	// Prefix is prepended to the names of the command line
	// arguments/environment variables used to initialize the client,
	// so that multiple clients can be configured. It is an error to
	// leave this empty.
	Prefix string

	// ServerURL is the full URL (including port, if necessary) of the
	// REST service to connect to.
	ServerURL *string

	// AppKey is the accounts-issued app key used to identify the
	// source of requests in API calls.
	AppKey *string

	// TODO: timeout, http capture, debug, retry config, tls config
}

func (cfg *APIClientConfig) argname(arg string) string {
	return fmt.Sprintf("%s-%s", cfg.Prefix, arg)
}

func (cfg *APIClientConfig) CommandSpec() string {
	return fmt.Sprintf("[--%s] [--%s]",
		cfg.argname("server-url"), cfg.argname("app-key"))
}

func (cfg *APIClientConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.ServerURL = svc.StringOpt(cmd, cfg.argname("server-url"), "",
		cfg.Prefix+" service URL")
	cfg.AppKey = svc.StringOpt(cmd, cfg.argname("app-key"), "",
		"App key to identify this client to the "+cfg.Prefix+" service")
}

func (cfg *APIClientConfig) GetClientConfigs() []apiclient.Config {
	return []apiclient.Config{
		apiclient.WithServerURL(*cfg.ServerURL),
		apiclient.WithAppKey(*cfg.AppKey),
		apiclient.WithBuildInfoUserAgent(),
	}
}

func (cfg *APIClientConfig) NewClient(configs ...apiclient.Config) (*apiclient.Client, error) {
	if cfg.ServerURL == nil || *cfg.ServerURL == "" {
		return nil, fmt.Errorf("ServerURL for API client %s cannot be empty", cfg.Prefix)
	}
	if cfg.AppKey == nil || *cfg.AppKey == "" {
		return nil, fmt.Errorf("AppKey for API client %s cannot be empty", cfg.Prefix)
	}

	alog.Info{
		"action":     "new_apiclient",
		"server_url": *cfg.ServerURL,
	}.Log()

	cfgs := make([]apiclient.Config, 0, 5)
	cfgs = append(cfgs, cfg.GetClientConfigs()...)
	cfgs = append(cfgs, configs...)

	return apiclient.New(cfg.Prefix, cfgs...)
}
