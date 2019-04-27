// Copyright 2016 Anki, Inc.

package logsender

import (
	"errors"
	"fmt"
	"log"
	"os"
	"sync"
	"time"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/client"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

const (
	// DefaultMaxParallelFlush sets the maximum number of PutRecords operations
	// that can be in-flight at any given moment.
	DefaultMaxParallelFlush = 3

	// DefaultFlushInterval specifies the maximum amount of time data will be
	// buffered before a flush operation is triggered.
	DefaultFlushInterval = time.Second
)

var (
	// ErrQueueFull is returned by a write operation if flush operations are
	// in progress and the next buffer is full.
	ErrQueueFull = errors.New("log write queue full")
)

var (
	nullMetadata = logrecord.Metadata{
		Source:     "__null__",
		Sourcetype: "__null__",
		Host:       "__null__",
		Index:      "__null__",
	}
	defaultConnectivityRetries = 3
	defaultAWSRetries          = 1000
)

// KinesisPutter describes the interface for sending data to Kinesis.
type KinesisPutter interface {
	PutRecords(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error)
}

// Config provides a configuration option for NewSender.
type Config func(s *Sender) error

// FlushInterval provides NewSender with the duration to wait between
// log flushes to Kinesis.
//
// Overrides DefaultFlushInterval
func FlushInterval(i time.Duration) Config {
	return func(s *Sender) error {
		return s.setFlushInterval(i)
	}
}

// MaxParallelFlush provides NewSender with the maximum number of Kinesis flush
// operations to operate concurrently without blocking.
//
// Overrides DefaultMaxParallelFlush
func MaxParallelFlush(max int) Config {
	return func(s *Sender) error {
		return s.setMaxParallelFlush(max)
	}
}

// WithSession specifies an explicit AWS session to use when the Sender
// creates a kinesis.Kinesis instance.
//
// This can be useful to provide an aws session that's configured to use a
// particular credentials provider, etc.
//
// Has no effect if used in conjunction with WithKinesis.
func WithSession(sess client.ConfigProvider) Config {
	return func(s *Sender) error {
		return s.setSession(sess)
	}
}

// WithKinesis specifies a configured KinesisPutter (typically a *kinesis.Kinesis instance)
// to use to send data to Kinesis.
//
// Uses a default instance if not provided, which obtains configuration information from
// the environment, filesystem or ec2 instance metadata.
func WithKinesis(k KinesisPutter) Config {
	return func(s *Sender) error {
		return s.setKinesis(k)
	}
}

// WithErrorHandler provides NewSender with a custom ErrorHandler.  The default
// ErrorHandler will cause the application to panic if a hard Kinesis error
// occurs.
func WithErrorHandler(h ErrorHandler) Config {
	return func(s *Sender) error {
		return s.setErrorHandler(h)
	}
}

// WithErrorLog provides a logger which will be written to in the event of a
// log transmission error.
func WithErrorLog(lg *log.Logger) Config {
	return func(s *Sender) error {
		return s.setErrorLog(lg)
	}
}

// WithAWSRetries specifies the number of attempts the transmitter will make
// to send log data to Kinesis before failing.
func WithAWSRetries(count int) Config {
	return func(s *Sender) error {
		return s.setAWSRetries(count)
	}
}

// WithConnectivityRetries specifies the number of retries the Sender will make
// at startup to verify its connection with Kinesis.  This should usually be
// a low number so that configuration errors cause a fast failure.
func WithConnectivityRetries(count int) Config {
	return func(s *Sender) error {
		if count < 0 {
			return errors.New("invalid value for WithConnectivityRetries")
		}
		return s.setConnectivityRetries(count)
	}
}

// WithNoConnectivityCheck disables the initial connectivity check made with
// Kinesis when New is called.
func WithNoConnectivityCheck() Config {
	return func(s *Sender) error {
		return s.setConnectivityRetries(-1)
	}
}

func withSizeLimits(sizes pendingSizeLimits) Config {
	return func(s *Sender) error {
		s.sizeLimits = sizes
		return nil
	}
}

func withMaxObjectsPerPut(max int) Config {
	return func(s *Sender) error {
		s.sizeLimits.maxObjectsPerPut = max
		return nil
	}
}

func withMaxSoftRecordSize(max int) Config {
	return func(s *Sender) error {
		s.sizeLimits.maxSoftRecordSize = max
		return nil
	}
}

func withMaxPendingSize(max int) Config {
	return func(s *Sender) error {
		s.sizeLimits.maxPendingSize = max
		return nil
	}
}

func withFlushTicker(ch chan time.Time) Config {
	return func(s *Sender) error {
		s.newTicker = func(d time.Duration) *time.Ticker {
			return &time.Ticker{
				C: ch,
			}
		}
		return nil
	}
}

// Sender collects log data and streams it to Kinesis.
type Sender struct {
	flushInterval       time.Duration
	maxParallelFlush    int
	awsRetries          int
	awsSession          client.ConfigProvider
	connectivityRetries int
	k                   KinesisPutter
	streamName          string
	stats               *stats
	errorHandler        ErrorHandler
	errorLog            *log.Logger

	entryChan chan writeRequest
	stopChan  chan struct{}
	stopWG    sync.WaitGroup
	stopOnce  sync.Once

	// used by tests
	newTicker  func(d time.Duration) *time.Ticker // used by unit tests
	sizeLimits pendingSizeLimits
}

// New creates and configures a new Kinesis Sender.
func New(streamName string, config ...Config) (*Sender, error) {
	s := &Sender{
		streamName:          streamName,
		stats:               new(stats),
		entryChan:           make(chan writeRequest),
		stopChan:            make(chan struct{}),
		maxParallelFlush:    DefaultMaxParallelFlush,
		flushInterval:       DefaultFlushInterval,
		errorHandler:        new(defaultErrorHandler),
		errorLog:            log.New(os.Stdout, "", log.LstdFlags),
		awsRetries:          defaultAWSRetries,
		connectivityRetries: defaultConnectivityRetries,
		newTicker:           time.NewTicker,
		sizeLimits:          defaultPendingSizeLimits,
	}
	for _, c := range config {
		if err := c(s); err != nil {
			return nil, err
		}
	}
	if s.awsSession == nil {
		s.awsSession = session.New()
	}
	connCheckKinesis := s.k
	if s.k == nil {
		retryer := kinesisRetryer{
			DefaultRetryer: client.DefaultRetryer{NumMaxRetries: s.awsRetries},
			errorLog:       s.errorLog,
		}
		cfg := request.WithRetryer(aws.NewConfig(), retryer)
		s.k = kinesis.New(s.awsSession, cfg)
		// reduce retries for initial connectivity check
		connCheckRetryer := kinesisRetryer{
			DefaultRetryer: client.DefaultRetryer{NumMaxRetries: s.connectivityRetries},
			errorLog:       s.errorLog,
		}
		connCheckCfg := request.WithRetryer(aws.NewConfig(), connCheckRetryer)
		connCheckKinesis = kinesis.New(s.awsSession, connCheckCfg)
	}
	if s.connectivityRetries >= 0 {
		if err := checkConnectivity(streamName, connCheckKinesis); err != nil {
			return nil, err
		}
	}
	s.stopWG.Add(1)
	go s.sender()
	return s, nil
}

// Stop triggers shutdown of the sender.  It blocks until all
// outstanding data has been flushed to Kinesis.
func (s *Sender) Stop() {
	s.stopOnce.Do(func() {
		close(s.stopChan)
		s.stopWG.Wait()
	})
}

// Stats returns current Sender statistics.
func (s *Sender) Stats() Stats {
	return s.stats.values()
}

func (s *Sender) setFlushInterval(i time.Duration) error {
	if i <= 0 {
		return errors.New("Invalid flush interval")
	}
	s.flushInterval = i
	return nil
}

func (s *Sender) setAWSRetries(count int) error {
	s.awsRetries = count
	return nil
}

func (s *Sender) setConnectivityRetries(count int) error {
	s.connectivityRetries = count
	return nil
}

func (s *Sender) setMaxParallelFlush(max int) error {
	if max < 0 {
		return errors.New("Invalid value")
	}
	s.maxParallelFlush = max
	return nil
}

func (s *Sender) setSession(sess client.ConfigProvider) error {
	s.awsSession = sess
	return nil
}

func (s *Sender) setKinesis(k KinesisPutter) error {
	s.k = k
	return nil
}

func (s *Sender) setErrorHandler(h ErrorHandler) error {
	s.errorHandler = h
	return nil
}

func (s *Sender) setErrorLog(lg *log.Logger) error {
	s.errorLog = lg
	return nil
}

// flusher runs in a goroutine taking sets of records to flush and dispatching
// them to Kinesis.
func (s *Sender) flusher(pending <-chan *pendingItems) {
	stats := s.stats
	for {
		select {
		case p := <-pending:
			if p == nil {
				s.stopWG.Done()
				return
			}

			if p.rawSize == 0 {
				continue
			}

			for flushed := false; !flushed; {
				flushed = true
				start := time.Now()
				softErrs, hardErr := p.flushToKinesis(s.k, s.streamName)
				delta := int64(time.Since(start) / time.Millisecond)
				stats.addFlushTime(delta)
				for _, err := range softErrs {
					switch err.Code {
					case throughputExceededCode:
						stats.incValue(&stats.ThruputExceededErrors)
					default:
						stats.incValue(&stats.OtherPutErrors)
					}
				}

				if hardErr != nil {
					stats.incValue(&stats.HardErrors)
					switch v := s.errorHandler.FlushFailed(s, hardErr); v {
					case FlushRetry:
						flushed = false
					case FlushDiscard:
					default:
						panic(fmt.Sprintf("Invalid response %#v from FlushFailed", v))
					}
				}
			}
		}
	}
}

var newFlushTicker = func(d time.Duration) *time.Ticker {
	return time.NewTicker(d)
}

// a single sender goroutine reads events to be sent to Kinesis, buffers them
// and sends batches of them a flusher goroutine to be dispatched.
func (s *Sender) sender() {
	// start a couple of goroutines to flush to Kinesis in the background.
	sendChan := make(chan *pendingItems)
	for i := 0; i < s.maxParallelFlush; i++ {
		s.stopWG.Add(1)
		go s.flusher(sendChan)
	}

	// Multiple event entries are compressed into a single Kinesis record
	// newEncoderPool returns a sync.Pool cache of those encoders.
	recPool := newEncoderPool()

	// pending holds a current set of records waiting to be flushed
	// one record is allocated per (source, sourcetype, host, index) metadata key
	pending := newPendingItems(recPool, s.sizeLimits)

	stats := s.stats
	flushTicker := s.newTicker(s.flushInterval)

	flush := func() {
		select {
		case sendChan <- pending:
			stats.incValue(&stats.FlushCount)
			pending = newPendingItems(recPool, s.sizeLimits)

		default:
			// all flushers currently busy; continue adding to existing
			// record encoder and try again when scheduled
			stats.incValue(&stats.FlushFullCount)
		}
	}

	addEntry := func(e logrecord.LogEntry) (err error) {
		if err = pending.addEntry(e, true); err == errPendingIsFull || err == errReachedSoftLimit {
			flush()
			// In case flushing failed, buffer beyond the soft limit
			// record encoder size (256kb) to avoid dropping log data.
			err = pending.addEntry(e, false)
		}
		if err != nil {
			stats.incValue(&stats.DroppedWrites)
		}
		return err
	}

	for {
		select {
		case <-flushTicker.C:
			if !pending.isEmpty() {
				flush()
			}

		case req := <-s.entryChan:
			stats.incValue(&stats.EntryCount)
			req.result <- addEntry(req.entry)

		case <-s.stopChan:
			close(s.entryChan) // ensure future writes fail with a panic rather than blocking
			flushTicker.Stop()
			if !pending.isEmpty() {
				sendChan <- pending // block exiting till flush finishes
			}
			close(sendChan) // ensure flush goroutines exit
			s.stopWG.Done()
			return
		}
	}
}

// write a log entry to Kinesis.
func (s *Sender) write(e logrecord.LogEntry) error {
	// TODO: use a sync.Pool to reduce allocation overhead of all these
	// writeRequests and embedded channels
	r := writeRequest{
		entry:  e,
		result: make(chan error, 1),
	}
	s.entryChan <- r
	return <-r.result
}

// NewEventWriter creates an EventWriter that can be used to stream pre-broken
// events to Kinesis.
//
// If addTimestamp is set to true, then the current time will be sent along with
// the event, which may obviate Splunk's need to parse the data out of the
// event text itself (if it even has one).
func (s *Sender) NewEventWriter(md logrecord.Metadata, addTimestamp bool) *EventWriter {
	md.IsUnbroken = false
	return &EventWriter{
		md:           md,
		addTimestamp: addTimestamp,
		s:            s,
	}
}

type writeRequest struct {
	entry  logrecord.LogEntry
	result chan error
}
