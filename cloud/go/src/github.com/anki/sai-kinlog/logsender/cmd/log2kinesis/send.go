package main

import (
	"io"
	"os"
	"os/signal"
	"regexp"
	"syscall"
	"time"

	"github.com/anki/sai-kinlog/logsender"
	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/anki/sai-kinlog/logsender/logsplit"
	"github.com/gwatts/flagvals"
	cli "github.com/jawher/mow.cli"
)

func sendCommand(app *cli.Cli) {
	app.Command("send", "Read events from stdin and send to Kinesis", func(cmd *cli.Cmd) {
		cmd.Spec = "--stream-name --source [--sourcetype] [--host] [--index] " +
			"[--timestamp] [--capture] " +
			"[--split-mode] [--split-regex] [--max-lines] [CMD [ARGS...]]"

		stream := cmd.String(cli.StringOpt{
			Name:   "stream-name",
			EnvVar: "LOG_KINESIS_STREAM",
			Desc: `Kinesis stream name to send to.  ` +
				`Set to "testsplit" to print split events to stdout and ` +
				`validate --split-mode settings`,
		})
		source := cmd.String(cli.StringOpt{
			Name:   "source",
			EnvVar: "LOG_SOURCE",
			Desc:   "Source name to assign to log data",
		})
		sourcetype := cmd.String(cli.StringOpt{
			Name:   "sourcetype",
			EnvVar: "LOG_SOURCETYPE",
			Desc:   "Sourcetype to assign to log data",
			Value:  defaultSourceType,
		})
		host := cmd.String(cli.StringOpt{
			Name:   "host",
			EnvVar: "LOG_HOST",
			Desc:   "Host name to assign to log data",
			Value:  hostname,
		})
		index := cmd.String(cli.StringOpt{
			Name:   "index",
			EnvVar: "LOG_INDEX",
			Desc:   "Index to send log data to",
		})
		timestampFormat := cmd.String(cli.StringOpt{
			Name:   "timestamp",
			EnvVar: "TIMESTAMP_FMT",
			Desc: "Prefix events with specified timestamp format; " +
				"set to empty string to omit timestamp.",
			Value: time.RFC3339,
		})
		capture := flagvals.NewOneOfString(stdout, stderr)
		capture.Value = stdout
		cmd.Var(cli.VarOpt{
			Name:   "capture",
			EnvVar: "CMD_CAPTURE",
			Desc:   "File descriptor to capture for executed commands.  Either stdout or stderr",
			Value:  capture,
		})
		splitMode := flagvals.NewOneOfString("none", "newline", "regex")
		splitMode.Value = "newline"
		cmd.Var(cli.VarOpt{
			Name:   "split-mode",
			EnvVar: "SPLIT_MODE",
			Desc: `Set event splitting method.  If "none" then the entire input ` +
				`will be treated as a single event.  ` +
				`If "newline" then each line will be treated as a single event.  ` +
				`If "regex" then events will be split at lines beginning with the supplied pattern.`,
			Value: splitMode,
		})
		splitRegex := cmd.String(cli.StringOpt{
			Name:   "split-regex",
			EnvVar: "SPLIT_REGEX",
			Desc:   "Sets the regular expression to use to split events when split-mode is set to regex",
			Value:  `^\d{4}[/-]\d{2}[/-]\d{2}`, // 2017-05-01 or 2017/05/01
		})
		maxEventLines := cmd.Int(cli.IntOpt{
			Name:   "max-lines",
			EnvVar: "MAX_EVENT_LINES",
			Desc:   "The maximum number of lines to buffer for a single event when used with split-mode=(none|regex)",
			Value:  1000,
		})

		exec := cmd.StringArg("CMD", "", "Optional command to execute")
		execargs := cmd.StringsArg("ARGS", nil, "The command arguments")

		cmd.Action = func() {
			var (
				r        io.Reader = os.Stdin
				writer   io.WriteCloser
				sender   *logsender.Sender
				err      error
				shutdown func(force bool) (exitCode int)
			)

			if *exec != "" {
				var rd io.Reader
				shutdown, rd, err = execCommand(*exec, *execargs, capture.Value)
				if err != nil {
					fail("Failed to start command: %v", err)
				}
				r = rd
				defer func() {
					cli.Exit(shutdown(false))
				}()
			}

			md := logrecord.Metadata{
				IsUnbroken: false,
				Source:     *source,
				Sourcetype: *sourcetype,
				Host:       *host,
				Index:      *index,
			}

			if *stream == "testsplit" {
				writer = new(testEventWriter)

			} else {
				sender, err = logsender.New(*stream)
				if err != nil {
					fail("Failed to initialize Kinesis sender: %v", err)
				}

				writer = sender.NewEventWriter(md, false)
			}

			logSplitter := &logsplit.LogSplitter{
				Timestamp:     *timestampFormat,
				MaxEventLines: *maxEventLines,
			}

			switch splitMode.Value {
			case "none":
				logSplitter.Splitter = new(logsplit.NoSplit)

			case "newline":
				logSplitter.Splitter = new(logsplit.LineSplitter)

			case "regex":
				r, err := regexp.Compile(*splitRegex)
				if err != nil {
					fail("Invalid regular expression: %s", err)
				}
				logSplitter.Splitter = logsplit.NewRegexSplitter(r)

			default:
				fail("Invalid setting for split-mode")
			}

			// install a signal handler so we can terminate cleanly
			sigchan := make(chan os.Signal)
			signal.Notify(sigchan, os.Interrupt, syscall.SIGTERM)
			go func() {
				<-sigchan
				if shutdown != nil {
					shutdown(true) // force shutdown
				}
				writer.Close()
			}()

			if err := logSplitter.Copy(r, writer); err != nil {
				fail("Failed to copy log data: %v", err)
			}

			if sender != nil {
				sender.Stop()
			}
		}
	})
}
