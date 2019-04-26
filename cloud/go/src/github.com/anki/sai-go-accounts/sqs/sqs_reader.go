package sqs

import (
	"encoding/json"
	"fmt"
	"sync"
	"time"

	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-util/log"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/sqs"
	"github.com/aws/aws-sdk-go/service/sqs/sqsiface"
)

var (
	// How long to wait after a queue operation failure before retring
	sqsReadRetryInterval = time.Second
)

type SQSReader interface {
	ReceiveMessage() (resp []*SQSMessage, err error)
	DeleteMessage(string) (err error)
}

type AwsSQSReader struct {
	SQS                 sqsiface.SQSAPI
	QueueUrl            string
	WaitTimeSeconds     int64
	MaxNumberOfMessages int64
}

func (r AwsSQSReader) DeleteMessage(rh string) (err error) {
	params := &sqs.DeleteMessageInput{
		QueueUrl:      aws.String(r.QueueUrl),
		ReceiptHandle: aws.String(rh),
	}

	_, err = r.SQS.DeleteMessage(params)
	if err != nil {
		return err
	}

	return nil
}

func (r AwsSQSReader) ReceiveMessage() ([]*SQSMessage, error) {
	params := &sqs.ReceiveMessageInput{
		QueueUrl:            aws.String(r.QueueUrl),
		WaitTimeSeconds:     aws.Int64(r.WaitTimeSeconds),
		MaxNumberOfMessages: aws.Int64(r.MaxNumberOfMessages),
	}

	resp, err := r.SQS.ReceiveMessage(params)
	if err != nil {
		return nil, err
	}

	now := time.Now()
	msgs := make([]*SQSMessage, len(resp.Messages))
	for i, m := range resp.Messages {
		msgs[i] = &SQSMessage{
			MessageId:     *m.MessageId,
			Body:          *m.Body,
			ReceiptHandle: *m.ReceiptHandle,
			RecvTime:      now,
		}
	}
	return msgs, nil
}

type SQSMessage struct {
	MessageId     string
	Body          string
	ReceiptHandle string
	RecvTime      time.Time
}

type NoAutodeleteMessage struct {
	Userid  string `json:"user_id"`
	Orderid string `json:"order_id"`
}

// QueueReader reads messages from an incoming SQS Queue and sends the decoded message
// into an outbound channel.
type QueueReader struct {
	Reader SQSReader
	wg     sync.WaitGroup
	stop   chan struct{}
}

// Start starts a number of goroutines to read messages.
// Calling Stop() will trigger shutdown.
func (sr *QueueReader) Start(grcount int) {
	fmt.Println("starting queue reader")
	sr.stop = make(chan struct{})
	sr.wg.Add(grcount)
	for i := 0; i < grcount; i++ {
		go sr.processor()
	}
}

func (sr *QueueReader) processor() {
	for {
		select {
		case <-sr.stop:
			sr.wg.Done()
			return
		default:
			msgs, err := sr.Reader.ReceiveMessage()

			if err != nil {
				alog.Warn{"status": "receive_message_failed", "error": err}.Log()
				time.Sleep(sqsReadRetryInterval)
				continue
			}
			if len(msgs) == 0 {
				continue
			}

			for _, msg := range msgs {
				var nam NoAutodeleteMessage

				// unwind the json message
				if err := json.Unmarshal([]byte(msg.Body), &nam); err != nil {
					alog.Warn{"status": "receive_message_failed", "error": err}.Log()
				}
				if nam.Userid == "" || nam.Orderid == "" {
					alog.Warn{"status": "receive_message_failed", "error": "user_id or order_id is empty"}.Log()
				}

				// Fetch the existing user
				ua, err := user.AccountById(nam.Userid)

				if err != nil {
					alog.Error{"action": "update_user", "status": "unexpected_error", "error": err, "user_id": nam.Userid}.Log()
				}
				if ua == nil {
					alog.Debug{"action": "update_user", "status": "user_not_found", "user_id": nam.Userid}.Log()
				}

				// set the flag and update the user
				ua.User.NoAutodelete = true
				if err = user.UpdateUserAccount(ua); err != nil {
					alog.Error{"action": "update_user", "status": "unexpected_error", "error": err, "user_id": nam.Userid}.Log()
				}

				err = sr.Reader.DeleteMessage(msg.ReceiptHandle)
				if err != nil {
					alog.Error{"action": "delete_message_failed", "error": err}.Log()
				}
			}
		}
	}
}

// Stop stops new SQSs and waits for all processors to finish and exit
func (sr *QueueReader) Stop() {
	close(sr.stop)
	sr.wg.Wait()
}
