package logsplit

import (
	"regexp"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

type capture struct {
	bufs []string
}

func (c *capture) Write(b []byte) (int, error) {
	c.bufs = append(c.bufs, string(b))
	return len(b), nil
}

var splitTests = []struct {
	input    string
	expected []string
	splitter EventSplitter
}{
	{
		input:    "line one\nline two\r\nline three\n\nline four",
		expected: []string{"line one\n", "line two\n", "line three\n", "line four\n"},
		splitter: new(LineSplitter),
	}, {
		input:    "2017-05-01 line one\nline two\n2017-05-01 line three\n2017-05-01 line four\n",
		expected: []string{"2017-05-01 line one\nline two\n", "2017-05-01 line three\n", "2017-05-01 line four\n"},
		splitter: NewRegexSplitter(regexp.MustCompile(`\d{4}-\d{2}-\d{2} .+`)),
	}, {
		input:    "line one\nline two\n\nline three\n",
		expected: []string{"line one\nline two\n\nline three\n"},
		splitter: new(NoSplit),
	},
}

func TestSplit(t *testing.T) {
	assert := assert.New(t)

	for _, test := range splitTests {
		ls := &LogSplitter{Splitter: test.splitter}
		out := new(capture)
		err := ls.Copy(strings.NewReader(test.input), out)
		assert.Nil(err, "copy should not fail")
		assert.Equal(test.expected, out.bufs, "data should match expected")
	}
}

func TestMaxLines(t *testing.T) {
	assert := assert.New(t)
	require := require.New(t)

	input := "line one\nline two\nline three\n\nline four\nline five"
	expected := []string{"line one\nline two\n", "line three\n\n", "line four\nline five\n"}
	ls := &LogSplitter{
		MaxEventLines: 2,
		Splitter:      new(NoSplit),
	}
	out := new(capture)
	err := ls.Copy(strings.NewReader(input), out)
	require.Nil(err, "copy should not fail")
	assert.Equal(expected, out.bufs)
}

func TestAddTimestamp(t *testing.T) {
	assert := assert.New(t)
	require := require.New(t)

	input := "line one\nline two"

	ls := &LogSplitter{
		Timestamp: time.RFC3339,
		Splitter:  new(LineSplitter),
	}

	out := new(capture)
	err := ls.Copy(strings.NewReader(input), out)
	require.Nil(err, "copy should not fail")
	require.Equal(2, len(out.bufs), "should capture two lines")

	now := time.Now()

	for _, line := range out.bufs {
		fields := strings.Fields(line)
		ts, err := time.Parse(time.RFC3339, fields[0])
		require.Nil(err, "should not fail to parse time component for ts=%q line %q", fields[0], line)
		assert.WithinDuration(now, ts, time.Second, "timestamp should be close to now")
	}
}
