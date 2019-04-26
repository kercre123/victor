package commands

import (
	"context"
	"fmt"

	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func NewEchoClientCommand() *grpcsvc.ClientCommand {
	var data *string
	return grpcsvc.NewClientCommand(
		"jdocs",    // prefix for args and env variables
		"[--data]", // spec
		func(cmd *cli.Cmd) { // init
			data = svc.StringOpt(cmd, "data", "Hello darkness, my old friend", "String to echo")
		},
		func(conn *grpc.ClientConn) error { // action
			client := jdocspb.NewJdocsClient(conn)
			out, err := client.Echo(context.Background(),
				&jdocspb.EchoReq{Data: *data},
			)
			if err != nil {
				return err
			} else {
				fmt.Println(out.Data)
			}
			return nil
		})
}
