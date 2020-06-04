package httpcapture

import (
	"fmt"

	"github.com/anki/sai-blobstore/client/blobstore"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

const (
	RequestIDKey   = "Usr-Requestid"
	UploadTimeKey  = "Srv-Upload-Time"
	ClientIPKey    = "Usr-Clientip"
	ServerIPKey    = "Usr-Serverip"
	HTTPURLKey     = "Usr-Httpurl"
	ServiceNameKey = "Usr-Service"
)

var columns = []struct {
	Fmt      string
	KeyName  string
	DispName string
}{
	{"%-30s", ServiceNameKey, "Service"},
	{"%-40.38s", RequestIDKey, "Request ID"},
	{"%-22s", UploadTimeKey, "Time"},
	{"%-18.15s", ClientIPKey, "Client IP"},
	{"%s", HTTPURLKey, "URL"},
}

func TailCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("tail", "Tail HTTP capture entries from the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] [--client-ip] [--host-ip] [--service] [--url-prefix]"

		namespace := cmd.StringOpt("n namespace", "sai-dev-capture",
			"Namespace to monitor")
		clientIP := cmd.StringOpt("c client-ip", "", "Client IP address to filter on")
		serverIP := cmd.StringOpt("h host-ip", "", "Server IP address to filter on")
		serviceName := cmd.StringOpt("s service", "", "Service to filter on")
		urlPrefix := cmd.StringOpt("u url-prefix", "", "URL prefix to filter on")

		cmd.Action = func() {
			c := newClient(*cfg)

			filt := new(blobstore.MetadataFilter)
			filt.AddExact(ClientIPKey, *clientIP)
			filt.AddExact(ServerIPKey, *serverIP)
			filt.AddExact(ServiceNameKey, *serviceName)
			filt.AddPrefix(HTTPURLKey, *urlPrefix)

			tailCfg := &blobstore.TailConfig{
				Namespace: *namespace,
				Filters:   filt,
			}

			fmt.Println("Namespace", *namespace)
			for _, col := range columns {
				fmt.Printf(col.Fmt, col.DispName)
			}
			fmt.Println("")
			err := c.Tail(tailCfg, func(matches []map[string]string) bool {
				for _, md := range matches {
					for _, col := range columns {
						fmt.Printf(col.Fmt, md[col.KeyName])
					}
					fmt.Println("")
				}
				return false
			})
			if err != nil {
				cliutil.Fail(err.Error())
			}
		}
	})
}
