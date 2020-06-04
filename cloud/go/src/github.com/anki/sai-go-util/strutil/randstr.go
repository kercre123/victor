// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

/*
Package strutil provides a method for generating random strings based  128 bit unique
identifiers.

It can generate ids using either case sensitive or case insensitive characters.

The generated values are random, but do not comply with the UUID4 spec
*/
package strutil

import (
	"crypto/rand"
	"io"
	"math"
)

const (
	Digits    = Reader("0123456789")
	Lower     = Reader("abcdefghijklmnopqrstuvwxyz")
	Upper     = Reader("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
	LowerSafe = Reader("23456789abcdefghjkmnpqrstuvwxyz")
	AllSafe   = Reader("23456789abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ")
	All       = Digits + Lower + Upper
)

// Bits per byte
var (
	DigitsBPB    = math.Log2(float64(len(Digits)))
	LowerBPB     = math.Log2(float64(len(Lower)))
	UpperBPB     = math.Log2(float64(len(Upper)))
	LowerSafeBPB = math.Log2(float64(len(LowerSafe)))
	AllSafeBPB   = math.Log2(float64(len(AllSafe)))
	AllBPB       = math.Log2(float64(len(All)))
)

var (
	RandSource io.Reader = rand.Reader
)

type Reader string

func NewReader(charset string) io.Reader {
	return Reader(charset)
}

func (r Reader) Read(buf []byte) (n int, err error) {
	buflen := cap(buf)
	rbytes := make([]byte, buflen)
	if _, err := io.ReadFull(RandSource, rbytes); err != nil {
		return 0, err
	}

	for i := 0; i < buflen; i++ {
		buf[i] = r[int(rbytes[i])%len(r)]
	}
	return buflen, nil
}

var (
	UUIDBitCount = 128 // Minimum number of bits we want encoded into a UUID

	shortLowerByteCount = int(math.Ceil(float64(UUIDBitCount) / LowerSafeBPB))
	shortMixedByteCount = int(math.Ceil(float64(UUIDBitCount) / AllSafeBPB))
)

func ShortLowerUUID() (string, error) {
	buf := make([]byte, shortLowerByteCount)
	if _, err := io.ReadFull(LowerSafe, buf); err != nil {
		return "", err
	}
	return string(buf), nil
}

func ShortMixedUUID() (string, error) {
	buf := make([]byte, shortMixedByteCount)
	if _, err := io.ReadFull(LowerSafe, buf); err != nil {
		return "", err
	}
	return string(buf), nil
}

// LowerStr returns a random string consisting of lowercase
// letters and digits.
func LowerStr(length int) string {
	buf := make([]byte, length)
	io.ReadFull(LowerSafe, buf)
	return string(buf)
}

// Str returns a random string with characters from the
// passed in reader.
func Str(reader Reader, length int) (string, error) {
	buf := make([]byte, length)
	if _, err := io.ReadFull(reader, buf); err != nil {
		return "", err
	}
	return string(buf), nil
}
