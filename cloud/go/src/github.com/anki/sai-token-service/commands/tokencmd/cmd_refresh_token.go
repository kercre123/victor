package tokencmd

import (
	"context"
	"fmt"

	"github.com/anki/sai-service-framework/grpc/grpcclient"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/model"
	"github.com/anki/sai-token-service/proto/tokenpb"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func RefreshToken() svc.ServiceConfig {
	var tok *string
	var requestSts *bool

	return svc.WithCommandObject(
		"refresh-token",
		"Issue the RefreshToken gRPC call",
		grpcclient.NewClientCommand(
			"token",
			"--token [--request-sts]",
			func(cmd *cli.Cmd) {
				tok = cmd.StringOpt("t token", "", "JWT Token to refresh")
				requestSts = cmd.BoolOpt("s request-sts", false, "Request an STS token")
			},
			func(conn *grpc.ClientConn) error {
				client := tokenpb.NewTokenClient(conn)
				resp, err := client.RefreshToken(
					model.WithOutboundAccessToken(context.Background(), *tok),
					&tokenpb.RefreshTokenRequest{
						RefreshJwtTokens: true,
						RefreshStsTokens: *requestSts,
					})
				if err != nil {
					return err
				}
				fmt.Println(resp)
				return nil
			}),
	)
}
