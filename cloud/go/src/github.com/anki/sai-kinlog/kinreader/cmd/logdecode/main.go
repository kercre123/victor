/*
Command logdecode reads log data recorded go the dumpstream command and
reformats it, optionally writing it to multiple files based on
source/sourcetype/host/index metadata.
*/

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	cli "github.com/jawher/mow.cli"
)

func fail(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, format, a...)
	cli.Exit(100)
}

type jsonRecord struct {
	ArrivalTime    time.Time `json:"arrival_time"`
	PartitionKey   string    `json:"partition_key"`
	SequenceNumber string    `json:"seq"`
	Data           []byte    `json:"data"`
}

type logPayload struct {
	entry         *logrecord.LogEntry
	kinesisRecord *jsonRecord
}

type formatter interface {
	format(w io.Writer, p *logPayload) error
}

// rawLogFormatter emits just the raw log data, with no other metadata
// it results in something that looks like a normal log file
type rawLogFormatter struct{}

func (f *rawLogFormatter) format(w io.Writer, p *logPayload) (err error) {
	data := p.entry.Data.Data
	if len(data) == 0 {
		return nil
	}
	if _, err = w.Write(data); err != nil {
		return err
	}
	if data[len(data)-1] != '\n' {
		_, err = w.Write([]byte{'\n'})
	}
	return err
}

type jsonFormat struct {
	logrecord.Metadata
	LogTS          *time.Time `json:"log_timestamp"`
	Data           string     `json:"event"`
	ArrivalTime    time.Time  `json:"approx_arrival_time"`
	PartitionKey   string     `json:"partition_key"`
	SequenceNumber string     `json:"seq"`
}

// jsonLogFormatter emits a JSON object for each event that includes metadata
// that was sent with the event, along with metadata recorded by Kinesis such
// as approximate arrival time.
type jsonLogFormatter struct {
	indent bool
}

func (f *jsonLogFormatter) format(w io.Writer, p *logPayload) (err error) {
	data := &jsonFormat{
		Metadata:       p.entry.Metadata,
		LogTS:          p.entry.Data.Time,
		Data:           string(p.entry.Data.Data),
		ArrivalTime:    p.kinesisRecord.ArrivalTime,
		PartitionKey:   p.kinesisRecord.PartitionKey,
		SequenceNumber: p.kinesisRecord.SequenceNumber,
	}
	var b []byte
	if f.indent {
		b, err = json.MarshalIndent(data, "", "  ")
	} else {
		b, err = json.Marshal(data)
	}
	if err != nil {
		return err
	}
	_, err = w.Write(b)
	w.Write([]byte{'\n'})
	return err
}

type logWriter interface {
	writeEntry(f formatter, p *logPayload) error
}

// streamWriter sends output data to a single linear io.Writer
type streamWriter struct {
	w io.Writer
}

func (sw *streamWriter) writeEntry(f formatter, p *logPayload) error {
	return f.format(sw.w, p)
}

// fileWriter accepts a template to use for constructing filenames
// to write data to.   It will dynamically create required directories
// and send formatted log data to those files.
type fileWriter struct {
	pathTemplate string
	fhByMD       map[logrecord.Metadata]*os.File
	fhByName     map[string]*os.File
}

func newFileWriter(filepath string) *fileWriter {
	return &fileWriter{
		pathTemplate: filepath,
		fhByMD:       make(map[logrecord.Metadata]*os.File),
		fhByName:     make(map[string]*os.File),
	}
}

func (fw *fileWriter) fileForEntry(p *logPayload) (*os.File, error) {
	if fh, ok := fw.fhByMD[p.entry.Metadata]; ok {
		return fh, nil
	}
	md := p.entry.Metadata
	fn := strings.Replace(fw.pathTemplate, "%source%", md.Source, -1)
	fn = strings.Replace(fn, "%sourcetype%", md.Sourcetype, -1)
	fn = strings.Replace(fn, "%host%", md.Host, -1)
	fn = strings.Replace(fn, "%index%", md.Index, -1)

	if fh, ok := fw.fhByName[fn]; ok {
		fw.fhByMD[p.entry.Metadata] = fh
		return fh, nil
	}

	if dir := filepath.Dir(fn); dir != "" {
		if err := os.MkdirAll(dir, 0777); err != nil {
			return nil, fmt.Errorf("failed to create directory %s: %v", dir, err)
		}
	}
	fh, err := os.Create(fn)
	if err != nil {
		return nil, err
	}
	fw.fhByMD[md] = fh
	fw.fhByName[fn] = fh
	return fh, nil
}

func (fw *fileWriter) writeEntry(f formatter, p *logPayload) error {
	w, err := fw.fileForEntry(p)
	if err != nil {
		return err
	}
	return f.format(w, p)
}

func main() {
	app := cli.App("logdecode", "Reads and converts log data dumped by dumpstream")
	app.Spec = "[--target-filepath] [--indent-json] [--format=<raw|json>]"

	targetFilepath := app.StringOpt("f target-filepath", "",
		"Filepath to write to.  Will create any directories as needed and may include "+
			"%source%, %sourcetype%, %host% and %index% variables which will be expanded. "+
			"eg. \"logdata/%index/%source%-%host%.log\".  "+
			"Defaults to stdout")
	format := app.StringOpt("format", "raw", "'raw' or 'json' - Output as raw log data, or as JSON including metadata")
	indent := app.BoolOpt("indent-json", false, "Enables JSON indenting for humanish readable output")

	app.Action = func() {
		var rec jsonRecord
		var data logrecord.Data
		var fmt formatter
		var w logWriter

		switch *format {
		case "raw":
			fmt = &rawLogFormatter{}

		case "json":
			fmt = &jsonLogFormatter{indent: *indent}
		default:
			fail("Invalid value for --format")
		}

		if *targetFilepath != "" {
			w = newFileWriter(*targetFilepath)
		} else {
			w = &streamWriter{os.Stdout}
		}

		scanner := bufio.NewScanner(os.Stdin)
		for scanner.Scan() {
			if err := json.Unmarshal(scanner.Bytes(), &rec); err != nil {
				fail("Failed to decode record: %v", err)
			}
			dec, err := logrecord.NewDecoder(rec.Data)
			if err != nil {
				fail("Failed to create log record decoder: %v", err)
			}
			md := dec.Metadata()
			for {
				err := dec.Decode(&data)
				if err == io.EOF {
					break
				}
				if err != nil {
					fail("Failed to decode log record: %v", err)
				}
				lp := &logPayload{
					kinesisRecord: &rec,
					entry: &logrecord.LogEntry{
						Metadata: md,
						Data:     data,
					},
				}
				if err := w.writeEntry(fmt, lp); err != nil {
					fail("Failed to write entry: %v", err)
				}
			}
		}
	}

	app.Run(os.Args)
}
