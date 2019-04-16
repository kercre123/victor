package main

import (
	"syscall"
	"unsafe"
)

const (
	// If you change the buffer size (300) to something else, you need to make the same change in micDataProcessor.cpp
	bufferSize      = 300
	samplesPerCycle = 160
	headerMagicNum  = 0x08675309deadbeef
)

type MicSDKData struct {
	robotAngle float32
	samples    [samplesPerCycle]int16
}

type memoryMap struct {
	headerMagicNum uint64
	queuedCount    uint64
	readerCount    uint64
	objects        [bufferSize]MicSDKData
}

// SharedCircularBuffer contains local state about the memory mapping, as well as the
// pointer to the map itself.
type SharedCircularBuffer struct {
	name    string
	initted bool // Will be initted false due to the zero value of a boolean.
	buffer  *memoryMap
	mapFile int
	offset  uint64
	//owner bool // unused. We don't host any buffers in go.
}

// NewSharedCircularBuffer allocates an SCB client and returns a pointer to it. It will
// ATTEMPT to initialize the client, but will still return an uninitialized client, should
// it fail. The return values of client functions will indicate whether the shared memory
// object persists in its absence (the only reason this should fail.)
//
// It is IMPERATIVE that you destroy the returned object, since the destructor decrements
// the client count on the shared object.
func NewSharedCircularBuffer(name string) *SharedCircularBuffer {
	client := SharedCircularBuffer{name: name}
	client.init()
	client.offset = 0xffffffffffffffff
	return &client
}

// Close is the SharedCircularBuffer destructor. Be VERY sure to call it when you're
// done, even if you're failing with an exception. It's important that SCB decrement
// the reader count to deactivate the queueing of data.
func (client *SharedCircularBuffer) Close() {
	if client.mapFile != -1 {
		syscall.Close(client.mapFile)
	}
	if client.initted {
		client.buffer.readerCount--
	}
}

// GetNext returns the next valid object in the buffer. If you're up-to-date, or the
// writer hasn't started yet, the second part of the return value will be "please wait".
// If it's been too long since you last asked for data, you've missed some data, and
// you'll be fast-forwarded to the most recent entry. In this case, the second part of
// the return tuple will be "behind".
func (client *SharedCircularBuffer) GetNext() (*MicSDKData, uint64, string) {
	client.offset++
	if (!client.initted && !client.init()) ||
		client.buffer.queuedCount == 0 ||
		client.buffer.queuedCount <= client.offset {

		client.offset--
		return nil, 0, "please wait"
	}

	getStatus := "okay"
	if client.offset < client.buffer.queuedCount-(3*bufferSize/4) {
		offset = client.buffer.queuedCount - 1
		getStatus = "behind"
	}

	return &client.buffer.objects[client.offset%bufferSize], client.offset, getStatus
}

func (client *SharedCircularBuffer) init() bool {
	if client.initted {
		return true
	}

	var err error
	fullBufferPathName := "/run/sharedCircularBuffer/" + client.name
	client.mapFile, err = syscall.Open(fullBufferPathName, syscall.O_RDWR, 0)

	if err != nil {
		client.mapFile = -1
		return false
	}

	var tmpMemoryMap memoryMap

	mmap, err := syscall.Mmap(
		client.mapFile,
		0,
		int(unsafe.Sizeof(tmpMemoryMap)),
		syscall.PROT_READ|syscall.PROT_WRITE,
		syscall.MAP_SHARED)

	if err != nil {
		return false
	}

	client.buffer = (*memoryMap)(unsafe.Pointer(&mmap[0]))

	if client.buffer.headerMagicNum != headerMagicNum {
		return false
	}

	client.buffer.readerCount++
	client.initted = true

	return true
}
