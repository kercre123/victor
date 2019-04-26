package testclient

import (
	"context"
	"crypto/tls"
	"fmt"
	"github.com/anki/sai-go-accounts/client/accounts"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	pb "github.com/anki/sai-token-service/proto/tokenpb"
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

const (
	RobotESN = "chipper-test-client"
)

type MapCredentials map[string]string

func (r MapCredentials) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return r, nil
}

func (r MapCredentials) RequireTransportSecurity() bool {
	return true
}

func sessionMetadata(sessionToken string, appKey string) credentials.PerRPCCredentials {
	metadata := MapCredentials{
		"anki-user-session": sessionToken,
		"anki-app-key":      appKey,
	}
	return metadata
}

func jwtMetadata(jwtToken, appKey string) credentials.PerRPCCredentials {
	metadata := MapCredentials{
		"anki-access-token": jwtToken,
		"anki-app-key":      appKey,
	}
	return metadata
}

func getSessionToken(ctx context.Context, auth AuthConfig) (string, error) {
	cfg, err := config.Load("", false, *auth.AccountsEnv, "default")
	if err != nil {
		fmt.Println("Error loading cli config for auth", err)
		return "", err
	}
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Accounts)
	if err != nil {
		fmt.Println("Error creating cli config for auth", err)
		return "", err
	}
	c, err := accounts.NewAccountsClient("sai-go-cli", apicfg...)
	if err != nil {
		fmt.Println("Error creating account auth client", err)
		return "", err
	}

	resp, err := c.NewUserSession(ctx, *auth.AnkiUsername, *auth.AnkiPassword)
	if err != nil {
		fmt.Println("Error logging into accounts", err)
		return "", err
	}

	jresp, err := resp.Json()
	if err != nil {
		fmt.Println("Error getting json response for accounts auth", err)
		fmt.Println(resp.Body)
		return "", err
	}

	token, err := jresp.FieldStr("session", "session_token")
	if err != nil {
		fmt.Println("Error getting session out of json response for accounts auth", err)
		return "", err
	}
	return token, nil
}

func getTLSCreds(auth AuthConfig) (credentials.TransportCredentials, error) {
	cert, err := tls.LoadX509KeyPair(*auth.FactoryCertPath, *auth.FactoryKeyPath)
	if err != nil {
		fmt.Println("Error loading factory key pair", err)
		return nil, err
	}

	return credentials.NewTLS(&tls.Config{
		Certificates: []tls.Certificate{cert},
		RootCAs:      rootcerts.ServerCertPool(),
	}), nil
}

func getJWT(ctx context.Context, auth AuthConfig) (*pb.TokenBundle, error) {
	// Get session token from the accounts service
	sessionToken, err := getSessionToken(ctx, auth)
	if err != nil {
		return nil, err
	}

	sessionCreds := sessionMetadata(sessionToken, *auth.ChipperAppKey)
	var dialOpts []grpc.DialOption
	tlsCreds, err := getTLSCreds(auth)
	if err != nil {
		return nil, err
	}

	dialOpts = append(dialOpts, grpc.WithTransportCredentials(tlsCreds))
	dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(sessionCreds))
	rpcConn, err := grpc.Dial(*auth.TmsUrl, dialOpts...)
	if err != nil {
		fmt.Println("Error dialing TMS for chipper auth", err)
		return nil, err
	}

	rpcClient := pb.NewTokenClient(rpcConn)
	req := pb.AssociatePrimaryUserRequest{
		SkipClientToken: true,
	}

	resp, err := rpcClient.AssociatePrimaryUser(context.Background(), &req)
	if err != nil {
		fmt.Println("Error calling associate primary user", err)
		return nil, err
	}

	return resp.Data, nil

}
