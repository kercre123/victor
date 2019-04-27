// Copyright 2016 Anki, Inc.

package kinreader

import (
	"fmt"
	"log"
	"sync"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

type iteratorOffset string

const (
	streamActive   = "ACTIVE"
	streamUpdating = "UPDATING"
)

const (
	latestIterator           iteratorOffset = "LATEST"
	afterSequenceNumIterator iteratorOffset = "AFTER_SEQUENCE_NUMBER"
	trimHorizonIterator      iteratorOffset = "TRIM_HORIZON"
)

var (
	// DefaultKinesisMaxRetries specifies the value to provide a default Kinesis
	// handler for maximum retry count.  Failure beyond that count will cause the reader
	// to exit.
	DefaultKinesisMaxRetries = 2
)

var (
	// DefaultProcessor sends record metadata from shards to the default logger
	// Normally this is overriden by using WithProcessor in combination with
	// NewReader.
	DefaultProcessor = new(ShardLogger)
)

// KinesisDescribeStream is implemented by *kinesis.Kinesis
type KinesisDescribeStream interface {
	DescribeStream(input *kinesis.DescribeStreamInput) (*kinesis.DescribeStreamOutput, error)
}

// KinesisGetRecords is implemented by *kinesis.Kinesis
type KinesisGetRecords interface {
	GetRecords(input *kinesis.GetRecordsInput) (*kinesis.GetRecordsOutput, error)
}

// KinesisGetShardIterator is implemented by *kinesis.Kinesis
type KinesisGetShardIterator interface {
	GetShardIterator(input *kinesis.GetShardIteratorInput) (*kinesis.GetShardIteratorOutput, error)
}

// KinesisReader describes the subset of *kinesis.Kinesis needed for a Reader.
type KinesisReader interface {
	KinesisDescribeStream
	KinesisGetShardIterator
	KinesisGetRecords
}

// Config defines a configuration option for Reader.
type Config func(sp *Reader) error

// WithKinesis specifies a KinesisReader instance to use with a Reader.
//
// If omitted then the KinesisReader will attempt to configure a connection
// to Kinesis using the machine's environment.
func WithKinesis(k KinesisReader) Config {
	return func(r *Reader) error {
		return r.setKinesis(k)
	}
}

// WithProcessor specifies a ShardProcessor to send records to.  A client
// application will implement this to process data read from the stream in
// some way.
//
// If it's not provided then DefaultProcessor will be used.
func WithProcessor(p ShardProcessor) Config {
	return func(r *Reader) error {
		return r.setProcessor(p)
	}
}

// WithLogger specifies an optional logger to send periodic status
// updates to.
func WithLogger(l *log.Logger) Config {
	return func(r *Reader) error {
		return r.setLogger(l)
	}
}

// WithCheckpointer provides a store to hold the current read offset for
// each shard within a Kinesis stream.  This allows the reader to pickup
// where it left off after a restart.
func WithCheckpointer(cp Checkpointer) Config {
	return func(r *Reader) error {
		return r.setCheckpointer(cp)
	}
}

// DefaultToLatest configures a Reader to start reading data from the latest
// point in a shard (ie. the latest data written to it), if no checkpoint
// already exists.  This will cause the reader to ignore any existing data
// in the shard if it has never read from it before.  This is the default
// behaviour.
func DefaultToLatest() Config {
	return func(r *Reader) error {
		return r.setStartAt(latestIterator)
	}
}

// DefaultToHorizon configures a Reader to start reading data from the trim
// horizion point in a shard (ie. the earliest available record), if no
// checkpoint already exists.  This will cause the reader to read all available
// data in a shard if it has never read from it before.
func DefaultToHorizon() Config {
	return func(r *Reader) error {
		return r.setStartAt(trimHorizonIterator)
	}
}

// A Reader reads data from a single Kinesis stream (which may consist of
// many shards) and dispatches records to a ShardProcessor.
//
// The Reader starts a goroutine for each shard it's currently processing and
// concurrently dispatches blocks of Kinesis records to the configured ShardProcessor.
type Reader struct {
	stream       string         // the name of the Kinesis stream to read from
	k            KinesisReader  // a *kinesis.Kinesiss instance typically
	processor    ShardProcessor // what's actually going to process the data
	checkpointer Checkpointer   // track shard progress
	startAt      iteratorOffset // where in the shard to start reading from, if no checkpoint
	log          *log.Logger    // optional logger

	shardStatus chan shardStatus // passes info between individual processShard goroutines and the main dispatch loop
	stopm       sync.Mutex
	stop        bool
	stopShards  chan struct{}                     // closed during shutdown to signal processors to stop
	newTimer    func(d time.Duration) *time.Timer // used by unit tests
}

// New creates a NewReader to receive data from the specified Kinesis stream.
//
// It will default to using an in-memory checkpoint tracker, reading from the latest
// items in the stream and a default *kinesis.Kinesis instance (which obtains its
// configured from the environment, filesystem or ec2 instance variables).
func New(streamName string, config ...Config) (r *Reader, err error) {
	sp := &Reader{
		stream:    streamName,
		processor: DefaultProcessor,
		startAt:   latestIterator,

		shardStatus: make(chan shardStatus),
		stopShards:  make(chan struct{}),
		newTimer:    time.NewTimer,
	}

	for _, c := range config {
		if err := c(sp); err != nil {
			return nil, err
		}
	}

	if sp.k == nil {
		cfg := &aws.Config{
			MaxRetries: aws.Int(DefaultKinesisMaxRetries),
		}
		sp.k = kinesis.New(session.New(cfg))
	}

	if sp.checkpointer == nil {
		if sp.checkpointer, err = NewMemCheckpointer(); err != nil {
			return nil, err
		}
	}

	return sp, nil
}

func (r *Reader) setLogger(l *log.Logger) error {
	r.log = l
	return nil
}

func (r *Reader) setKinesis(k KinesisReader) error {
	r.k = k
	return nil
}

func (r *Reader) setProcessor(p ShardProcessor) error {
	r.processor = p
	return nil
}

func (r *Reader) setStartAt(start iteratorOffset) error {
	r.startAt = start
	return nil
}

func (r *Reader) setCheckpointer(cp Checkpointer) error {
	r.checkpointer = cp
	return nil
}

func (r *Reader) info(format string, a ...interface{}) {
	if r.log != nil {
		r.log.Printf(format, a...)
	}
}

// Run starts the stream processor.  It returns after a fatal error occurs,
// or after a call to Stop() has caused all shard processors to complete.
func (r *Reader) Run() error {
	return r.dispatch()
}

// Stop will request a clean shutdown of the reader.  It does not block and
// is idempotent.
func (r *Reader) Stop() {
	r.stopm.Lock()
	defer r.stopm.Unlock()
	if !r.stop {
		r.stop = true
		close(r.stopShards)
	}
}

// initialize shard list and find read offset based on current checkpoints
// and default start position
func (r *Reader) init() (shardMap, Checkpoints, error) {
	shardMap := make(shardMap)
	streamStatus, shards, err := r.getShards()
	if err != nil {
		return nil, nil, err
	}
	if streamStatus != streamActive && streamStatus != streamUpdating {
		return nil, nil, fmt.Errorf("Stream not in ACTIVE or UPDATING state (state=%s)", streamStatus)
	}
	shardMap.update(r, shards)

	cp, err := r.checkpointer.AllCheckpoints()
	if err != nil {
		return nil, nil, err
	}

	if len(cp) != 0 || r.startAt != latestIterator {
		return shardMap, cp, nil
	}

	// if no cp and set == latest, close all except open shard
	// set their stream iterator to latet
	// if latest then mark all open shards at latest
	for _, s := range shardMap {
		if s.isClosed() {
			s.checkpointClosed()
		} else {
			s.defaultToLatest = true
		}
	}
	return shardMap, cp, nil
}

// dispatch is the main loop that pulls data from Kinesis shards and
// distributes the records to ShardProcessors.
func (r *Reader) dispatch() (dispatcherr error) {
	var (
		active int
	)

	// ensure that we stop and block on running shard processors before exiting
	defer func() {
		r.Stop()
		r.info("action=shutdown_wait shards_outstanding=%d", active)
		for active > 0 {
			select {
			case status := <-r.shardStatus:
				active--
				status.shard.processingRunning = false
			}
		}
	}()

	shardMap, cp, err := r.init()
	if err != nil {
		return err
	}

	for {
		streamStatus, shards, err := r.getShards()
		if err != nil {
			return err
		}

		if streamStatus != streamActive && streamStatus != streamUpdating {
			return fmt.Errorf("Stream not in ACTIVE or UPDATING state (state=%s)", streamStatus)
		}

		// add new shards, delete old
		shardMap.update(r, shards)
		r.info("action=start Processing %d shards", len(shardMap))

		// start any required processors
		active += r.startShardProcessors(cp, shardMap)

		select {
		case <-r.stopShards:
			r.info("action=start_shutdown")
			return nil

		case status := <-r.shardStatus:
			active--
			shardId := *status.shard.ShardId
			status.shard.processingRunning = false
			r.info("action=processor_exited shard_id=%s finished=%t failed=%t error=%v\n",
				shardId, status.isFinished, status.failError != nil, status.failError)
			if err := status.failError; err != nil {
				return err
			}
			if status.isFinished {
				status.shard.localFinished = true
			}

			// don't loop if shutdown in progress
			select {
			case <-r.stopShards:
				return nil
			default:
			}
		}
	}
}

func (r *Reader) startShardProcessors(cp Checkpoints, sm shardMap) (started int) {
	for shardId, s := range sm {
		if s.processingRunning {
			// already processing this shard; no need to look further
			r.info("action=start_shard_processor status=skip "+
				"reason=already_running shard_id=%s", aws.StringValue(s.ShardId))
			continue
		}

		lastCheckpoint, hasCheckpoint := cp[shardId]

		// If either of the parent shards are still open, or we still have data to read from it
		// then skip this child shard until that parent is completed to maintain data order
		if !sm.isParentFinished(s, cp) || !sm.isAdjacentParentFinished(s, cp) {
			r.info("action=start_shard_processor status=skip "+
				"reason=unfinished_parents shard_id=%s",
				aws.StringValue(s.ShardId))
			continue
		}

		// If the shard is closed and we've already processed all the data in it, skip it
		if s.isFinished(lastCheckpoint) {
			r.info("action=start_shard_processor status=skip "+
				"reason=finished shard_id=%s",
				aws.StringValue(s.ShardId))
			continue

		}

		// else start a new processor
		var offset string
		if hasCheckpoint {
			offset = lastCheckpoint.Checkpoint
		}

		s.processingRunning = true
		r.info("action=start_processor shard_id=%s", aws.StringValue(s.ShardId))
		go r.processShard(s, offset)
		started++
	}
	return started
}

func (r *Reader) getShardIterator(s *Shard, offset string) (iterator *string, err error) {
	ipt := &kinesis.GetShardIteratorInput{
		ShardId:    s.ShardId,
		StreamName: aws.String(r.stream),
	}
	if offset == "" {
		if s.defaultToLatest {
			ipt.ShardIteratorType = aws.String(string(latestIterator))
		} else {
			ipt.ShardIteratorType = aws.String(string(trimHorizonIterator))
		}
	} else {
		ipt.ShardIteratorType = aws.String(string(afterSequenceNumIterator))
		ipt.StartingSequenceNumber = aws.String(offset)
	}

	resp, err := r.k.GetShardIterator(ipt)
	if err != nil {
		return nil, err
	}
	r.info("action=get_shard_iterator shard=%s type=%s start=%s\n",
		*s.ShardId, aws.StringValue(ipt.ShardIteratorType), offset)
	return resp.ShardIterator, nil
}

// processShard reads records from a shard and dispatches them to the configured ShardProcessor.
// It is executed in its own goroutine by startShardProcessors.
func (r *Reader) processShard(s *Shard, offset string) {
	fail := func(err error) {
		r.shardStatus <- shardStatus{
			shard:     s,
			failError: err,
			isStopped: true,
		}
	}

	it, err := r.getShardIterator(s, offset)
	if err != nil {
		fail(fmt.Errorf("getShardIterator failed: %s", err))
		return
	}

	for {
		if it == nil {
			// have read all the data in the shard; now CLOSED.
			// (this shard has been split or merged)
			r.info("action=shard_finished shard_id=%s", aws.StringValue(s.ShardId))
			r.shardStatus <- shardStatus{
				shard:      s,
				isFinished: true,
				isStopped:  true,
			}
			return
		}

		ipt := &kinesis.GetRecordsInput{
			// Don't specify a limit; GetRecords will return up to 10MB of data
			ShardIterator: it,
		}

		recs, err := r.k.GetRecords(ipt)
		if err != nil {
			fail(fmt.Errorf("GetRecords failed: %s", err))
			return
		}

		if len(recs.Records) > 0 {
			r.info("action=shard_got_records shard_id=%s record_count=%d time_behind_ms=%d",
				aws.StringValue(s.ShardId), len(recs.Records), aws.Int64Value(recs.MillisBehindLatest))
		}

		if err := r.processor.ProcessRecords(s, recs.Records); err != nil {
			fail(fmt.Errorf("ProcessRecords failed: %s", err))
			return
		}
		it = recs.NextShardIterator

		// TODO: send shard stats to parent
		t := r.newTimer(time.Second) // TODO: make dynamic
		select {
		case <-r.stopShards:
			r.shardStatus <- shardStatus{
				shard:     s,
				isStopped: true,
			}
			return

		case <-t.C:
		}
	}
}

// getShards fetches the latest shard list for the stream.
func (r *Reader) getShards() (streamStatus string, shards []*kinesis.Shard, err error) {
	var start *string
	for {
		ipt := &kinesis.DescribeStreamInput{
			ExclusiveStartShardId: start,
			StreamName:            aws.String(r.stream),
		}
		resp, err := r.k.DescribeStream(ipt)
		if err != nil {
			r.info("DescribeStreams failed: %s", err)
			return "", nil, err
		}
		stream := resp.StreamDescription
		streamStatus = aws.StringValue(stream.StreamStatus)
		shards = append(shards, stream.Shards...)
		if aws.BoolValue(stream.HasMoreShards) {
			start = stream.Shards[len(stream.Shards)-1].ShardId
		} else {
			break
		}
	}
	return streamStatus, shards, nil
}

type shardStatus struct {
	shard      *Shard
	isStopped  bool
	isFinished bool
	failError  error
}
