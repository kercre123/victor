package commands

import (
	"context"
	"fmt"
	"io/ioutil"
	"os"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

func NewReadDocClientCommand() *grpcsvc.ClientCommand {
	var account, thing, name, docfile *string
	var dv *int
	return grpcsvc.NewClientCommand(
		"jdocs", // prefix for args and env variables
		"[--account] [--thing] [--name] [--docfile] [--dv]", // spec
		func(cmd *cli.Cmd) { // init
			account = svc.StringOpt(cmd, "account", "", "")
			thing = svc.StringOpt(cmd, "thing", "", "")
			name = svc.StringOpt(cmd, "name", "", "Document name")
			dv = svc.IntOpt(cmd, "dv", 0, "My document version (0 => read latest)")
			docfile = svc.StringOpt(cmd, "docfile", "", "If present, read data is written to this file")
		},
		func(conn *grpc.ClientConn) error {
			client := jdocspb.NewJdocsClient(conn)
			rresp, err := client.ReadDocs(context.Background(),
				&jdocspb.ReadDocsReq{
					Account: *account,
					Thing:   *thing,
					Items: []*jdocspb.ReadDocsReq_Item{
						&jdocspb.ReadDocsReq_Item{
							DocName:      *name,
							MyDocVersion: uint64(*dv),
						},
					},
				},
			)
			if err != nil {
				return err
			} else if len(rresp.Items) != 1 {
				return fmt.Errorf("read-doc return wrong number of documents. Expected 1, got %d", len(rresp.Items))
			} else {
				fmt.Printf("--------------------\n")
				fmt.Printf("Status         = %s\n", rresp.Items[0].Status.String())
				if (rresp.Items[0].Status == jdocspb.ReadDocsResp_UNCHANGED) ||
					(rresp.Items[0].Status == jdocspb.ReadDocsResp_CHANGED) {
					doc := rresp.Items[0].Doc
					fmt.Printf("DocVersion     = %d\n", doc.DocVersion)
					fmt.Printf("FmtVersion     = %d\n", doc.FmtVersion)
					fmt.Printf("ClientMetadata = %s\n", doc.ClientMetadata)
					fmt.Printf("JsonDoc        = %s\n", doc.JsonDoc)
					fmt.Printf("--------------------\n")
					alog.Info{
						"action": "read-doc",
						"status": "success",
						"dv":     doc.DocVersion,
						"fv":     doc.FmtVersion,
						"md":     doc.ClientMetadata,
						"doc":    doc.JsonDoc,
					}.Log()
					if *docfile != "" {
						ioutil.WriteFile(*docfile, []byte(doc.JsonDoc), os.ModePerm)
					}
				} else {
					fmt.Printf("--------------------\n")
				}
			}
			return nil
		})
}
