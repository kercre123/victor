package token

import (
	"anki/log"
	"context"

	pb "github.com/anki/sai-token-service/proto/tokenpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type conn struct {
	conn   *grpc.ClientConn
	client pb.TokenClient
}

var platformOpts []grpc.DialOption

func newConn(serverURL string, creds credentials.PerRPCCredentials) (*conn, error) {
	dialOpts, err := getDialOptions(creds)
	if err != nil {
		return nil, err
	}
	if platformOpts != nil && len(platformOpts) > 0 {
		dialOpts = append(dialOpts, platformOpts...)
	}
	rpcConn, err := grpc.Dial(serverURL, dialOpts...)
	if err != nil {
		return nil, err
	}

	rpcClient := pb.NewTokenClient(rpcConn)

	ret := &conn{
		conn:   rpcConn,
		client: rpcClient}
	return ret, nil
}

func (c *conn) associatePrimary(session, robotID string) (*pb.TokenBundle, error) {
	req := pb.AssociatePrimaryUserRequest{
		GenerateStsToken: false,
		RobotId:          robotID,
		UserSession:      session}
	response, err := c.client.AssociatePrimaryUser(context.Background(), &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) refreshToken(existingToken string) (*pb.TokenBundle, error) {
	req := pb.RefreshTokenRequest{
		RefreshJwtTokens: true,
		RefreshStsTokens: false,
		Token:            existingToken}
	response, err := c.client.RefreshToken(context.Background(), &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) Close() error {
	return c.conn.Close()
}

func getDialOptions(creds credentials.PerRPCCredentials) ([]grpc.DialOption, error) {
	var dialOpts []grpc.DialOption
	certCreds, err := getTLSCert()
	if err != nil {
		log.Println("Error getting TLS cert:", err)
		return nil, err
	}
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(certCreds))
	if creds != nil {
		dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(creds))
	}
	return dialOpts, nil
}

type rpcMetadata map[string]string

func (r *rpcMetadata) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return *r, nil
}

func (r *rpcMetadata) RequireTransportSecurity() bool {
	return true
}

// TODO: make this configurable
const ankiDevKey = "xiepae8Ach2eequiphee4U"

func getAuthMetadata(sessionToken string) *rpcMetadata {
	return &rpcMetadata{
		"anki-user-session": sessionToken,
		"anki-app-key":      ankiDevKey,
	}
}

func getTokenMetadata(jwtToken string) *rpcMetadata {
	return &rpcMetadata{
		"anki-access-token": jwtToken,
		"anki-app-key":      ankiDevKey,
	}
}
