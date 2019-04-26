package testclient

import (
	"context"
	"fmt"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/proto/tokenpb"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

const (
	DefaultPort     = 10000
	DefaultServer   = "127.0.0.1"
	DefaultDeviceId = "00000000"
	DefaultFW       = "0.12"
	QModeText       = "text"
)

var Services = map[string]pb.IntentService{
	"default": pb.IntentService_DEFAULT,
	"goog":    pb.IntentService_DIALOGFLOW,
	"ms":      pb.IntentService_BING_LUIS,
	"lex":     pb.IntentService_LEX,
}

var Modes = map[string]pb.RobotMode{
	"command": pb.RobotMode_VOICE_COMMAND,
	"game":    pb.RobotMode_GAME,
}

var Encoding = map[string]pb.AudioEncoding{
	"pcm":  pb.AudioEncoding_LINEAR_PCM,
	"opus": pb.AudioEncoding_OGG_OPUS,
}

type clientOpts struct {
	Audio           pb.AudioEncoding
	Mode            pb.RobotMode // voice command or games
	Qmode           string       // text or stream
	AppKey          string
	DeviceId        string
	Session         string
	LangCode        pb.LanguageCode
	SingleUtterance bool
	SkipDas         bool
	Service         pb.IntentService
	Fw              string
	AuthCfg         AuthConfig
	TokenBundle     *tokenpb.TokenBundle
	Insecure        bool
	SaveAudio       bool
}

type ChipperClient struct {
	Client pb.ChipperGrpcClient
	Conn   *grpc.ClientConn
	Ctx    context.Context
	Opts   *clientOpts
}

type appKey struct {
	Value string
}

type AuthConfig struct {
	AnkiUsername    *string
	AnkiPassword    *string
	FactoryCertPath *string
	FactoryKeyPath  *string
	// setup stuff
	TmsUrl        *string
	ChipperAppKey *string
	AccountsEnv   *string
}

func (authCfg *AuthConfig) CommandInitializ(cmd *cli.Cmd) {
	authCfg.AccountsEnv = svc.StringOpt(cmd, "accounts-env", "", "env for anki account (prod/dev/beta)")
	authCfg.AnkiUsername = svc.StringOpt(cmd, "anki-user-name", "", "anki account username")
	authCfg.AnkiPassword = svc.StringOpt(cmd, "anki-password", "", "anki account password")
	authCfg.FactoryCertPath = svc.StringOpt(cmd, "factory-cert-path", "", "path to dev cert file")
	authCfg.FactoryKeyPath = svc.StringOpt(cmd, "factory-key-path", "", "path to dev key file")
	authCfg.ChipperAppKey = svc.StringOpt(cmd, "chipper-app-key", "", "chipper app key for the correct env")
	authCfg.TmsUrl = svc.StringOpt(cmd, "tms-url", "", "url of token service")

}

func (authCfg *AuthConfig) CommandSpec() string {
	return "--accounts-env " +
		" --anki-user-name " +
		"--anki-password " +
		"--factory-cert-path --factory-key-path " +
		"--chipper-app-key " +
		"--tms-url "
}

func NewTestClient(ctx context.Context, addr string, port int, options ...ClientOpt) (*ChipperClient, error) {

	copts := &clientOpts{
		Audio:           pb.AudioEncoding_LINEAR_PCM,
		Mode:            pb.RobotMode_VOICE_COMMAND,
		Qmode:           QModeText,
		LangCode:        pb.LanguageCode_ENGLISH_US,
		SingleUtterance: false,
		Service:         pb.IntentService_DIALOGFLOW,
		DeviceId:        DefaultDeviceId,
		Insecure:        false, // set to true for local testing
		SkipDas:         false,
		SaveAudio:       false,
	}

	for _, opt := range options {
		opt(copts)
	}

	var opts []grpc.DialOption
	if copts.Insecure {
		// local testing only
		opts = append(opts, grpc.WithInsecure())
	} else {
		// non-local, need TLS and credentials
		alog.Info{"action": "Using system cert"}.Log()
		opts = append(opts, grpc.WithTransportCredentials(credentials.NewClientTLSFromCert(nil, "")))

		// add perRPCCredentials
		// Get JWT from TMS to attach to chipper requests
		tokens, err := getJWT(ctx, copts.AuthCfg)
		if err != nil {
			alog.Error{"action": "get_jwt", "error": err}.Log()
			return nil, err
		}
		copts.TokenBundle = tokens

		if copts.TokenBundle != nil {
			opts = append(opts, grpc.WithPerRPCCredentials(jwtMetadata(copts.TokenBundle.Token, *copts.AuthCfg.ChipperAppKey)))
		}
	}

	address := fmt.Sprintf("%s:%d", addr, port)
	alog.Info{"action": "creating connection", "addr": address}.Log()

	conn, err := grpc.DialContext(ctx, address, opts...)
	if err != nil {
		alog.Error{"action": "create_conn", "status": "fail", "error": err}.Log()
		return nil, err
	}

	c := &ChipperClient{
		Client: pb.NewChipperGrpcClient(conn),
		Conn:   conn,
		Ctx:    ctx,
		Opts:   copts,
	}

	for _, opt := range options {
		opt(c.Opts)
	}

	return c, nil
}

func (c *ChipperClient) SetMode(m pb.RobotMode) {
	c.Opts.Mode = m
}

func (c *ChipperClient) SetContext(ctx context.Context) {
	c.Ctx = ctx
}

func (c *ChipperClient) SetFW(fw string) {
	c.Opts.Fw = fw
}
