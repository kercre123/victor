package ogg

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg CONTAINER SOURCE CODE.              *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2010             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: packing variable sized words into an octet stream
  last mod: $Id: bitwise.c 18051 2011-08-04 17:56:39Z giles $

 ********************************************************************/


// function: Packing variable sized words into an octet stream

// We're 'LSb' endian; if we write a word but read individual bits,
// then we'll read the lsb first

import (
	"encoding/binary"
)

const bw_BUFFER_INCREMENT = 256
const bw_INT32_MAX int32 = 0x7FFFFFFF

// mask is a constant. Do not change this!
var mask = []uint32{
	0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff, 0x000001ff,
	0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff,
	0x00007fff, 0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff,
	0x3fffffff, 0x7fffffff, 0xffffffff}

// mask8B is a constant. Do not change this!
var mask8B = []uint8{
	0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff}

type PackBuffer struct {
	endbyte int32
	endbit  int8 // int8 with overflow

	buffer []byte
	ptr    []byte
}

func (b *PackBuffer) WriteInit() {
	b.buffer = make([]byte, bw_BUFFER_INCREMENT)
	b.ptr = b.buffer
}

func (b *PackBuffer) WriteInitB() {
	b.WriteInit()
}

func (b *PackBuffer) WriteCheck() bool {
	if b.ptr == nil || b.buffer == nil {
		return false
	}
	return true
}

func (b *PackBuffer) WriteCheckB() bool {
	return b.WriteCheck()
}

func (b *PackBuffer) WriteTrunc(bits int32) {
	_bytes := bits >> 3
	if b.ptr != nil {
		bits -= _bytes * 8
		b.ptr = b.buffer[_bytes:]
		b.endbit = int8(bits)
		b.endbyte = _bytes
		// b.ptr &= mask[bits]

		mask_arr := [4]byte{}
		mask_bits := mask_arr[:]
		binary.LittleEndian.PutUint32(mask_bits, mask[bits])

		b.ptr[0] &= mask_bits[0]
		b.ptr[1] &= mask_bits[1]
		b.ptr[2] &= mask_bits[2]
		b.ptr[3] &= mask_bits[3]
	}
}

func (b *PackBuffer) WriteTruncB(bits int32) {
	_bytes := bits >> 3
	if b.ptr != nil {
		bits -= _bytes * 8
		b.ptr = b.buffer[_bytes:]
		b.endbit = int8(bits)
		b.endbyte = _bytes
		b.ptr[0] &= mask8B[bits]
	}
}

// Takes only up to 32 bits. 
func (b *PackBuffer) Write(value uint32, bits int8) {
	if bits < 0 || bits > 32 {
		goto err
	}
	if b.endbyte >= int32(len(b.buffer)-4) {
		if b.ptr == nil {
			return
		}
		if int32(len(b.buffer)) > bw_INT32_MAX-bw_BUFFER_INCREMENT {
			goto err
		}
		b.buffer = append(b.buffer, make([]byte, bw_BUFFER_INCREMENT)...)
		b.ptr = b.buffer[b.endbyte:]
	}

	value &= mask[bits]
	bits += b.endbit

	b.ptr[0] |= byte(value << uint(b.endbit))

	if bits >= 8 {
		b.ptr[1] = byte((value >> (8 - uint(b.endbit))))
		if bits >= 16 {
			b.ptr[2] = byte((value >> (16 - uint(b.endbit))))
			if bits >= 24 {
				b.ptr[3] = byte((value >> (24 - uint(b.endbit))))
				if bits >= 32 {
					if b.endbit != 0 {
						b.ptr[4] = byte((value >> (32 - uint(b.endbit))))
					} else {
						b.ptr[4] = 0
					}
				}
			}
		}
	}

	b.endbyte += int32(bits / 8)
	b.ptr = b.ptr[bits/8:]
	b.endbit = bits & 7
	return
err:
	b.WriteClear()
}

// Takes only up to 32 bits. 
func (b *PackBuffer) WriteB(value uint32, bits int8) {
	if bits < 0 || bits > 32 {
		goto err
	}
	if b.endbyte >= int32(len(b.buffer)-4) {
		if b.ptr == nil {
			return
		}
		if int32(len(b.buffer)) > bw_INT32_MAX-bw_BUFFER_INCREMENT {
			goto err
		}
		b.buffer = append(b.buffer, make([]byte, bw_BUFFER_INCREMENT)...)
		b.ptr = b.buffer[b.endbyte:]
	}

	value = (value & mask[bits]) << (32 - uint(bits))
	bits += b.endbit

	b.ptr[0] |= byte(value >> (24 + uint(b.endbit)))

	if bits >= 8 {
		b.ptr[1] = byte((value >> (16 + uint(b.endbit))))
		if bits >= 16 {
			b.ptr[2] = byte(value >> (8 + uint(b.endbit)))
			if bits >= 24 {
				b.ptr[3] = byte(value >> uint(b.endbit))
				if bits >= 32 {
					if b.endbit != 0 {
						b.ptr[4] = byte(value << (8 - uint(b.endbit)))
					} else {
						b.ptr[4] = 0
					}
				}
			}
		}
	}

	b.endbyte += int32(bits / 8)
	b.ptr = b.ptr[bits/8:]
	b.endbit = bits & 7
	return
err:
	b.WriteClear()
}

func (b *PackBuffer) WriteAlign() {
	bits := 8 - b.endbit
	if bits < 8 {
		b.Write(0, bits)
	}
}

func (b *PackBuffer) WriteAlignB() {
	bits := 8 - b.endbit
	if bits < 8 {
		b.WriteB(0, bits)
	}
}

func (b *PackBuffer) handlerFunc(value uint32, bits int8, msb bool) {
	if msb == false {
		b.Write(value, bits)
	} else {
		b.WriteB(value, bits)
	}
}

func (b *PackBuffer) writeCopyHelper(source []byte, bits int8, msb bool) {
	ptr := source
	_bytes := int8(bits / 8)
	bits -= _bytes * 8

	if b.endbit != 0 {
		var i int8
		// unaligned copy.  Do it the hard way. 
		for i = 0; i < _bytes; i++ {
			b.handlerFunc(uint32(ptr[i]), 8, msb)
		}
	} else {
		// aligned block copy 
		if b.endbyte+int32(_bytes+1) >= int32(len(b.buffer)) {
			if b.ptr == nil {
				goto err
			}
			if b.endbyte+int32(_bytes)+bw_BUFFER_INCREMENT > int32(len(b.buffer)) {
				goto err
			}
			storage := b.endbyte + int32(_bytes) + bw_BUFFER_INCREMENT
			b.buffer = append(b.buffer, make([]byte, storage)...)
			b.ptr = b.buffer[b.endbyte:]
		}

		copy(b.ptr, source[0:_bytes])
		b.ptr = b.ptr[_bytes:]
		b.endbyte += int32(_bytes)
		b.ptr[0] = 0
	}
	if bits != 0 {
		if msb == true {
			b.handlerFunc(uint32(ptr[_bytes]>>(8-uint(bits))), bits, msb)
		} else {
			b.handlerFunc(uint32(ptr[_bytes]), bits, msb)
		}
	}
	return
err:
	b.WriteClear()
}

func (b *PackBuffer) WriteCopy(source []byte, bits int8) {
	b.writeCopyHelper(source, bits, false)
}

func (b *PackBuffer) WriteCopyB(source []byte, bits int8) {
	b.writeCopyHelper(source, bits, true)
}

func (b *PackBuffer) Reset() {
	if b.ptr == nil {
		return
	}
	b.ptr = b.buffer
	b.buffer[0] = 0
	b.endbyte = 0
	b.endbit = 0
}

func (b *PackBuffer) ResetB() {
	b.Reset()
}

func (b *PackBuffer) WriteClear() {
	// This frees the old buffer and replaces it with a new one.
	b = new(PackBuffer)
}

func (b *PackBuffer) WriteClearB() {
	b.WriteClear()
}

func (b *PackBuffer) ReadInit(buf []byte) {
	b.endbyte = 0
	b.endbit = 0

	b.ptr = buf
	b.buffer = buf
}

func (b *PackBuffer) ReadInitB(buf []byte) {
	b.ReadInit(buf)
}

// Read in bits without advancing the bitptr; bits <= 32 
func (b *PackBuffer) Look(bits int8) int32 {
	var ret, m uint32

	if bits < 0 || bits > 32 {
		return -1
	}
	m = mask[bits]
	bits += b.endbit

	if b.endbyte >= int32(len(b.buffer)-4) {
		// not the main path 
		if b.endbyte > int32(len(b.buffer))-int32((bits+7)>>3) {
			// special case to afunc reading b.ptr[0], which might be past
			// the end of the buffer; also skips some useless accounting 
			return -1
		} else if bits == 0 {
			return 0
		}
	}

	ret = uint32(b.ptr[0] >> uint(b.endbit))
	if bits > 8 {
		ret |= uint32(b.ptr[1]) << (8 - uint(b.endbit))
		if bits > 16 {
			ret |= uint32(b.ptr[2]) << (16 - uint(b.endbit))
			if bits > 24 {
				ret |= uint32(b.ptr[3]) << (24 - uint(b.endbit))
				if bits > 32 && b.endbit != 0 {
					ret |= uint32(b.ptr[4]) << (32 - uint(b.endbit))
				}
			}
		}
	}
	return int32(m & ret)
}

// Read in bits without advancing the bitptr; bits <= 32
func (b *PackBuffer) LookB(bits int8) int32 {
	var ret uint32
	m := 32 - bits

	if m < 0 || m > 32 {
		return -1
	}
	bits += b.endbit

	if b.endbyte >= int32(len(b.buffer)-4) {
		// not the main path 
		if b.endbyte > int32(len(b.buffer))-int32((bits+7)>>3) {
			return -1
			// special case to afunc reading b.ptr[0], which might be past
			// the end of the buffer; also skips some useless accounting 
		} else if bits == 0 {
			return 0
		}
	}

	ret = uint32(b.ptr[0]) << (24 + uint(b.endbit))
	if bits > 8 {
		ret |= uint32(b.ptr[1]) << (16 + uint(b.endbit))
		if bits > 16 {
			ret |= uint32(b.ptr[2]) << (8 + uint(b.endbit))
			if bits > 24 {
				ret |= uint32(b.ptr[3]) << uint(b.endbit)
				if bits > 32 && b.endbit != 0 {
					ret |= uint32(b.ptr[4]) >> (8 - uint(b.endbit))
				}
			}
		}
	}
	return int32(ret >> (uint32(m) >> 1) >> (uint32(m+1) >> 1))
}

func (b *PackBuffer) Look1() int32 {
	if b.endbyte >= int32(len(b.buffer)) {
		return -1
	}
	return int32((b.ptr[0] >> uint(b.endbit)) & 1)
}

func (b *PackBuffer) Look1B() int32 {
	if b.endbyte >= int32(len(b.buffer)) {
		return -1
	}
	return int32((b.ptr[0] >> (7 - uint(b.endbit))) & 1)
}

func (b *PackBuffer) Adv(bits int8) {
	bits += b.endbit

	if b.endbyte > int32(len(b.buffer))-int32((bits+7)>>3) {
		goto overflow
	}

	b.ptr = b.ptr[bits/8:]
	b.endbyte += int32(bits / 8)
	b.endbit = bits & 7
	return

overflow:
	b.ptr = nil
	b.endbyte = int32(len(b.buffer))
	b.endbit = 1
}

func (b *PackBuffer) AdvB(bits int8) {
	b.Adv(bits)
}

func (b *PackBuffer) Adv1() {
	b.endbit++
	if b.endbit > 7 {
		b.endbit = 0
		b.ptr = b.ptr[1:]
		b.endbyte++
	}
}

func (b *PackBuffer) Adv1B() {
	b.Adv1()
}

// bits <= 32 
func (b *PackBuffer) Read(bits int8) int32 {
	var ret, m uint32
	
	if bits < 0 || bits > 32 {
		goto err
	}
	m = mask[bits]
	bits += b.endbit

	if b.endbyte >= int32(len(b.buffer)-4) {
		// not the main path 
		if b.endbyte > int32(len(b.buffer)) - int32((bits+7)>>3) {
			goto overflow
			// special case to afunc reading b.ptr[0], which might be past 
			// the end of the buffer; also skips some useless accounting 
		} else if bits == 0 {
			return 0
		}
	}

	ret = uint32(b.ptr[0]) >> uint(b.endbit)
	if bits > 8 {
		ret |= uint32(b.ptr[1]) << (8 - uint(b.endbit))
		if bits > 16 {
			ret |= uint32(b.ptr[2]) << (16 - uint(b.endbit))
			if bits > 24 {
				ret |= uint32(b.ptr[3]) << (24 - uint(b.endbit))
				if bits > 32 && b.endbit != 0 {
					ret |= uint32(b.ptr[4]) << (32 - uint(b.endbit))
				}
			}
		}
	}

	ret &= m
	b.ptr = b.ptr[bits/8:]
	b.endbyte += int32(bits / 8)
	b.endbit = bits & 7
	return int32(ret)

overflow:
err:
	b.ptr = nil
	b.endbyte = int32(len(b.buffer))
	b.endbit = 1
	return -1
}

// bits <= 32 
func (b *PackBuffer) ReadB(bits int8) int32 {
	var ret uint32
	m := int32(32 - bits)

	if m < 0 || m > 32 {
		goto err
	}
	bits += b.endbit

	if b.endbyte+4 >= int32(len(b.buffer)) {
		// not the main path 
		if b.endbyte > int32(len(b.buffer))-int32((bits+7)>>3) {
			goto overflow
			// special case to afunc reading b.ptr[0], which might be past 
			// the end of the buffer; also skips some useless accounting 
		} else if bits == 0 {
			return 0
		}
	}

	ret = uint32(b.ptr[0]) << (24 + uint(b.endbit))
	if bits > 8 {
		ret |= uint32(b.ptr[1]) << (16 + uint(b.endbit))
		if bits > 16 {
			ret |= uint32(b.ptr[2]) << (8 + uint(b.endbit))
			if bits > 24 {
				ret |= uint32(b.ptr[3]) << uint((b.endbit))
				if bits > 32 && b.endbit != 0 {
					ret |= uint32(b.ptr[4]) >> (8 - uint(b.endbit))
				}
			}
		}
	}
	ret = ((ret&0xffffffff) >> uint(m >> 1)) >> uint((m + 1) >> 1)
	b.ptr = b.ptr[bits/8:]
	b.endbyte += int32(bits / 8)
	b.endbit = bits & 7
	return int32(ret)

overflow:
err:
	b.ptr = nil
	b.endbyte = int32(len(b.buffer))
	b.endbit = 1
	return -1
}

func (b *PackBuffer) Read1() int32 {
	var ret int32

	if b.endbyte >= int32(len(b.buffer)) {
		goto overflow
	}
	ret = int32((b.ptr[0] >> uint(b.endbit)) & 1)

	b.endbit++
	if b.endbit > 7 {
		b.endbit = 0
		b.ptr = b.ptr[1:]
		b.endbyte++
	}
	return ret

overflow:
	b.ptr = nil
	b.endbyte = int32(len(b.buffer))
	b.endbit = 1
	return -1
}

func (b *PackBuffer) Read1B() int32 {
	var ret int32

	if b.endbyte >= int32(len(b.buffer)) {
		goto overflow
	}
	ret = int32((b.ptr[0] >> (7 - uint(b.endbit))) & 1)

	b.endbit++
	if b.endbit > 7 {
		b.endbit = 0
		b.ptr = b.ptr[1:]
		b.endbyte++
	}
	return ret

overflow:
	b.ptr = nil
	b.endbyte = int32(len(b.buffer))
	b.endbit = 1
	return -1
}

func (b *PackBuffer) Bytes() int32 {
	return b.endbyte + int32((b.endbit+7)/8)
}

func (b *PackBuffer) Bits() int32 {
	return b.endbyte*8 + int32(b.endbit)
}

func (b *PackBuffer) BytesB() int32 {
	return b.Bytes()
}

func (b *PackBuffer) BitsB() int32 {
	return b.Bits()
}

func (b *PackBuffer) GetBuffer() []byte {
	return b.buffer
}

func (b *PackBuffer) GetBufferB() []byte {
	return b.GetBuffer()
}
