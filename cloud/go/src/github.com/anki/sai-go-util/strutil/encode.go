// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package strutil

import (
	"errors"
	"math"
	"math/big"
)

func EncodeInt(reader Reader, minLength int, n uint64) string {
	var (
		result [64]byte
		rem    uint64
	)
	rl := uint64(len(reader))
	if rl < 2 {
		panic("Invalid reader for EncodeInt")
	}

	i := len(result)
	for n > 0 {
		i--
		n, rem = n/rl, n%rl
		result[i] = reader[rem]
	}
	for len(result)-i < minLength {
		i--
		result[i] = reader[0]
	}
	return string(result[i:])
}

// ToBinary packs a value represented by an encoded string into an arbitrarily large
// big-endian unsigned binary number.
func ToBinary(reader Reader, value string) (packed []uint8) {
	var (
		table  [256]uint8
		result = new(big.Int)
		base   = big.NewInt(int64(len(reader)))
		num    big.Int
	)
	for i, ch := range reader {
		table[ch] = uint8(i)
	}

	for i := 0; i < len(value); i++ {
		if i > 0 {
			result.Mul(result, base)
		}
		result.Add(result, num.SetInt64(int64(table[value[i]])))
	}

	return result.Bytes()
}

// FromBinary takes a packed big endian unsigned binary number and encodes it using
// the supplied Reader.
func FromBinary(reader Reader, packed []uint8, minLength int) string {
	var (
		zero  big.Int
		mod   big.Int
		base  = big.NewInt(int64(len(reader)))
		value = new(big.Int).SetBytes(packed)
		rlen  = float64(len(reader))
		plen  = float64(len(packed))
	)

	// allocate enough space to hold all result digits

	l := int(math.Ceil(plen * (8 / math.Log2(rlen))))
	if l < minLength {
		l = minLength
	}
	result := make([]byte, l)

	i := len(result)
	for value.Cmp(&zero) != 0 {
		i--
		value.DivMod(value, base, &mod)
		result[i] = reader[mod.Int64()]
	}
	for bc := l - i; minLength-bc > 0; bc++ {
		i--
		result[i] = reader[0]
	}
	return string(result[i:])
}

const hex = "0123456789abcdef"

// ToUUIDFormat takes a value held in an encoded string and returns its 128
// bit representation formatted as a UUID (though does not match the UUID spec)
//
// Returns an error if the number is larger than 128 bits
func ToUUIDFormat(reader Reader, value string) (string, error) {
	packed := ToBinary(reader, value)
	if len(packed) > 16 {
		return "", errors.New("Value too large")
	}

	result := make([]byte, 32)
	var v byte
	p := 31
	for i := len(packed); i > 0; i-- {
		v = packed[i-1]
		result[p] = hex[v&0xf]
		p--
		result[p] = hex[(v>>4)&0xf]
		p--
	}
	for ; p >= 0; p-- {
		result[p] = '0'
	}

	return string(result[0:8]) + "-" + string(result[8:12]) + "-" + string(result[12:16]) + "-" + string(result[16:20]) + "-" + string(result[20:]), nil
}

// FromUUIDFormat converts a UUID string into a number represented by a Reader
func FromUUIDFormat(reader Reader, uuid string, minLength int) (string, error) {
	if len(uuid) != 36 {
		return "", errors.New("Invalid length UUID")
	}

	packed := make([]byte, 16)
	var ch, nyb byte
	hi := true
	i := 0
	for j := 0; j < 36; j++ {
		ch = uuid[j]
		if ch == '-' {
			continue
		}
		switch {
		case '0' <= ch && ch <= '9':
			nyb = ch - '0'
		case 'a' <= ch && ch <= 'f':
			nyb = ch - 'a' + 10
		case 'A' <= ch && ch <= 'F':
			nyb = ch - 'A' + 10
		default:
			return "", errors.New("Illegal UUID")
		}
		if hi {
			packed[i] = nyb << 4
		} else {
			packed[i] |= nyb
			i++
		}
		hi = !hi
	}

	return FromBinary(reader, packed, minLength), nil
}
