/*
Command dumpstream reads entries from a Kinesis stream
and writes them to stdout or file as JSON
*/
package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"sync"
	"time"

	"github.com/anki/sai-kinlog/kinreader"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
	cli "github.com/jawher/mow.cli"
	"github.com/juju/ratelimit"
)

func fail(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, format, a...)
	cli.Exit(100)
}

const (
	emitRaw  = "raw"
	emitJSON = "json"
)

type jsonRecord struct {
	ArrivalTime    time.Time `json:"arrival_time"`
	PartitionKey   string    `json:"partition_key"`
	SequenceNumber string    `json:"seq"`
	Data           []byte    `json:"data"`
}

type processor struct {
	format string
	rl     *ratelimit.Bucket
	m      sync.Mutex
	target io.Writer
}

func newProcessor(format string, target io.Writer, maxPerSecond int) (*processor, error) {
	if format != emitRaw && format != emitJSON {
		return nil, errors.New("invalid data format")
	}
	return &processor{
		format: format,
		rl:     ratelimit.NewBucketWithRate(float64(maxPerSecond), 5),
		target: target,
	}, nil
}

func (p *processor) ProcessRecords(shard *kinreader.Shard, records []*kinesis.Record) error {
	var last string
	p.rl.Wait(1)

	for _, rec := range records {
		switch p.format {
		case emitRaw:
			if err := p.emitRaw(rec); err != nil {
				return err
			}
		case emitJSON:
			if err := p.emitJSON(rec); err != nil {
				return err
			}
		default:
			panic("Invalid format: " + p.format)
		}
		last = aws.StringValue(rec.SequenceNumber)
	}
	return shard.Checkpoint(last)
}

func (p *processor) emitRaw(rec *kinesis.Record) error {
	p.m.Lock()
	defer p.m.Unlock()
	_, err := p.target.Write(rec.Data)
	return err
}

func (p *processor) emitJSON(rec *kinesis.Record) error {
	// encode before acquiring lock
	jrec := &jsonRecord{
		ArrivalTime:    aws.TimeValue(rec.ApproximateArrivalTimestamp),
		PartitionKey:   aws.StringValue(rec.PartitionKey),
		SequenceNumber: aws.StringValue(rec.SequenceNumber),
		Data:           rec.Data,
	}
	data, err := json.Marshal(jrec)
	if err != nil {
		return err
	}
	data = append(data, '\n')
	p.m.Lock()
	defer p.m.Unlock()
	_, err = p.target.Write(data)
	return err

}

func main() {
	app := cli.App("dumpstream", "Dumps the content from a Kinesis stream")

	app.Spec = "--stream-name [--checkpoint-file] [--data-format] [--filename] [--max-rate] [--default-to-horizon]"

	stream := app.String(cli.StringOpt{
		Name:   "stream-name",
		EnvVar: "LOG_KINESIS_STREAM",
		Desc:   "Kinesis stream name to read from",
	})
	checkPointFile := app.String(cli.StringOpt{
		Name:   "checkpoint-file",
		EnvVar: "CHECKPOINT_FILE",
		Desc:   "File to store checkpoint information.  Default to in-memory checkpointing",
	})
	dataFormat := app.String(cli.StringOpt{
		Name:   "data-format",
		Value:  "json",
		EnvVar: "DATA_FORMAT",
		Desc:   `Which format to emit the data in - either "json" or "raw"`,
	})
	filename := app.String(cli.StringOpt{
		Name:   "filename",
		EnvVar: "TARGET_FILENAME",
		Desc:   "Which file to write the output to; defaults to stdout",
	})
	maxRate := app.Int(cli.IntOpt{
		Name:   "max-rate",
		EnvVar: "MAX-RATE",
		Desc:   "Maximum number of requests to Kinesis to make per second (Kinesis supports 5 reads/second/shard)",
		Value:  3,
	})
	defaultToHorizon := app.Bool(cli.BoolOpt{
		Name:   "default-to-horizon",
		Value:  false,
		EnvVar: "DEFAULT_TO_HORIZION",
		Desc:   "Enable to cause data to start reading from the earliest available point in the stream, rather than the latest if no checkpoint file exists",
	})

	app.Action = func() {
		var w io.Writer
		var err error

		logger := log.New(os.Stderr, "", log.LstdFlags)

		if *filename == "" {
			w = os.Stdout
		} else {
			w, err = os.Create(*filename)
			if err != nil {
				fail("Failed to open %s for write: %v", *filename, err)
			}
		}

		sp, err := newProcessor(*dataFormat, w, *maxRate)
		if err != nil {
			fail("%v", err)
		}

		defaultBehavior := kinreader.DefaultToLatest()
		if *defaultToHorizon {
			defaultBehavior = kinreader.DefaultToHorizon()
		}

		cfg := []kinreader.Config{
			defaultBehavior,
			kinreader.WithLogger(logger),
			kinreader.WithProcessor(sp),
		}

		if *checkPointFile != "" {
			cp, err := kinreader.NewFileCheckpointer(*checkPointFile)
			if err != nil {
				fail("Failed to open checkpoint file: %v", err)
			}
			cfg = append(cfg, kinreader.WithCheckpointer(cp))
		}

		r, err := kinreader.New(*stream, cfg...)
		if err != nil {
			fail("Failed to open reader %v", err)
		}
		err = r.Run()
		if err != nil {
			fail("Failed to process stream: %v", err)
		}
	}

	app.Run(os.Args)
}
