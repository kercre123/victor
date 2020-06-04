package cli

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"strings"
	"testing"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-kinlog/logsender"
	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

func TestSetupStdoutLogging(t *testing.T) {
	var buf bytes.Buffer

	defer func(org string) { logType = org }(logType)
	logType = string(LogToStdout)

	// capture "stdout"
	defer func(org io.Writer) { alog.Stdout = org }(os.Stdout)
	alog.Stdout = &buf

	logType = "stdout"
	SetupLogging()
	defer runCleanupHandlers()

	alog.Info{"action": "unittest_write"}.Log()
	runCleanupHandlers()

	out := buf.String()
	if !strings.Contains(out, "unittest_write") {
		fmt.Println(buf.String())
		t.Fatal("Logger did not emit log entry to stdout")
	}
}

type fakeKinesisPut func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error)

func (f fakeKinesisPut) PutRecords(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
	return f(input)
}

// Fake a Kinesis server and capture log data sent to it.
// Make sure the log record we generate is in there.
func TestSetupKinesisLogging(t *testing.T) {
	dec := captureKinesisRecord(t, func(t *testing.T) {
		alog.Info{"action": "unittest_write"}.Log()
	})

	var data logrecord.Data
	if err := dec.Decode(&data); err != nil {
		t.Fatal("Unepxected error decoding record", err)
	}

	if entry := string(data.Data); !strings.Contains(entry, "unittest_write") {
		t.Error("Did not get expected log data, got: ", entry)
	}
}

// Test that NewAccessLogWriter sets the correct source and sourcetype
func TestNewAccessLogWriter(t *testing.T) {
	LogMetadata.Source = "atest"

	dec := captureKinesisRecord(t, func(t *testing.T) {
		w := NewAccessLogWriter("")
		fmt.Fprintf(w, "unittest_write")
	})

	// Check metadata
	md := dec.Metadata()
	expectedSource := "atest_access_log"
	expectedSourcetype := "sai_access_combined"

	if md.Source != expectedSource {
		t.Errorf("Incorrect source expected=%q actual=%q", expectedSource, md.Source)
	}
	if md.Sourcetype != expectedSourcetype {
		t.Errorf("Incorrect sourcetype expected=%q actual=%q", expectedSourcetype, md.Sourcetype)
	}

	var data logrecord.Data
	if err := dec.Decode(&data); err != nil {
		t.Fatal("Unepxected error decoding record", err)
	}

	if entry := string(data.Data); !strings.Contains(entry, "unittest_write") {
		t.Error("Did not get expected log data, got: ", entry)
	}
}

// Setup Kinesis logging and run function f in that context
// returns a decoder for the first non-test probe record sent
func captureKinesisRecord(t *testing.T, f func(t *testing.T)) *logrecord.Decoder {
	defer func(org string) { logType = org }(logType)
	logType = string(LogToKinesis)

	defer func(org func(stream string, config ...logsender.Config) (*logsender.Sender, error)) {
		newKinesisSender = org
	}(newKinesisSender)

	var records []*kinesis.PutRecordsRequestEntry
	fakePut := fakeKinesisPut(func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		records = append(records, input.Records...)
		return &kinesis.PutRecordsOutput{Records: []*kinesis.PutRecordsResultEntry{{}}}, nil
	})

	newKinesisSender = func(stream string, config ...logsender.Config) (*logsender.Sender, error) {
		return logsender.New(stream, logsender.WithKinesis(fakePut))
	}

	SetupLogging()
	defer runCleanupHandlers()

	// run user code
	f(t)

	runCleanupHandlers()

	// first record is the sender's test probe
	if len(records) < 2 {
		t.Fatalf("Incorrect record count.  expected>=2 actual=%d", len(records))
	}

	// second record should contain our log entry
	d, err := logrecord.NewDecoder(records[1].Data)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	return d
}
