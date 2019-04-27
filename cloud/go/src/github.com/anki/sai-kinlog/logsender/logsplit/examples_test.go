package logsplit_test

import (
	"fmt"
	"regexp"
	"strings"
	"time"

	"github.com/anki/sai-kinlog/logsender/logsplit"
)

type prefixWriter struct{}

func (w prefixWriter) Write(p []byte) (int, error) {
	fmt.Println("----")
	fmt.Print(string(p))
	return len(p), nil
}

func ExampleLogSplitter_multiline() {
	// define our sample events
	input := `02 May 2017 14:01:02 This is event one
It spans two lines.
02 May 2017 14:02:00 This is event two

It span three lines.
02 May 2017 14:02:30 This is a single line event.`

	// define a regular expression to match lines beginning with a
	// timestamp such as "02 May 2017 14:01:02"
	r := regexp.MustCompile(`^\d{2} \w{3} \d{4} \d{2}:\d{2}:\d{2}`)

	// create the splitter
	ls := &logsplit.LogSplitter{
		Splitter: logsplit.NewRegexSplitter(r),
	}

	// prefixWriter ads "----\n" before every Write it receives
	w := new(prefixWriter)

	ls.Copy(strings.NewReader(input), w)

	// Output:
	// ----
	// 02 May 2017 14:01:02 This is event one
	// It spans two lines.
	// ----
	// 02 May 2017 14:02:00 This is event two
	//
	// It span three lines.
	// ----
	// 02 May 2017 14:02:30 This is a single line event.
}

func ExampleLogSplitter_addTimestamp() {
	// split every input line into a single event
	// and add a timestamp to each line.

	input := `Line one
Line two

Line three after blank line
`
	// create the splitter
	ls := &logsplit.LogSplitter{
		Splitter:  new(logsplit.LineSplitter),
		Timestamp: time.RFC1123,

		// Override TimeSrc so this test gets a predictable timestamp
		TimeSrc: func() time.Time { return time.Date(2017, 5, 1, 12, 0, 0, 0, time.UTC) },
	}

	// prefixWriter ads "----\n" before every Write it receives
	w := new(prefixWriter)

	ls.Copy(strings.NewReader(input), w)

	// Output:
	// ----
	// Mon, 01 May 2017 12:00:00 UTC Line one
	// ----
	// Mon, 01 May 2017 12:00:00 UTC Line two
	// ----
	// Mon, 01 May 2017 12:00:00 UTC Line three after blank line
}
