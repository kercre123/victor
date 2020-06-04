package tokencmd

import (
	"context"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-service-framework/grpc/grpcclient"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/proto/tokenpb"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func AssociatePrimaryUser() svc.ServiceConfig {
	var userSession *string
	var robotCertFile *string
	var requestSts *bool

	return svc.WithCommandObject(
		"associate-primary-user",
		"Issue the AssociatePrimaryUser gRPC call",
		grpcclient.NewClientCommand(
			"token",
			"--user-session [--session-cert-file] [--request-sts]",
			func(cmd *cli.Cmd) {
				robotCertFile = cmd.StringOpt("f session-cert-file", "", "Certificate data for Robot Session certificate to store in S3")
				userSession = cmd.StringOpt("u user-session", "", "Accounts user session")
				requestSts = cmd.BoolOpt("s request-sts", false, "Request an STS token")
			},
			func(conn *grpc.ClientConn) error {
				req := &tokenpb.AssociatePrimaryUserRequest{
					GenerateStsToken: *requestSts,
				}

				if *robotCertFile != "" {
					cert, err := ioutil.ReadFile(*robotCertFile)
					if err != nil {
						return err
					}
					req.SessionCertificate = []byte(cert)
				}

				client := tokenpb.NewTokenClient(conn)
				resp, err := client.AssociatePrimaryUser(
					grpcutil.WithUserSessionKey(context.Background(), *userSession), req)
				if err != nil {
					return err
				}
				fmt.Println(resp)
				return nil
			}),
	)
}
