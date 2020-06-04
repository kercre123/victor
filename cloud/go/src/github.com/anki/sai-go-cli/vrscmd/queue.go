package vrscmd

import (
	"encoding/base64"
	"encoding/json"
	"fmt"

	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sqs"
	cli "github.com/jawher/mow.cli"
)

func addToQueueCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("queue", "Add a virtual reward to the VRS inbound queue", func(cmd *cli.Cmd) {
		cmd.Spec = "--guid --sku [--qty] [--user-id...]"

		guid := cmd.StringOpt("g guid", "", "GUID for virtual reward")
		sku := cmd.StringOpt("s sku", "", "SKU to add")
		qty := cmd.IntOpt("q qty", 1, "SKU quantity")
		userIDs := cmd.StringsOpt("u user-id", nil, "Additional users to give virtual rewards to")

		cmd.Action = func() {
			cfg := *cfg

			sess, err := session.NewSession()
			if err != nil {
				cliutil.Fail("Failed to create AWS session: %v", err)
			}

			cdata, _ := json.Marshal(map[string]interface{}{
				"cart_guid":   *guid,
				"local_users": *userIDs,
			})

			body, _ := json.Marshal(map[string]interface{}{
				"client_data": base64.StdEncoding.EncodeToString(cdata),
				"rewards": map[string]int{
					*sku: *qty,
				},
			})

			queueURL := cfg.Env.ServiceURLs.ByService(config.VirtualRewardsQueue)
			if queueURL == "" {
				cliutil.Fail("No config entry for virtualrewards_queue_url")
			}

			svc := sqs.New(sess)
			params := &sqs.SendMessageInput{
				MessageBody: aws.String(string(body)),
				QueueUrl:    aws.String(queueURL),
			}
			_, err = svc.SendMessage(params)
			if err != nil {
				cliutil.Fail("SQS send failed: %v", err)
			}
			fmt.Println("Sent to queue at ", queueURL, string(body))
		}
	})
}
