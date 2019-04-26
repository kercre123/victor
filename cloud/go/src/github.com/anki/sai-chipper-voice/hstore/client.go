package hstore

import (
	"encoding/json"
	"errors"
	"sync"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/firehose"

	"github.com/anki/sai-chipper-voice/server/chippermetrics"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/cenkalti/backoff"
	"github.com/jawher/mow.cli"
)

const (
	maxFirehoseRecordSize   = 1 << 20 // 1,000 KiB or 1MB
	maxFirehoseBatchRecords = 500     // records per call or
	maxFirehoseBatchSize    = 4 << 20 // 4 MiB, whichever is smaller
	maxFirehoseBatches      = 2000    // 2000 transactions per second

	AWSAccessDeniedError = "AccessDeniedException"
)

var (
	ErrClientStopped      = errors.New("Fail to put record. Client is stopped")
	ErrMarshalJsonRecord  = errors.New("Fail to marshal record into json")
	ErrRecordSizeExceeded = errors.New("Record bytes size must less than 1MB")

	// HypothesisMetrics saves metrics related to hypothesis store client and firehose
	HypothesisMetrics = metricsutil.NewChildRegistry(chippermetrics.Metrics, "hypothesis_store")
)

type FirehosePutter interface {
	PutRecordBatch(b *firehose.PutRecordBatchInput) (*firehose.PutRecordBatchOutput, error)
}

// Client will batch some number of records then put them into the Firehose
type Client struct {
	sync.RWMutex

	*Config

	firehose FirehosePutter

	// logTranscript flag to print transcript and parameters in logs
	logTranscript bool

	// recordQueue is a channel to stuff records
	recordQueue chan *firehose.Record

	// buffer stores records till there're enough to make a batch
	buffer *Buffer

	// semaphore tp limit the number of goroutines used to send firehose requests
	semaphore semaphore

	// flushTimeout is the interval to periodically clear the buffer
	flushTimeout time.Duration

	// retries counts the number of firehose retrys
	retries int

	wg sync.WaitGroup

	// stop signal that client is stopping, flush any in-flight data
	stop chan struct{}

	// isStopped disallow any addition of new records, set to true after calling Client.Stop()
	isStopped bool
}

func NewFirehose(cfg *Config) *firehose.Firehose {
	// create Firehose client
	sess := session.Must(session.NewSession())
	creds := stscreds.NewCredentials(sess, *cfg.RoleArn)
	awsConfig := aws.NewConfig().WithCredentials(creds).WithRegion(*cfg.Region)
	return firehose.New(sess, awsConfig)
}

func New(cfg *Config) *Client {
	if cfg == nil {
		cfg = &Config{}
	}
	return &Client{
		logTranscript: false,
		Config:        cfg,
		retries:       0,
		isStopped:     true,
	}
}

func NewClient(cfg *Config, fh FirehosePutter, logTranscript bool) *Client {
	c := New(cfg)
	c.logTranscript = logTranscript
	c.Init(fh)
	return c
}

func (c *Client) Init(fh FirehosePutter) {

	// set config defaults
	c.Config.defaults()

	c.buffer = &Buffer{
		streamName: *c.Config.StreamName,
	}

	c.firehose = fh
	c.recordQueue = make(chan *firehose.Record, *c.Config.RecordQueueSize)
	c.semaphore = make(chan struct{}, *c.Config.MaxRequests)
	c.flushTimeout = c.Config.BufferFlushTime.Duration()
	c.stop = make(chan struct{})
}

func (c *Client) CommandSpec() string {
	return " " +
		// Required. Firehose stream parameters
		"--hstore-firehose-stream " +
		"--hstore-firehose-region " +
		"--hstore-firehose-role-arn " +

		// Optional. See Config struct for definitions.
		"[--hstore-queue-size] " +
		"[--hstore-queue-flush-time] " +
		"[--hstore-buffer-size] " +
		"[--hstore-max-requests] " +
		"[--hstore-max-retry] "

}

func (c *Client) CommandInitialize(cmd *cli.Cmd) {
	// Required
	c.Config.StreamName = svc.StringOpt(cmd, "hstore-firehose-stream", "", "Hypothesis store Firehose stream name")
	c.Config.Region = svc.StringOpt(cmd, "hstore-firehose-region", "", "Hypothesis store Firehose region")
	c.Config.RoleArn = svc.StringOpt(cmd, "hstore-firehose-role-arn", "", "Hypothesis store Firehose assume-role arn")

	// Optional
	c.Config.RecordQueueSize = svc.IntOpt(cmd, "hstore-queue-size", 0, "channel size for records before being added to queue buffer")
	c.Config.BufferFlushTime = svc.DurationOpt(cmd, "hstore-queue-flush-time", time.Second, "queue buffer flush time in ms. e.g. '10ms'")
	c.Config.BufferSize = svc.IntOpt(cmd, "hstore-buffer-size", 0, "queue buffer size")
	c.Config.MaxRequests = svc.IntOpt(cmd, "hstore-max-requests", 0, "max concurrent request to Firehose")
	c.Config.MaxRetry = svc.IntOpt(cmd, "hstore-max-retry", 0, "max retry attempts for a failed Firehose Put operation")
}

// Start client
func (c *Client) Start() {
	alog.Info{"action": "start_firehose_batch_client", "stream": *c.StreamName}.Log()
	c.wg.Add(1)
	c.isStopped = false
	go c.loop()
}

// Stop the client
func (c *Client) Stop() {
	// set isStopped so client will stop adding records
	c.Lock()
	c.isStopped = true
	c.Unlock()

	// send stop signal
	close(c.stop)

	// wait for loop() to finish
	c.wg.Wait()

	// wait for goroutines to complete
	alog.Info{"action": "wait_for_flush_done", "num": c.semaphore.used()}.Log()
	c.semaphore.wait()

	// close queue
	close(c.recordQueue)

	alog.Warn{"action": "hypostore_client_stopped"}.Log()
}

// Put adds a record to the client's queue
func (c *Client) Put(record Record) error {
	if c.logTranscript {
		alog.Debug{
			"action":     "hypostore_put_record",
			"hypothesis": record.Hypothesis,
			"record":     record,
		}.Log()
	}

	c.RLock()
	stopped := c.isStopped
	c.RUnlock()

	if stopped {
		return ErrClientStopped
	}

	recordBytes, err := json.Marshal(record)

	if err != nil {
		metricsutil.GetCounter("PutJsonError", HypothesisMetrics).Inc(1)
		return ErrMarshalJsonRecord
	}

	if len(recordBytes) > maxFirehoseRecordSize {
		metricsutil.GetCounter("PutSizeError", HypothesisMetrics).Inc(1)
		return ErrRecordSizeExceeded
	}

	c.recordQueue <- &firehose.Record{Data: recordBytes}

	metricsutil.GetCounter("Put", HypothesisMetrics).Inc(1)
	alog.Debug{"action": "hypostore_put_record", "status": "success"}.Log()
	return nil
}

// needToDrain checks if there's enough records in the buffer to make a BatchInput
// returns true once we have enough records to create a batch or
// total records size in bytes reached the limit
// always check before stuffing one more record into the buffer
// calling code needs to lock
func (c *Client) needToDrain() bool {
	return c.buffer.Count() >= maxFirehoseBatchRecords || c.buffer.Size() >= maxFirehoseBatchSize
}

// flush records to the firehose
// Divide the records into legit firehose BatchInputs for batch Puts
// failed requests are put back into records for next round of processing
// retry failures if necessary
func (c *Client) flush(records []*firehose.Record) (nProcessed, nSuccess, nRetry int) {
	defer c.semaphore.release()

	alog.Info{"action": "flush_records", "num_records": len(records)}.Log()

	exp := backoff.NewExponentialBackOff()
	backoffInterval := time.Duration(0)

	// no. of records we want to put
	nRecords := len(records)

	// no. of records successfully put into firehose
	nSuccess = 0

	// no. of records processed, including retries
	nProcessed = 0

	// no. of times to retry
	nRetry = 0

	// keep looping until we have processed all records or reached retry limits
	for {
		// create a PutRecordBatchInput
		nBytes := 0
		batch := make([]*firehose.Record, 0)
		for _, r := range records {
			recordSize := len(r.Data)
			if (nBytes+recordSize) >= maxFirehoseBatchSize || len(batch) >= maxFirehoseBatchRecords {
				// the batch has reached the firehose limits
				break
			}

			// keep adding to the batch
			nBytes += recordSize
			batch = append(batch, r)
		}

		// delete records that are added to the batch
		records = records[len(batch):]

		// put into firehose
		alog.Info{
			"action":      "put_record_batch",
			"status":      "start",
			"num_records": len(batch),
			"bytes_size":  nBytes,
		}.Log()

		start := time.Now()

		output, err := c.firehose.PutRecordBatch(
			&firehose.PutRecordBatchInput{
				DeliveryStreamName: c.Config.StreamName,
				Records:            batch,
			},
		)

		metricsutil.GetTimer("FirehosePutBatchLatency", HypothesisMetrics).UpdateSince(start)
		metricsutil.GetCounter("FirehoseBatchesPut", HypothesisMetrics).Inc(1)

		// uh-oh, errors!
		if err != nil {
			metricsutil.GetCounter("FirehosePutBatchError", HypothesisMetrics).Inc(1)

			code := alog.StatusFail
			if aerr, ok := err.(awserr.Error); ok {
				code = aerr.Code()
			}

			if code == firehose.ErrCodeServiceUnavailableException || code == firehose.ErrCodeLimitExceededException {
				// service unavailable, backoff and retry
				backoffInterval = exp.NextBackOff()
				alog.Warn{
					"action":     "set_backoff_interval",
					"status":     alog.StatusError,
					"interval":   backoffInterval,
					"retry":      nRetry,
					"error_code": code,
					"msg":        "encounter resource usage error while putting a batch, backoff a bit",
				}.Log()
			}

			if code == AWSAccessDeniedError {
				alog.Error{
					"action":     "put_batch_record",
					"status":     alog.StatusError,
					"error":      err,
					"error_code": code,
					"msg":        "access to firehose is denied",
				}.Log()
				metricsutil.GetCounter("FirehoseAccessDeniedErr", HypothesisMetrics).Inc(1)
				return
			}

			alog.Warn{
				"action":     "put_record_batch",
				"status":     alog.StatusError,
				"error_code": code,
				"error":      err,
				"msg":        "fail to put a record batch to firehose",
			}.Log()
		}

		// check for failed requests
		failed := int64(0)
		if output != nil && output.FailedPutCount != nil {
			failed = *output.FailedPutCount
			if failed != 0 {
				metricsutil.GetCounter("FirehoseFailedPutCount", HypothesisMetrics).Inc(failed)
				// put failed requests back into records for processing in the next round
				for i, resp := range output.RequestResponses {
					if resp.RecordId != nil {
						// successful puts will have RecordId set to non-nil value
						continue
					}
					alog.Warn{
						"action":     "failed_record",
						"status":     alog.StatusFail,
						"error_code": resp.ErrorCode,
						"error":      resp.ErrorMessage,
						"index":      i,
						"msg":        "checking failed requests in a batch",
					}.Log()
					records = append(records, batch[i])
				}
			}
		} else {
			alog.Warn{
				"action":  "get_failed_put_count",
				"status":  alog.StatusFail,
				"error":   err,
				"output":  output,
				"retries": nRetry,
				"msg":     "firehouse FailedPutCount is nil, retry entire batch",
			}.Log()

			// strange, we should always get FailedPutCount
			// we should retry the entire batch in this case
			failed = int64(len(batch))
			records = append(records, batch...)
		}

		nProcessed += len(batch)
		nSuccess += (len(batch) - int(failed))

		alog.Info{
			"action":          "put_record_batch",
			"status":          alog.StatusOK,
			"batch_size":      len(batch),
			"batch_failures":  failed,
			"total_success":   nSuccess,
			"total_processed": nProcessed,
			"total_records":   nRecords,
			"retry":           nRetry,
			"msg":             "completed a batch",
		}.Log()

		if nSuccess >= nRecords {
			// nothing more to put into firehose, exit
			metricsutil.GetCounter("FirehosePutSuccessCount", HypothesisMetrics).Inc(int64(nSuccess))
			return
		}

		// if we've gone through all records once, this round is a retry, incr counter
		if nProcessed >= nRecords {
			nRetry++
		}

		// check if we've reached the retry limit
		if nRetry > *c.MaxRetry {
			remaining := len(records)
			alog.Warn{
				"action":            "reach_retry_limit",
				"status":            "exit_flush",
				"remaining_records": remaining,
			}.Log()

			metricsutil.GetCounter("FirehoseRetryMaxReached", HypothesisMetrics).Inc(1)
			metricsutil.GetCounter("FirehoseRetryFailedCount", HypothesisMetrics).Inc(int64(remaining))

			return
		}

		if nRetry > 0 {
			metricsutil.GetCounter("FirehoseRetryCount", HypothesisMetrics).Inc(1)
			backoffInterval = exp.NextBackOff()
		}

		if backoffInterval > 0 {
			alog.Debug{"action": "backoff_sleep", "interval": backoffInterval, "retry": nRetry}.Log()
			time.Sleep(backoffInterval)
		}
	}

}

// loop and flush at the configured interval, or when the buffer is exceeded.
func (c *Client) loop() {
	defer c.wg.Done()
	flushTicker := time.NewTicker(c.flushTimeout)

	flush := func(msg string, records []*firehose.Record) {
		alog.Info{
			"action":      "flush_buffer",
			"reason":      msg,
			"num_records": len(records),
		}.Log()
		c.semaphore.acquire()
		go c.flush(records)
	}

	for {
		select {
		case <-flushTicker.C:
			c.Lock()
			records := c.buffer.Drain()
			c.Unlock()
			if len(records) > 0 {
				flush("flush_timeout", records)
			}

		case record := <-c.recordQueue:
			records := make([]*firehose.Record, 0)
			c.Lock()
			if c.needToDrain() {
				// drain the buffer
				records = c.buffer.Drain()
			}
			c.buffer.Put(record)
			c.Unlock()

			if len(records) > 0 {
				flush("put_drain", records)
			}

		case <-c.stop:
			// receive stop signal
			flushTicker.Stop()

			// flush remaining data in buffer
			c.Lock()
			records := c.buffer.Drain()
			c.Unlock()

			//c.semaphore.acquire()
			if len(records) > 0 {
				flush("client_stopped", records)
			}

			alog.Info{
				"action": "firehose_client_stop",
				"status": "exit_loop",
			}.Log()
			return
		}
	}
}

func (c *Client) bufferCount() int {
	return c.buffer.Count()
}
