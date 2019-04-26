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

import (
	"fmt"
	"os"
	"testing"
)

var outPb PackBuffer
var inPb PackBuffer

func ilog(v uint32) (ret int8) {
	for v != 0 {
		ret++
		v >>= 1
	}
	return
}

func report(str string) {
	fmt.Fprintf(os.Stderr, "%s", str)
	os.Exit(1)
}

func cliptest(b []uint32, vals int, bits int8, comp []byte, compsize int) {
	var bytes, i int32
	var buffer []byte

	outPb.Reset()
	for i = 0; i < int32(vals); i++ {
		if bits != 0 {
			outPb.Write(uint32(b[i]), bits)
		} else {
			outPb.Write(uint32(b[i]), ilog(b[i]))
		}
	}
	buffer = outPb.GetBuffer()
	bytes = outPb.Bytes()
	if bytes != int32(compsize) {
		report("wrong number of bytes!\n")
	}
	for i = 0; i < bytes; i++ {
		if buffer[i] != comp[i] {
			for i = 0; i < bytes; i++ {
				fmt.Fprintf(os.Stderr, "%x %x\n", int(buffer[i]), int(comp[i]))
			}
			report("wrote incorrect value!\n")
		}
	}
	inPb.ReadInit(buffer)
	for i = 0; i < int32(vals); i++ {
		var tbit int8
		if bits != 0 {
			tbit = bits
		} else {
			tbit = ilog(b[i])
		}

		if inPb.Look(tbit) == -1 {
			report("out of data!\n")
		}
		if inPb.Look(tbit) != int32(b[i]&mask[tbit]) {
			report("looked at incorrect value!\n")
		}
		if tbit == 1 {
			if inPb.Look1() != int32(b[i]&mask[tbit]) {
				report("looked at single bit incorrect value!\n")
			}
		}
		if tbit == 1 {
			if inPb.Read1() != int32(b[i]&mask[tbit]) {
				report("read incorrect single bit value!\n")
			}
		} else if uint32(inPb.Read(tbit)) != (b[i] & mask[tbit]) {
			report("read incorrect value!\n")
		}
	}
	if inPb.Bytes() != bytes {
		report("leftover bytes after read!\n")
	}
}

func cliptestB(b []uint32, vals int, bits int8, comp []byte, compsize int) {
	var bytes, i int32
	var buffer []byte

	outPb.ResetB()
	for i = 0; i < int32(vals); i++ {
		if bits != 0 {
			outPb.WriteB(b[i], bits)
		} else {
			outPb.WriteB(b[i], ilog(b[i]))
		}
	}
	buffer = outPb.GetBufferB()
	bytes = outPb.BytesB()
	if bytes != int32(compsize) {
		report("wrong number of bytes!\n")
	}
	for i = 0; i < bytes; i++ {
		if buffer[i] != comp[i] {
			for i = 0; i < bytes; i++ {
				fmt.Fprintf(os.Stderr, "%x %x\n", int(buffer[i]), int(comp[i]))
			}
			report("wrote incorrect value!\n")
		}
	}
	inPb.ReadInitB(buffer)
	for i = 0; i < int32(vals); i++ {
		var tbit int8
		if bits != 0 {
			tbit = bits
		} else {
			tbit = ilog(b[i])
		}

		if inPb.LookB(tbit) == -1 {
			report("out of data!\n")
		}
		if inPb.LookB(tbit) != int32(b[i]&mask[tbit]) {
			report("looked at incorrect value!\n")
		}
		if tbit == 1 {
			if inPb.Look1B() != int32(b[i]&mask[tbit]) {
				report("looked at single bit incorrect value!\n")
			}
		}
		if tbit == 1 {
			if inPb.Read1B() != int32(b[i]&mask[tbit]) {
				report("read incorrect single bit value!\n")
			}
		} else if inPb.ReadB(tbit) != int32(b[i]&mask[tbit]) {
			report("read incorrect value!\n")
		}
	}
	if inPb.BytesB() != bytes {
		report("leftover bytes after read!\n")
	}
}

var (
	testbuffer1 = []uint32{
		18, 12, 103948, 4325, 543, 76, 432, 52, 3, 65, 4, 56, 32, 42, 34, 21, 1, 23, 32, 546, 456, 7,
		567, 56, 8, 8, 55, 3, 52, 342, 341, 4, 265, 7, 67, 86, 2199, 21, 7, 1, 5, 1, 4}

	test1size int = 43

	testbuffer2 = []uint32{
		216531625, 1237861823, 56732452, 131, 3212421, 12325343, 34547562, 12313212,
		1233432, 534, 5, 346435231, 14436467, 7869299, 76326614, 167548585,
		85525151, 0, 12321, 1, 349528352}

	test2size int = 21

	testbuffer3 = []uint32{
		1, 0, 14, 0, 1, 0, 12, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1,
		0, 1, 30, 1, 1, 1, 0, 0, 1, 0, 0, 0, 12, 0, 11, 0, 1, 0, 0, 1}

	test3size int = 56

	large = []uint32{
		2136531625, 2137861823, 56732452, 131, 3212421, 12325343, 34547562, 12313212,
		1233432, 534, 5, 2146435231, 14436467, 7869299, 76326614, 167548585,
		85525151, 0, 12321, 1, 2146528352}

	onesize int = 33

	one = []byte{146, 25, 44, 151, 195, 15, 153, 176, 233, 131, 196, 65, 85, 172, 47, 40,
		34, 242, 223, 136, 35, 222, 211, 86, 171, 50, 225, 135, 214, 75, 172,
		223, 4}

	oneB = []byte{150, 101, 131, 33, 203, 15, 204, 216, 105, 193, 156, 65, 84, 85, 222,
		8, 139, 145, 227, 126, 34, 55, 244, 171, 85, 100, 39, 195, 173, 18,
		245, 251, 128}

	twosize int = 6

	two = []byte{61, 255, 255, 251, 231, 29}

	twoB = []byte{247, 63, 255, 253, 249, 120}

	threesize int = 54

	three = []byte{169, 2, 232, 252, 91, 132, 156, 36, 89, 13, 123, 176, 144, 32, 254,
		142, 224, 85, 59, 121, 144, 79, 124, 23, 67, 90, 90, 216, 79, 23, 83,
		58, 135, 196, 61, 55, 129, 183, 54, 101, 100, 170, 37, 127, 126, 10,
		100, 52, 4, 14, 18, 86, 77, 1}

	threeB = []byte{206, 128, 42, 153, 57, 8, 183, 251, 13, 89, 36, 30, 32, 144, 183,
		130, 59, 240, 121, 59, 85, 223, 19, 228, 180, 134, 33, 107, 74, 98,
		233, 253, 196, 135, 63, 2, 110, 114, 50, 155, 90, 127, 37, 170, 104,
		200, 20, 254, 4, 58, 106, 176, 144, 0}

	foursize int = 38

	four = []byte{18, 6, 163, 252, 97, 194, 104, 131, 32, 1, 7, 82, 137, 42, 129, 11, 72,
		132, 60, 220, 112, 8, 196, 109, 64, 179, 86, 9, 137, 195, 208, 122, 169,
		28, 2, 133, 0, 1}

	fourB = []byte{36, 48, 102, 83, 243, 24, 52, 7, 4, 35, 132, 10, 145, 21, 2, 93, 2, 41,
		1, 219, 184, 16, 33, 184, 54, 149, 170, 132, 18, 30, 29, 98, 229, 67,
		129, 10, 4, 32}

	fivesize int = 45

	five = []byte{169, 2, 126, 139, 144, 172, 30, 4, 80, 72, 240, 59, 130, 218, 73, 62,
		241, 24, 210, 44, 4, 20, 0, 248, 116, 49, 135, 100, 110, 130, 181, 169,
		84, 75, 159, 2, 1, 0, 132, 192, 8, 0, 0, 18, 22}

	fiveB = []byte{1, 84, 145, 111, 245, 100, 128, 8, 56, 36, 40, 71, 126, 78, 213, 226,
		124, 105, 12, 0, 133, 128, 0, 162, 233, 242, 67, 152, 77, 205, 77,
		172, 150, 169, 129, 79, 128, 0, 6, 4, 32, 0, 27, 9, 0}

	sixsize int = 7

	six = []byte{17, 177, 170, 242, 169, 19, 148}

	sixB = []byte{136, 141, 85, 79, 149, 200, 41}
)

func TestBitwise(t *testing.T) {
	var (
		buffer []byte
		bytes  int32
		i      int
	)

	// Test read/write together
	// Later we test against pregenerated bitstreams
	outPb.WriteInit()

	fmt.Fprintf(os.Stderr, "\nSmall preclipped packing (LSb): ")
	cliptest(testbuffer1, test1size, 0, one, onesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nNull bit call (LSb): ")
	cliptest(testbuffer3, test3size, 0, two, twosize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nLarge preclipped packing (LSb): ")
	cliptest(testbuffer2, test2size, 0, three, threesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\n32 bit preclipped packing (LSb): ")
	outPb.Reset()
	for i = 0; i < test2size; i++ {
		outPb.Write(large[i], 32)
	}
	buffer = outPb.GetBuffer()
	bytes = outPb.Bytes()
	inPb.ReadInit(buffer)
	for i = 0; i < test2size; i++ {
		if inPb.Look(32) == -1 {
			report("out of data. failed!")
		}
		if inPb.Look(32) != int32(large[i]) {
			fmt.Fprintf(os.Stderr, "%ld != %ld (%lx!=%lx):", inPb.Look(32), large[i],
				inPb.Look(32), large[i])
			report("read incorrect value!\n")
		}
		inPb.Adv(32)
	}
	if inPb.Bytes() != bytes {
		report("leftover bytes after read!\n")
	}
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nSmall unclipped packing (LSb): ")
	cliptest(testbuffer1, test1size, 7, four, foursize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nLarge unclipped packing (LSb): ")
	cliptest(testbuffer2, test2size, 17, five, fivesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nSingle bit unclipped packing (LSb): ")
	cliptest(testbuffer3, test3size, 1, six, sixsize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nTesting read past end (LSb): ")
	inPb.ReadInit([]byte{'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'})
	for i = 0; i < 64; i++ {
		if inPb.Read(1) != 0 {
			fmt.Fprintf(os.Stderr, "failed; got -1 prematurely.\n")
			os.Exit(1)
		}
	}
	if inPb.Look(1) != -1 || inPb.Read(1) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	inPb.ReadInit([]byte{'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'})
	if inPb.Read(30) != 0 || inPb.Read(16) != 0 {
		fmt.Fprintf(os.Stderr, "failed 2; got -1 prematurely.\n")
		os.Exit(1)
	}

	if inPb.Look(18) != 0 || inPb.Look(18) != 0 {
		fmt.Fprintf(os.Stderr, "failed 3; got -1 prematurely.\n")
		os.Exit(1)
	}
	if inPb.Look(19) != -1 || inPb.Look(19) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	if inPb.Look(32) != -1 || inPb.Look(32) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	outPb.WriteClear()
	fmt.Fprintf(os.Stderr, "ok.\n")

	// lazy, cut-n-paste retest with MSb packing

	// Test read/write together 
	// Later we test against pregenerated bitstreams 
	outPb.WriteInitB()

	fmt.Fprintf(os.Stderr, "\nSmall preclipped packing (MSb): ")
	cliptestB(testbuffer1, test1size, 0, oneB, onesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nNull bit call (MSb): ")
	cliptestB(testbuffer3, test3size, 0, twoB, twosize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nLarge preclipped packing (MSb): ")
	cliptestB(testbuffer2, test2size, 0, threeB, threesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\n32 bit preclipped packing (MSb): ")
	outPb.ResetB()
	for i = 0; i < test2size; i++ {
		outPb.WriteB(large[i], 32)
	}
	buffer = outPb.GetBufferB()
	bytes = outPb.BytesB()
	inPb.ReadInitB(buffer)
	for i = 0; i < test2size; i++ {
		if inPb.LookB(32) == -1 {
			report("out of data. failed!")
		}
		if inPb.LookB(32) != int32(large[i]) {
			fmt.Fprintf(os.Stderr, "%ld != %ld (%lx!=%lx):", inPb.LookB(32), large[i],
				inPb.LookB(32), large[i])
			report("read incorrect value!\n")
		}
		inPb.AdvB(32)
	}
	if inPb.BytesB() != bytes {
		report("leftover bytes after read!\n")
	}
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nSmall unclipped packing (MSb): ")
	cliptestB(testbuffer1, test1size, 7, fourB, foursize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nLarge unclipped packing (MSb): ")
	cliptestB(testbuffer2, test2size, 17, fiveB, fivesize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nSingle bit unclipped packing (MSb): ")
	cliptestB(testbuffer3, test3size, 1, sixB, sixsize)
	fmt.Fprintf(os.Stderr, "ok.")

	fmt.Fprintf(os.Stderr, "\nTesting read past end (MSb): ")
	inPb.ReadInitB([]byte{'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'})
	for i = 0; i < 64; i++ {
		if inPb.ReadB(1) != 0 {
			fmt.Fprintf(os.Stderr, "failed; got -1 prematurely.\n")
			os.Exit(1)
		}
	}
	if inPb.LookB(1) != -1 || inPb.ReadB(1) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	inPb.ReadInitB([]byte{'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'})
	if inPb.ReadB(30) != 0 || inPb.ReadB(16) != 0 {
		fmt.Fprintf(os.Stderr, "failed 2; got -1 prematurely.\n")
		os.Exit(1)
	}

	if inPb.LookB(18) != 0 || inPb.LookB(18) != 0 {
		fmt.Fprintf(os.Stderr, "failed 3; got -1 prematurely.\n")
		os.Exit(1)
	}
	if inPb.LookB(19) != -1 || inPb.LookB(19) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	if inPb.LookB(32) != -1 || inPb.LookB(32) != -1 {
		fmt.Fprintf(os.Stderr, "failed; read past end without -1.\n")
		os.Exit(1)
	}
	outPb.WriteClearB()
	fmt.Fprintf(os.Stderr, "ok.\n\n")
}
