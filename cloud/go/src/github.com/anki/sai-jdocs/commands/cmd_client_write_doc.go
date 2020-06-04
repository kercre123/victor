package commands

import (
	"context"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func NewWriteDocClientCommand() *grpcsvc.ClientCommand {
	var account, thing, name, md, doc, docfile *string
	var dv, fv *int
	return grpcsvc.NewClientCommand(
		"jdocs", // prefix for args and env variables
		"[--account] [--thing] [--name] [--dv] [--fv] [--md] [--doc] [--docfile]", // spec
		func(cmd *cli.Cmd) { // init
			account = svc.StringOpt(cmd, "account", "", "")
			thing = svc.StringOpt(cmd, "thing", "", "")
			name = svc.StringOpt(cmd, "name", "", "Document name")
			dv = svc.IntOpt(cmd, "dv", 0, "Document version")
			fv = svc.IntOpt(cmd, "fv", 0, "Format version")
			md = svc.StringOpt(cmd, "md", "", "Client metadata string")
			doc = svc.StringOpt(cmd, "doc", "", "JSON document string")
			docfile = svc.StringOpt(cmd, "docfile", "", "If present, JSON write data is from this file")
		},
		func(conn *grpc.ClientConn) error {
			jsonDoc := *doc
			if *docfile != "" {
				if jsonBytes, err := ioutil.ReadFile(*docfile); err != nil {
					return err
				} else {
					jsonDoc = string(jsonBytes)
				}
			}

			client := jdocspb.NewJdocsClient(conn)
			wresp, err := client.WriteDoc(context.Background(),
				&jdocspb.WriteDocReq{
					Account: *account,
					Thing:   *thing,
					DocName: *name,
					Doc: &jdocspb.Jdoc{
						DocVersion:     uint64(*dv),
						FmtVersion:     uint64(*fv),
						ClientMetadata: *md,
						JsonDoc:        jsonDoc,
					},
				},
			)
			if err != nil {
				return err
			} else {
				alog.Info{
					"action": "write-doc",
					"status": "success",
				}.Log()
				fmt.Printf("--------------------\n")
				fmt.Printf("Status           = %s\n", wresp.Status.String())
				fmt.Printf("LatestDocVersion = %d\n", wresp.LatestDocVersion)
				fmt.Printf("--------------------\n")
			}
			return nil
		})
}
