package hstore

import (
	"github.com/stretchr/testify/assert"
	"testing"
	"time"
)

func TestSemaphore(t *testing.T) {
	var s semaphore = make(chan struct{}, 10)
	s.acquire()
	assert.Equal(t, 1, s.used())
	assert.Equal(t, 9, s.available())

	s.release()
	assert.Equal(t, 0, s.used())
	assert.Equal(t, 10, s.available())
}

func TestSemaphoreWait(t *testing.T) {
	var s semaphore = make(chan struct{}, 10)
	s.acquire()
	s.acquire()
	go func() {
		time.Sleep(time.Second)
		s.release()
		s.release()
	}()

	s.wait()
	assert.Equal(t, 10, s.used())
	t.Log("Done")
	s.release()
	s.acquire()
	assert.Equal(t, 10, s.used())
}
