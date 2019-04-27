// Copyright 2016 Anki, Inc.

package kinreader

import "time"

// replaced by unit tests
var newTimer = func(d time.Duration) *time.Timer {
	return time.NewTimer(d)
}
