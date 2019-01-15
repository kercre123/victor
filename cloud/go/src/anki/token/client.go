package token

import (
	"anki/opentracing"
	"anki/robot"
	"anki/token/identity"
	"anki/util"
	"context"
	"io/ioutil"

	pb "github.com/anki/sai-token-service/proto/tokenpb"
	otgrpc "github.com/opentracing-contrib/go-grpc"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type conn struct {
	conn   *grpc.ClientConn
	client pb.TokenClient
}

func newConn(identityProvider identity.Provider, serverURL string, creds credentials.PerRPCCredentials) (*conn, error) {
	dialOpts := append(getDialOptions(identityProvider, creds), util.CommonGRPC()...)
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

func (c *conn) associatePrimary(ctx context.Context, session string) (*pb.TokenBundle, error) {
	req := pb.AssociatePrimaryUserRequest{}
	cert, err := ioutil.ReadFile(robot.GatewayCert)
	if err != nil {
		return nil, err
	}
	req.SessionCertificate = cert
	response, err := c.client.AssociatePrimaryUser(ctx, &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) associateSecondary(ctx context.Context, session, clientName, appID string) (*pb.TokenBundle, error) {
	req := pb.AssociateSecondaryClientRequest{
		UserSession: session,
		ClientName:  clientName,
		AppId:       appID}
	response, err := c.client.AssociateSecondaryClient(ctx, &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) reassociatePrimary(ctx context.Context, clientName, appID string) (*pb.TokenBundle, error) {
	req := pb.ReassociatePrimaryUserRequest{
		ClientName: clientName,
		AppId:      appID}
	response, err := c.client.ReassociatePrimaryUser(ctx, &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) refreshToken(ctx context.Context, req pb.RefreshTokenRequest) (*pb.TokenBundle, error) {
	response, err := c.client.RefreshToken(ctx, &req)
	if err != nil {
		return nil, err
	}
	return response.Data, nil
}

func (c *conn) refreshJwtToken(ctx context.Context) (*pb.TokenBundle, error) {
	return c.refreshToken(ctx, pb.RefreshTokenRequest{RefreshJwtTokens: true})
}

func (c *conn) refreshStsCredentials(ctx context.Context) (*pb.TokenBundle, error) {
	return c.refreshToken(ctx, pb.RefreshTokenRequest{RefreshStsTokens: true})
}

func (c *conn) Close() error {
	return c.conn.Close()
}

func getDialOptions(identityProvider identity.Provider, creds credentials.PerRPCCredentials) []grpc.DialOption {
	var dialOpts []grpc.DialOption
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(identityProvider.TransportCredentials()))
	dialOpts = append(dialOpts, grpc.WithUnaryInterceptor(otgrpc.OpenTracingClientInterceptor(opentracing.OpenTracer)))
	dialOpts = append(dialOpts, grpc.WithStreamInterceptor(otgrpc.OpenTracingStreamClientInterceptor(opentracing.OpenTracer)))
	if creds != nil {
		dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(creds))
	}
	return dialOpts
}
