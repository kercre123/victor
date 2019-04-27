// Copyright 2016 Anki, Inc.

package kinreader

import (
	"encoding/json"
	"os"
	"sync"
)

// FileCheckpointer stores checkpoint information in a local file.
// It is not safe to share the file between concurrent readers.
type FileCheckpointer struct {
	m        sync.Mutex
	Filename string
	f        *os.File
}

// NewFileCheckpointer opens or creates a file to use as a FileCheckpointer.
func NewFileCheckpointer(filename string) (*FileCheckpointer, error) {
	f, err := os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		return nil, err
	}
	return &FileCheckpointer{Filename: filename, f: f}, nil
}

// SetCheckpoint synchronously updates the checkpoint file with
// the provided checkpoint data.
func (f *FileCheckpointer) SetCheckpoint(cp Checkpoint) error {
	f.m.Lock()
	defer f.m.Unlock()

	current, err := f.allCheckpoints()
	if err != nil {
		return err
	}
	f.f.Truncate(0)
	f.f.Seek(0, 0)
	defer f.f.Sync()
	current[cp.ShardId] = &cp
	return json.NewEncoder(f.f).Encode(current)
}

// AllCheckpoints fetches the current checkpoints from the file.
func (f *FileCheckpointer) AllCheckpoints() (cp Checkpoints, err error) {
	f.m.Lock()
	defer f.m.Unlock()

	return f.allCheckpoints()
}

func (f *FileCheckpointer) allCheckpoints() (cp Checkpoints, err error) {
	f.f.Seek(0, 0)
	s, err := f.f.Stat()
	if err != nil {
		return nil, err
	}
	if s.Size() == 0 {
		cp := make(Checkpoints)
		return cp, nil
	}

	if err := json.NewDecoder(f.f).Decode(&cp); err != nil {
		return nil, err
	}
	return cp, nil
}
