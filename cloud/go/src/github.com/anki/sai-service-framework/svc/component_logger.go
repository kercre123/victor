package svc

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path"
	"time"

	"github.com/jawher/mow.cli"
	"github.com/stoewer/go-strcase"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-kinlog/logsender"
	"github.com/anki/sai-kinlog/logsender/logrecord"
)

const (
	DefaultLogSourcetype = "sai_go_general"
	LogToKinesis         = "kinesis"
	LogToStdout          = "stdout"
	DefaultLogType       = LogToStdout
)

var DefaultLogSource = toLogSource(os.Args[0])

func toLogSource(exe string) string {
	return strcase.SnakeCase(path.Base(exe))
}

func init() {
	alog.ToStdout()
}

// LogComponent is a service object component which configures and
// initializes the Anki log subsystem, with support for configuring
// stdout or Kinesis based logging.
type LogComponent struct {
	// logType specifies the output destination for logging - either
	// "kinesis" or "stdout"
	logType *string

	// level sets the logging threshold. One of "DEBUG", "INFO",
	// "WARN", "ERROR", or "PANIC"
	level *string

	// kinesisStream names the Kinesis stream to send logs to if the
	// Type is "kinesis".
	kinesisStream *string

	// source sets the name to use in the source field for logs. The
	// name of the service is typical.
	source *string

	// sourceType sets the name to use in the source_type field for
	// logs. The default value of sai_go_general should not be
	// changed.
	sourceType *string

	// index sets which Splunk index the logs will be stored in.
	index *string

	// metadata holds the default base log metadata to send to
	// Kinesis
	metadata logrecord.Metadata

	// sender handles the actual transport of logs to Kinesis.
	sender *logsender.Sender

	shutdownHandler func()
}

func (c *LogComponent) CommandSpec() string {
	return "[--log-type] [--log-level] [--log-kinesis-stream] [--log-source] [--log-sourcetype] [--log-index]"
}

// Initialize the command-line options for setting up logging
func (c *LogComponent) CommandInitialize(cmd *cli.Cmd) {
	c.logType = StringOpt(cmd, "log-type", DefaultLogType,
		`Either "kinesis" or "stdout"`)
	c.level = StringOpt(cmd, "log-level", "INFO",
		`One of "DEBUG", "INFO", "WARN", "ERROR", or "PANIC"`)
	c.kinesisStream = StringOpt(cmd, "log-kinesis-stream", "",
		"Kinesis stream to send logs to")
	c.source = StringOpt(cmd, "log-source", DefaultLogSource,
		"Source to allocate to the general log")
	c.sourceType = StringOpt(cmd, "log-sourcetype", DefaultLogSourcetype,
		"Sourcetype to allocate to the general log")
	c.index = StringOpt(cmd, "log-index", "",
		"Index to allocate to the general log")
}

// Validate the logging config and initialize the logging subsystem.
func (c *LogComponent) Start(ctx context.Context) error {
	alog.Debug{
		"action": "service_object_start",
		"status": alog.StatusStart,
		"name":   "core.logging",
	}.Log()

	// validate
	if *c.logType != LogToStdout && *c.logType != LogToKinesis {
		return errors.New(fmt.Sprintf(`Unknown LOG_TYPE "%s"`, *c.logType))
	}

	if *c.logType == LogToKinesis && *c.kinesisStream == "" {
		return errors.New(fmt.Sprintf("LOG_TYPE=kinesis, but LOG_KINESIS_STREAM was not specified"))
	}

	if *c.level != "" {
		if err := alog.SetLevelByString(*c.level); err != nil {
			return errors.New(fmt.Sprintf(`Invalid LOG_LEVEL "%s"`, *c.level))
		}
	}

	if *c.source != "" {
		c.metadata.Source = *c.source
	} else {
		c.metadata.Source = DefaultLogSource
	}

	if *c.sourceType != "" {
		c.metadata.Sourcetype = *c.sourceType
	} else {
		c.metadata.Sourcetype = DefaultLogSourcetype
	}

	// initialize
	if commit := buildinfo.Info().Commit; len(commit) > 8 {
		alog.Commit = commit[:8]
	} else {
		alog.Commit = "unknown"
	}

	switch *c.logType {
	case LogToKinesis:
		c.initKinesisLogging()
	case LogToStdout:
		c.initStdoutLogging()
	}

	return nil
}

func (c *LogComponent) Stop() error {
	if c.shutdownHandler != nil {
		c.shutdownHandler()
	}
	return nil
}

func (c *LogComponent) Kill() error {
	return nil
}

// replaced by unit tests
var newKinesisSender = func(stream string) (*logsender.Sender, error) {
	return logsender.New(stream)
}

func (c *LogComponent) initKinesisLogging() {
	sender, err := newKinesisSender(*c.kinesisStream)
	if err != nil {
		Fail("Failed to setup Kinesis logger: %s", err)
	}

	// log the kinesis send stats every minute
	monitor := &logsender.Monitor{
		LogInterval: time.Minute,
		Logger:      alog.MakeLogger(alog.LogInfo, "kinlog_stats"),
		Sender:      sender,
	}
	monitor.Start()

	if hostname, err := os.Hostname(); err == nil {
		c.metadata.Host = hostname
	}

	alog.ToWriter(sender.NewEventWriter(c.metadata, false))
	c.sender = sender
	fmt.Printf("Sending log data to Kinesis stream=%s sourcetype=%s source=%s\n", *c.kinesisStream, *c.sourceType, *c.source)
	c.shutdownHandler = func() {
		fmt.Print("Shutting down Kinesis logger...")
		alog.ToStdout()
		monitor.Stop()
		c.sender.Stop()
		fmt.Println("Done.")
	}
}

func (c *LogComponent) initStdoutLogging() {
	fmt.Printf("Sending log data to stdout sourcetype=%s source=%s\n", *c.sourceType, *c.source)
	alog.ToStdout()
}

func (c *LogComponent) String() string {
	return fmt.Sprintf("&{type: %s, level: %s, stream: %s, source: %s, sourcetype: %s, index: %s}",
		*c.logType,
		*c.level,
		*c.kinesisStream,
		*c.source,
		*c.sourceType,
		*c.index)
}
