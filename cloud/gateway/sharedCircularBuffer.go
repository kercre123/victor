package main

import (
	"anki/log"
	"syscall"
	"unsafe"
)

const (
	// If you change the buffer size (300) to something else, you need to make the same change in micDataProcessor.cpp
	bufferSize      = 300
	samplesPerCycle = 160
	headerMagicNum  = 0x08675309001a2b3c
)

type MicSDKData struct {
	WinningDirection  int16
	WinningConfidence int16
	Samples           [samplesPerCycle]int16
}

type memoryMap struct {
	HeaderMagicNum uint64
	QueuedCount    uint64
	ReaderCount    uint64
	Objects        [bufferSize]MicSDKData
}

// SharedCircularBuffer contains local state about the memory mapping, as well as the
// pointer to the map itself.
type SharedCircularBuffer struct {
	Name    string
	Initted bool // Will be initted false due to the zero value of a boolean.
	Buffer  *memoryMap
	MapFile int
	Offset  uint64
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
	client := SharedCircularBuffer{Name: name}
	client.init()
	return &client
}

// Close is the SharedCircularBuffer destructor. Be VERY sure to call it when you're
// done, even if you're failing and returning an error. It's important that SCB decrement
// the reader count to deactivate the queueing of data.
func (client *SharedCircularBuffer) Close() {
	if client.MapFile != -1 {
		syscall.Close(client.MapFile)
	}
	if client.Initted {
		client.Buffer.ReaderCount--
	}
}

// GetNext returns the next valid object in the buffer. If you're up-to-date, or the
// writer hasn't started yet, the second part of the return value will be "please wait".
// If it's been too long since you last asked for data, you've missed some data, and
// you'll be fast-forwarded to the most recent entry. In this case, the second part of
// the return tuple will be "behind".
func (client *SharedCircularBuffer) GetNext() (*MicSDKData, uint64, string) {
	if (!client.Initted && !client.init()) ||
		client.Offset == client.Buffer.QueuedCount {

		return nil, client.Offset, "please wait"
	}

	getStatus := "okay"
	if client.Buffer.QueuedCount > (3*bufferSize/4) && client.Offset < client.Buffer.QueuedCount-(3*bufferSize/4) {
		client.Offset = client.Buffer.QueuedCount
		getStatus = "behind"
	}

	objectPtr := &client.Buffer.Objects[client.Offset%bufferSize]
	client.Offset++
	return objectPtr, client.Offset - 1, getStatus
}

func (client *SharedCircularBuffer) init() bool {
	if client.Initted {
		return true
	}

	var err error
	fullBufferPathName := "/run/sharedCircularBuffer/" + client.Name
	client.MapFile, err = syscall.Open(fullBufferPathName, syscall.O_RDWR, 0)

	if err != nil {
		client.MapFile = -1
		log.Println("Cannot open shared circular buffer file, ", fullBufferPathName)
		return false
	}

	var tmpMemoryMap memoryMap

	mmap, err := syscall.Mmap(
		client.MapFile,
		0,
		int(unsafe.Sizeof(tmpMemoryMap)),
		syscall.PROT_READ|syscall.PROT_WRITE,
		syscall.MAP_SHARED)

	if err != nil {
		log.Println("Cannot mmap shared circular buffer file, ", fullBufferPathName)
		return false
	}

	client.Buffer = (*memoryMap)(unsafe.Pointer(&mmap[0]))

	if client.Buffer.HeaderMagicNum != headerMagicNum {
		log.Println("Waiting for writer to complete initialization of circular buffer.")
		return false
	}

	client.Buffer.ReaderCount++
	client.Offset = client.Buffer.QueuedCount
	client.Initted = true

	return true
}
