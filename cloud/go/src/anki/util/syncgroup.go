package util

import "sync"

// SyncGroup extends sync.WaitGroup with simpler usage for the common case where you just
// want to add some functions and have them run in routines, then wait for them to all finish.
// It does this with the AddFunc function, which will call Add(1), then execute the passed-in
// function on a goroutine and call Done() on completion.
type SyncGroup struct {
	sync.WaitGroup
}

// AddFunc increases this SyncGroup's counter by 1, and immediately starts a goroutine to
// execute the given function object f. The goroutine will call Done() on this SyncGroup
// when f completes.
func (wg *SyncGroup) AddFunc(f func()) {
	if f == nil {
		return
	}
	wg.Add(1)
	go func() {
		f()
		wg.Done()
	}()
}
