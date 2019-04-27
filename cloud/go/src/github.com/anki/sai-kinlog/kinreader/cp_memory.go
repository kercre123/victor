// Copyright 2016 Anki, Inc.

package kinreader

import "sync"

// MemCheckpointer stores checkpoints only in memory.
// Generally useful for testing only.
type MemCheckpointer struct {
	m     sync.Mutex
	store Checkpoints
}

// NewMemCheckpointer create a MemCheckpointer.
func NewMemCheckpointer() (*MemCheckpointer, error) {
	return &MemCheckpointer{store: make(Checkpoints)}, nil
}

// SetCheckpoint updates the checkpoints.
func (m *MemCheckpointer) SetCheckpoint(cp Checkpoint) error {
	m.m.Lock()
	defer m.m.Unlock()
	m.store[cp.ShardId] = &cp
	return nil
}

// AllCheckpoints returns all currently set checkpoints.
func (m *MemCheckpointer) AllCheckpoints() (cp Checkpoints, err error) {
	cp = make(Checkpoints)
	m.m.Lock()
	for k, v := range m.store {
		cp[k] = v
	}
	m.m.Unlock()
	return cp, nil
}
