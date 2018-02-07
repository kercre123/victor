package util

import (
	"sync"
	"time"
)

// DoOnce wraps a sync.Once object, taking a function at initialization time
// rather than at execution time; that way, if the function to be executed is
// known in advance for all cases, it can be stored in the object rather than
// repeated in arguments to Once.Do()
type DoOnce struct {
	once sync.Once
	todo func()
}

// NewDoOnce returns a new DoOnce object that will store the given function and
// execute it in calls to DoOnce.Do()
func NewDoOnce(todo func()) DoOnce {
	return DoOnce{sync.Once{}, todo}
}

// Do executes the function associated with this DoOnce instance
func (d *DoOnce) Do() {
	d.once.Do(d.todo)
}

// CanSelect is a helper for struct{} channels that checks if the channel can be
// pulled from in a select statement
func CanSelect(ch <-chan struct{}) bool {
	select {
	case <-ch:
		return true
	default:
		return false
	}
}

// TimeFuncMs returns the time, in milliseconds, required to run the given function
func TimeFuncMs(function func()) float64 {
	callStart := time.Now()
	function()
	return float64(time.Now().Sub(callStart).Nanoseconds()) / float64(time.Millisecond/time.Nanosecond)
}
