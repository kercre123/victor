package commands

import (
	"context"
	"fmt"
	"strings"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func NewReadDocsClientCommand() *grpcsvc.ClientCommand {
	var account, thing, names *string
	return grpcsvc.NewClientCommand(
		"jdocs", // prefix for args and env variables
		"[--account] [--thing] [--names]", // spec
		func(cmd *cli.Cmd) { // init
			account = svc.StringOpt(cmd, "account", "", "")
			thing = svc.StringOpt(cmd, "thing", "", "")
			names = svc.StringOpt(cmd, "names", "", "Whitespace-separated list of document names")
		},
		func(conn *grpc.ClientConn) error {
			client := jdocspb.NewJdocsClient(conn)
			nameList := strings.Fields(*names)
			itemList := make([]*jdocspb.ReadDocsReq_Item, len(nameList))
			for i, name := range nameList {
				itemList[i] = &jdocspb.ReadDocsReq_Item{
					DocName:      name,
					MyDocVersion: uint64(0), // read latest
				}
			}
			rresp, err := client.ReadDocs(context.Background(),
				&jdocspb.ReadDocsReq{
					Account: *account,
					Thing:   *thing,
					Items:   itemList,
				},
			)
			if err != nil {
				return err
			} else {
				for i, _ := range rresp.Items {
					if (rresp.Items[i].Status == jdocspb.ReadDocsResp_UNCHANGED) ||
						(rresp.Items[i].Status == jdocspb.ReadDocsResp_CHANGED) {
						doc := rresp.Items[i].Doc
						alog.Info{
							"action": "read-doc",
							"status": "success",
							"dv":     doc.DocVersion,
							"fv":     doc.FmtVersion,
							"md":     doc.ClientMetadata,
							"doc":    doc.JsonDoc,
						}.Log()
					}
				}
				for i, _ := range rresp.Items {
					fmt.Printf("--------------------\n")
					fmt.Printf("DocName        = %s\n", itemList[i].DocName)
					fmt.Printf("Status         = %s\n", rresp.Items[i].Status.String())
					if (rresp.Items[i].Status == jdocspb.ReadDocsResp_UNCHANGED) ||
						(rresp.Items[i].Status == jdocspb.ReadDocsResp_CHANGED) {
						doc := rresp.Items[i].Doc
						fmt.Printf("DocVersion     = %d\n", doc.DocVersion)
						fmt.Printf("FmtVersion     = %d\n", doc.FmtVersion)
						fmt.Printf("ClientMetadata = %s\n", doc.ClientMetadata)
						fmt.Printf("JsonDoc        = %s\n", doc.JsonDoc)
					}
				}
				fmt.Printf("--------------------\n")
			}
			return nil
		})
}
