package metricsutil

import (
	"github.com/aalpern/go-metrics"
	"github.com/aws/aws-sdk-go/aws/client"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/sqs"
)

var (
	awsRegistry = NewRegistry("aws")
	registries  = map[string]metrics.Registry{}
)

func getServiceRegistry(svcName string) metrics.Registry {
	r, ok := registries[svcName]
	if !ok {
		r = NewChildRegistry(awsRegistry, svcName)
		registries[svcName] = r
	}
	return r
}

// AddAWSMetricsHandlers adds request handlers to an AWS client to
// track latency of every operation, as well as counts for bytes sent
// & received, and errors. These generic per-operation metrics are
// installed in the clients for all AWS services.
//
// Additionally, it will install more detailed metrics handlers for
// specific AWS services.
//
// SQS: The metrics handler for SQS also tracks messages sent,
//   received, deleted and failed (since a single API operation can
//   operate on more than one SQS message).
func AddAWSMetricsHandlers(c *client.Client) *metrics.Registry {
	reg := getServiceRegistry(c.ClientInfo.ServiceName)
	c.Handlers.Send.PushBackNamed(request.NamedHandler{
		Name: "metrics",
		Fn: func(r *request.Request) {
			GetTimer(r.Operation.Name, reg).UpdateSince(r.Time)
			requestContentLength := r.HTTPRequest.ContentLength
			if requestContentLength > 0 {
				GetCounter(r.Operation.Name+".BytesSent", reg).Inc(requestContentLength)
			}
			responseContentLength := r.HTTPResponse.ContentLength
			if responseContentLength > 0 {
				GetCounter(r.Operation.Name+".BytesReceived", reg).Inc(responseContentLength)
			}
			if r.HTTPResponse.StatusCode >= 400 {
				GetCounter(r.Operation.Name+".Errors", reg).Inc(1)
			}
		},
	})
	c.Handlers.Complete.PushBackNamed(request.NamedHandler{
		Name: "metrics",
		Fn: func(r *request.Request) {
			if r.RetryCount > 0 {
				GetCounter(r.Operation.Name+".RetryCount", reg).Inc(int64(r.RetryCount))
			}
		},
	})
	switch c.ClientInfo.ServiceName {
	case "sqs":
		addSQSMetricsHandlers(c)
	}
	return &reg
}

func addSQSMetricsHandlers(c *client.Client) {
	if c.ClientInfo.ServiceName != "sqs" {
		panic("addSQSMetricsHandlers() called on non-sqs AWS client. Fix your code.")
	}
	reg := getServiceRegistry(c.ClientInfo.ServiceName)
	c.Handlers.Complete.PushBackNamed(request.NamedHandler{
		Name: "metrics.sqs",
		Fn: func(r *request.Request) {
			switch r.Operation.Name {
			case "SendMessage":
				if r.Error == nil {
					GetCounter(r.Operation.Name+".MessagesSent", reg).Inc(1)
				}
			case "SendMessageBatch":
				if out, ok := r.Data.(*sqs.SendMessageBatchOutput); ok {
					GetCounter(r.Operation.Name+".MessagesSent", reg).Inc(int64(len(out.Successful)))
					GetCounter(r.Operation.Name+".MessagesFailed", reg).Inc(int64(len(out.Failed)))
				}
			case "ReceiveMessage":
				if out, ok := r.Data.(*sqs.ReceiveMessageOutput); ok {
					GetCounter(r.Operation.Name+".MessagesReceived", reg).Inc(int64(len(out.Messages)))
				}
			case "DeleteMessage":
				if r.Error == nil {
					GetCounter(r.Operation.Name+".MessagesDeleted", reg).Inc(1)
				}
			case "DeleteMessageBatch":
				if out, ok := r.Data.(*sqs.DeleteMessageBatchOutput); ok {
					GetCounter(r.Operation.Name+".MessagesDeleted", reg).Inc(int64(len(out.Successful)))
					GetCounter(r.Operation.Name+".MessagesFailed", reg).Inc(int64(len(out.Failed)))
				}
			}
		},
	})
}
