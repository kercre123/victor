package das

import (
	"bufio"
	"compress/gzip"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"os"
	"regexp"
	"time"
)

type sqsMessage struct {
	MessageId string
	Body      []byte
	RecvTime  time.Time
}

func decodeSQS(r io.Reader) (msg *sqsMessage, err error) {
	err = json.NewDecoder(r).Decode(&msg)
	return msg, err
}

func dumpMsg(fn string, msg *sqsMessage) {
	fmt.Println("Filename  : ", fn)
	fmt.Println("Message ID: ", msg.MessageId)
	fmt.Println("Received  : ", msg.RecvTime)
	fmt.Println("Payload   : ", string(msg.Body))
}

func dasGrep(path string, pattern *regexp.Regexp) {
	f, err := os.Open(path)
	if err != nil {
		log.Printf("Failed to open %s: %v", path, err)
		return
	}
	defer f.Close()

	mgr, err := newMaybeGzipReader(f)
	if err != nil {
		log.Printf("Failed to read %s: %v", path, err)
		return
	}
	if mgr.isGzipped {
		dasGrepJSON(path, mgr, pattern)
	} else {
		dasGrepSQS(path, mgr, pattern)
	}
}

func dasGrepSQS(path string, r io.Reader, pattern *regexp.Regexp) {
	msg, err := decodeSQS(r)
	if err != nil {
		log.Printf("Failed to decode %s: %v", path, err)
		return
	}

	if pattern.Match(msg.Body) {
		dumpMsg(path, msg)
		fmt.Println()
	}
}

func dasGrepJSON(path string, r io.Reader, pattern *regexp.Regexp) {
	headerPrinted := false
	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		line := scanner.Bytes()
		if pattern.Match(line) {
			if !headerPrinted {
				fmt.Println("Filename  : ", path)
				headerPrinted = true
			}
			fmt.Println(string(line))
		}
	}
	if err := scanner.Err(); err != nil {
		log.Println("Failed to read %s: %v", path, err)
	}
}

func openFile(path string) (f *os.File, isGzipped bool, err error) {
	f, err = os.Open(path)
	if err != nil {
		return nil, false, err
	}
	buf := make([]byte, 2)
	if _, err := io.ReadFull(f, buf); err != nil {
		return nil, false, err
	}

	isGzipped = buf[0] == 0x1f && buf[1] == 0x8b

	f.Seek(0, io.SeekStart)
	return f, isGzipped, nil
}

// maybeGzipReader detects whether the io.Reader passed to it is gzipped,
// and if so transparently decompresses it and sets isGzipped to true.
type maybeGzipReader struct {
	io.Reader
	isGzipped bool
}

func newMaybeGzipReader(r io.Reader) (*maybeGzipReader, error) {
	br := bufio.NewReader(r)
	buf, err := br.Peek(2)
	if err != nil {
		return nil, err
	}

	isGzipped := buf[0] == 0x1f && buf[1] == 0x8b
	if isGzipped {
		gzr, err := gzip.NewReader(br)
		if err != nil {
			return nil, err
		}
		return &maybeGzipReader{
			Reader:    gzr,
			isGzipped: true,
		}, nil
	}
	return &maybeGzipReader{
		Reader:    br,
		isGzipped: false,
	}, nil
}
