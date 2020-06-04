// Copyright 2016 Anki, Inc.

package logtest

import (
	"bytes"
	"log"
	"strings"

	"github.com/anki/sai-go-util/log"
)

// RunLog executes a function and records the log data generated while it
// was running to the supplied Buffer.
func Runlog(flags alog.AlogFlag, buf *bytes.Buffer, f func()) {
	defer func(oldLogger *log.Logger, oldFlags alog.AlogFlag) {
		alog.Logger = oldLogger
		alog.AlogFlags = oldFlags
	}(alog.Logger, alog.AlogFlags)

	alog.Logger = log.New(buf, "", 0)
	alog.AlogFlags = flags

	f()
}

// Capture executes a function and returns the log data generated while
// it was running.
func Capture(flags alog.AlogFlag, f func()) (result string) {
	buf := new(bytes.Buffer)
	Runlog(flags, buf, f)
	return buf.String()
}

// StripFuncName removes the functname name portion ofa log entry.
func StripFuncName(entry string) string {
	parts := strings.SplitN(entry, " ", 2)
	return parts[1]
}

// ExtractKeys pulls out k=v pairs from a string
func ExtractKeys(s string) (result map[string]string) {
	var (
		i, j          int
		ch            rune
		k             string
		inK, inV, inQ bool
	)
	result = make(map[string]string)
	s = strings.TrimSpace(s)

	for i, ch = range s {
		if inK {
			switch ch {
			case '=':
				k = s[j:i]
				inK, inV = false, true
				j = i + 1
			case ' ':
				j = i + 1 // skip preamble
			}
		} else if inV {
			if inQ {
				if ch == '"' {
					result[k] = s[j:i]
					inK, inV, inQ = false, false, false
					j = i + 1
				}
			} else {
				switch ch {
				case '"':
					inQ = true
					j = i + 1
				case ' ':
					result[k] = s[j:i]
					inK, inV = false, false
					j = i + 1
				}
			}
		} else {
			if ch != ' ' {
				inK = true
				j = i
			}
		}
	}
	if inV {
		result[k] = s[j:]
	}
	return result
}

// HasKeys checks to see if the supplied key names are defined in a map
// and returns a list of the missing key names.
func HasKeys(m map[string]string, keys ...string) (missing []string) {
	for _, k := range keys {
		if _, ok := m[k]; !ok {
			missing = append(missing, k)
		}
	}
	return missing
}
