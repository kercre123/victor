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

 function: code raw packets into framed OggSquish stream and
           decode Ogg streams back into raw packets
 last mod: $Id: framing.c 18052 2011-08-04 17:57:02Z giles $

 note: The CRC code is directly derived from public domain code by
 Ross Williams (ross@guest.adelaide.edu.au).  See docs/framing.html
 for details.

 ********************************************************************/

import (
	"bytes"
	"fmt"
	"log"
	"os"
	"reflect"
	"testing"
)

var (
	streamStateEnc, streamStateDecr StreamState
	syncState                       SyncState

	sequence int64
	lastno   int32
)

func checkpacket(op *Packet, length int, no int, pos int64) {
	if len(op.Packet) != length {
		fmt.Fprintf(os.Stderr, "incorrect packet length (%ld != %ld)!\n", len(op.Packet), length)
		os.Exit(1)
	}
	if op.GranulePos != pos {
		fmt.Fprintf(os.Stderr, "incorrect packet granpos (%ld != %ld)!\n", op.GranulePos, pos)
		os.Exit(1)
	}

	// packet number just follows sequence/gap; adjust the input number for that
	if no == 0 {
		sequence = 0
	} else {
		sequence++
		if no > int(lastno+1) {
			sequence++
		}
	}
	lastno = int32(no)
	if op.PacketNo != sequence {
		fmt.Fprintf(os.Stderr, "incorrect packet sequence %d != %d\n",
			op.PacketNo, sequence)
		os.Exit(1)
	}

	// Test data 
	for j := range op.Packet {
		if op.Packet[j] != (byte(j+no) & 0xff) {
			fmt.Fprintf(os.Stderr, "body data mismatch (1) at pos %d: %x!=%x!\n\n",
				j, op.Packet[j], (j+no)&0xff)
			os.Exit(1)
		}
	}
}

func check_page(data []byte, header []byte, page *Page) {
	// Test data 
	for j := range page.Body {
		if page.Body[j] != data[j] {
			fmt.Fprintf(os.Stderr, "body data mismatch (2) at pos %d: %x!=%x!\n\n",
				j, data[j], page.Body[j])
			os.Exit(1)
		}
	}

	// Test header 
	for j := range page.Header {
		if page.Header[j] != header[j] {
			fmt.Fprintf(os.Stderr, "header content mismatch at pos %d:\n", j)
			for j = 0; j < int(header[26])+27; j++ {
				fmt.Fprintf(os.Stderr, " (%d)%02x:%02x", j, header[j], page.Header[j])
			}
			fmt.Fprintf(os.Stderr, "\n")
			os.Exit(1)
		}
	}
	if len(page.Header) != int(header[26])+27 {
		fmt.Fprintf(os.Stderr, "header length incorrect! (%d!=%d)\n",
			len(page.Header), int(header[26])+27)
		os.Exit(1)
	}
}

func print_header(page *Page) {
	fmt.Fprintf(os.Stderr, "\nHEADER:\n")
	fmt.Fprintf(os.Stderr, "  capture: %c %c %c %c  version: %d  flags: %x\n",
		page.Header[0], page.Header[1], page.Header[2], page.Header[3],
		int(page.Header[4]), int(page.Header[5]))

	fmt.Fprintf(os.Stderr, "  granulepos: %d  serialno: %d  pageno: %ld\n",
		(page.Header[9]<<24)|(page.Header[8]<<16)|
			(page.Header[7]<<8)|page.Header[6],
		(page.Header[17]<<24)|(page.Header[16]<<16)|
			(page.Header[15]<<8)|page.Header[14],
		(page.Header[21]<<24)|(page.Header[20]<<16)|
			(page.Header[19]<<8)|page.Header[18])

	fmt.Fprintf(os.Stderr, "  checksum: %02x:%02x:%02x:%02x\n  segments: %d (",
		int(page.Header[22]), int(page.Header[23]),
		int(page.Header[24]), int(page.Header[25]),
		int(page.Header[26]))

	for j := 27; j < len(page.Header); j++ {
		fmt.Fprintf(os.Stderr, "%d ", page.Header[j])
	}
	fmt.Fprintf(os.Stderr, ")\n\n")
}

func copy_page(page *Page) {
	temp := make([]byte, len(page.Header))
	copy(temp, page.Header)
	page.Header = temp

	temp = make([]byte, len(page.Body))
	copy(temp, page.Body)
	page.Body = temp
}

func free_page(page *Page) {
	page.Header = nil
	page.Body = nil
}

func Error() {
	fmt.Fprintf(os.Stderr, "error!\n")
	os.Exit(1)
}

// 17 only 
var head1_0 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0x15, 0xed, 0xec, 0x91,
	1, 17}

// 17, 254, 255, 256, 500, 510, 600 byte, pad 
var head1_1 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0x59, 0x10, 0x6c, 0x2c,
	1, 17}

var head2_1 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x04,
	0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0x89, 0x33, 0x85, 0xce,
	13, 254, 255, 0, 255, 1, 255, 245, 255, 255, 0,
	255, 255, 90}

// nil packets; beginning,middle,end 
var head1_2 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_2 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x04,
	0x07, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0x5c, 0x3f, 0x66, 0xcb,
	17, 17, 254, 255, 0, 0, 255, 1, 0, 255, 245, 255, 255, 0,
	255, 255, 90, 0}

// large initial packet 
var head1_3 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0x01, 0x27, 0x31, 0xaa,
	18, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 10}

var head2_3 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x04,
	0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0x7f, 0x4e, 0x8a, 0xd2,
	4, 255, 4, 255, 0}

// continuing packet test
var head1_4 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_4 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0xf8, 0x3c, 0x19, 0x79,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255}

var head3_4 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x05,
	0x07, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 2, 0, 0, 0,
	0x38, 0xe6, 0xb6, 0x28,
	6, 255, 220, 255, 4, 255, 0}

// spill expansion test 
var head1_4b = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_4b = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x00,
	0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0xce, 0x8f, 0x17, 0x1a,
	23, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 10, 255, 4, 255, 0, 0}

var head3_4b = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x04,
	0x07, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 2, 0, 0, 0,
	0x9b, 0xb2, 0x50, 0xa1,
	1, 0}

// page with the 255 segment limit 
var head1_5 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_5 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x00,
	0x07, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0xed, 0x2a, 0x2e, 0xa7,
	255,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10}

var head3_5 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x04,
	0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 2, 0, 0, 0,
	0x6c, 0x3b, 0x82, 0x3d,
	1, 50}

// packet that overspans over an entire page 
var head1_6 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_6 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x00,
	0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0x68, 0x22, 0x7c, 0x3d,
	255, 100,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255}

var head3_6 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x01,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x01, 0x02, 0x03, 0x04, 2, 0, 0, 0,
	0xf4, 0x87, 0xba, 0xf3,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255}

var head4_6 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x05,
	0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 3, 0, 0, 0,
	0xf7, 0x2f, 0x6c, 0x60,
	5, 254, 255, 4, 255, 0}

// packet that overspans over an entire page 
var head1_7 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0,
	0xff, 0x7b, 0x23, 0x17,
	1, 0}

var head2_7 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x00,
	0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 1, 0, 0, 0,
	0x68, 0x22, 0x7c, 0x3d,
	255, 100,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255}

var head3_7 = []byte{0x4f, 0x67, 0x67, 0x53, 0, 0x05,
	0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 2, 0, 0, 0,
	0xd4, 0xe0, 0x60, 0xe5,
	1, 0}

func test_pack(pl []int32, headers [][]byte, byteskip, pageskip, packetskip int) {
	var (
		data          = make([]byte, 1024*1024) // for scripted test cases only
		inptr         int
		outptr        int
		deptr         int
		depacket      int32
		granule_pos   int64 = 7
		pageno        int
		packets       int
		pageout       = pageskip
		eosflag       bool
		bosflag       bool
		byteskipcount int
		err           error
	)

	streamStateEnc.Reset()
	streamStateDecr.Reset()
	syncState.Reset()

	for packets = 0; packets < packetskip; packets++ {
		depacket += pl[packets]
	}

	for packets = 0; ; packets++ {
		if pl[packets] == -1 {
			break
		}
	}

	for i := 0; i < packets; i++ {

		var op Packet // construct a test packet
		length := int(pl[i])

		op.Packet = data[inptr : inptr+length]
		if pl[i+1] < 0 {
			op.EOS = true
		} else {
			op.EOS = false
		}
		op.GranulePos = granule_pos

		granule_pos += 1024

		for j := range op.Packet {
			op.Packet[j] = byte(i) + byte(j)
			inptr++
		}

		// submit the test packet 
		if err = streamStateEnc.PacketIn(&op); err != nil {
			fmt.Println("streamStateEnc PacketIn returned not null")
		}

		// retrieve any finished pages 
		var page Page

		for streamStateEnc.PageOut(&page) == true {

			// We have a page.  Check it carefully 

			fmt.Fprintf(os.Stderr, "%d, ", pageno)

			if headers[pageno] == nil {
				fmt.Fprintf(os.Stderr, "coded too many pages!\n")
			}

			check_page(data[outptr:], headers[pageno], &page)

			outptr += len(page.Body)
			pageno++
			if pageskip != 0 {
				bosflag = true
				pageskip--
				deptr += len(page.Body)
			}

			// have a complete page; submit it to sync/decode 

			var pageDec Page
			var packetDec, packetDec2 Packet
			buf := syncState.Buffer(len(page.Header) + len(page.Body))
			if buf == nil {
				log.Fatal("buffer is nil\n")
			}
			var offset int
			byteskipcount += len(page.Header)
			if byteskipcount > int(byteskip) {
				copy(buf, page.Header[0:byteskipcount-byteskip])
				offset += byteskipcount - byteskip
				byteskipcount = byteskip
			}

			byteskipcount += len(page.Body)
			if byteskipcount > byteskip {
				copy(buf[offset:], page.Body[0:byteskipcount-byteskip])
				offset += (byteskipcount - byteskip)
				byteskipcount = byteskip
			}

			if syncState.Wrote(offset) == -1 {
				log.Fatal("func Wrote returned -1")
			}

			for {
				ret := syncState.PageOut(&pageDec)
				if ret == 0 {
					break
				}
				if ret < 0 {
					continue
				}
				// got a page.  Happy happy.  Verify that it's good. 

				fmt.Fprintf(os.Stderr, "(%d)", pageout)

				check_page(data[deptr:], headers[pageout], &pageDec)
				deptr += len(pageDec.Body)
				pageout++

				// submit it to deconstitution 
				if err = streamStateDecr.PageIn(&pageDec); err != nil {
					log.Fatal(err)
				}

				// packets out? 
				for streamStateDecr.PacketPeek(&packetDec2) > 0 {

					streamStateDecr.PacketPeek(nil)
					streamStateDecr.PacketOut(&packetDec) // just catching them all 
					// verify peek and out match 
					if reflect.DeepEqual(packetDec, packetDec2) == false {
						fmt.Fprintf(os.Stderr, "packetout != packetpeek! pos=%d\n",
							depacket)
						os.Exit(1)
					}

					// verify the packet! 
					// check data 
					if bytes.Equal(data[depacket:depacket+int32(len(packetDec.Packet))], packetDec.Packet) == false {
						fmt.Fprintf(os.Stderr, "packet data mismatch in decode! pos=%d\n",
							depacket)
						os.Exit(1)
					}
					// check bos flag 
					if bosflag == false && packetDec.BOS == false {
						fmt.Fprintf(os.Stderr, "BOS flag not set on packet!\n")
						os.Exit(1)
					}
					if bosflag && packetDec.BOS != false {
						fmt.Fprintf(os.Stderr, "BOS flag incorrectly set on packet!\n")
						os.Exit(1)
					}
					bosflag = true
					depacket += int32(len(packetDec.Packet))

					// check eos flag 
					if eosflag {
						fmt.Fprintf(os.Stderr, "Multiple decoded packets with eos flag!\n")
						os.Exit(1)
					}

					if packetDec.EOS != false {
						eosflag = true
					}

					// check granulepos flag 
					if packetDec.GranulePos != -1 {
						fmt.Fprintf(os.Stderr, " granule:%d ", packetDec.GranulePos)
					}
				}
			}
		}
	}
	if headers[pageno] != nil {
		fmt.Fprintf(os.Stderr, "did not write last page!\n")
	}
	if headers[pageout] != nil {
		fmt.Fprintf(os.Stderr, "did not decode last page!\n")
	}
	if inptr != outptr {
		fmt.Fprintf(os.Stderr, "encoded page data incomplete!\n")
	}
	if inptr != deptr {
		fmt.Fprintf(os.Stderr, "decoded page data incomplete!\n")
	}
	if inptr != int(depacket) {
		fmt.Fprintf(os.Stderr, "decoded packet data incomplete!\n")
	}
	if eosflag == false {
		fmt.Fprintf(os.Stderr, "Never got a packet with eos set!\n")
	}
	fmt.Fprintf(os.Stderr, "ok.\n")
}

//
// Main test routine
//
func TestFraming(t *testing.T) {

	streamStateEnc.Init(0x04030201)
	streamStateDecr.Init(0x04030201)

	// Exercise each code path in the framing code.  Also verify that
	// the checksums are working.

	{
		// 17 only 
		var packets = []int32{17, -1}
		var headret = [][]byte{head1_0, nil}

		fmt.Fprintf(os.Stderr, "testing single page encoding... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// 17, 254, 255, 256, 500, 510, 600 byte, pad 
		var packets = []int32{17, 254, 255, 256, 500, 510, 600, -1}
		var headret = [][]byte{head1_1, head2_1, nil}

		fmt.Fprintf(os.Stderr, "testing basic page encoding... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// nil packets; beginning,middle,end 
		var packets = []int32{0, 17, 254, 255, 0, 256, 0, 500, 510, 600, 0, -1}
		var headret = [][]byte{head1_2, head2_2, nil}

		fmt.Fprintf(os.Stderr, "testing basic nil packets... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// large initial packet 
		var packets = []int32{4345, 259, 255, -1}
		var headret = [][]byte{head1_3, head2_3, nil}

		fmt.Fprintf(os.Stderr, "testing initial-packet lacing > 4k... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// continuing packet test; with page spill expansion, we have to
		// overflow the lacing table. 
		var packets = []int32{0, 65500, 259, 255, -1}
		var headret = [][]byte{head1_4, head2_4, head3_4, nil}

		fmt.Fprintf(os.Stderr, "testing single packet page span... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// spill expand packet test
		var packets = []int32{0, 4345, 259, 255, 0, 0, -1}
		var headret = [][]byte{head1_4b, head2_4b, head3_4b, nil}

		fmt.Fprintf(os.Stderr, "testing page spill expansion... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	// page with the 255 segment limit 
	{
		var packets = []int32{0,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 50, -1}
		var headret = [][]byte{head1_5, head2_5, head3_5, nil}

		fmt.Fprintf(os.Stderr, "testing max packet segments... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// packet that overspans over an entire page
		var packets = []int32{0, 100, 130049, 259, 255, -1}
		var headret = [][]byte{head1_6, head2_6, head3_6, head4_6, nil}

		fmt.Fprintf(os.Stderr, "testing very large packets... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// test for the libogg 1.1.1 resync in large continuation bug
		// found by Josh Coalson)
		var packets = []int32{0, 100, 130049, 259, 255, -1}
		var headret = [][]byte{head1_6, head2_6, head3_6, head4_6, nil}

		fmt.Fprintf(os.Stderr, "testing continuation resync in very large packets... ")
		test_pack(packets, headret, 100, 2, 3)
	}

	{
		// term only page.  why not?
		var packets = []int32{0, 100, 64770, -1}
		var headret = [][]byte{head1_7, head2_7, head3_7, nil}

		fmt.Fprintf(os.Stderr, "testing zero data page (1 nil packet)... ")
		test_pack(packets, headret, 0, 0, 0)
	}

	{
		// build a bunch of pages for testing
		data := make([]byte, 1024*1024)
		pl := []int32{0, 1, 1, 98, 4079, 1, 1, 2954, 2057, 76, 34, 912, 0, 234, 1000, 1000, 1000, 300, -1}
		var inptr, i int32
		var page [5]Page

		streamStateEnc.Reset()

		for i = 0; pl[i] != -1; i++ {
			var op Packet
			length := pl[i]

			op.Packet = data[inptr : inptr+length]
			if pl[i+1] < 0 {
				op.EOS = true
			} else {
				op.EOS = false
			}
			op.GranulePos = int64((i + 1) * 1000)

			for j := range op.Packet {
				op.Packet[j] = byte(i) + byte(j)
			}
			streamStateEnc.PacketIn(&op)
		}

		data = nil

		// retrieve finished pages 
		for i = 0; i < 5; i++ {
			if streamStateEnc.PageOut(&page[i]) == false {
				fmt.Fprintf(os.Stderr, "Too few pages output building sync tests!\n")
				os.Exit(1)
			}
			copy_page(&page[i])
		}

		// Test lost pages on pagein/packetout: no rollback 
		{
			var temp Page
			var test Packet

			fmt.Fprintf(os.Stderr, "Testing loss of pages... ")

			syncState.Reset()
			streamStateDecr.Reset()
			for i = 0; i < 5; i++ {
				copy(syncState.Buffer(len(page[i].Header)), page[i].Header)
				syncState.Wrote(len(page[i].Header))
				copy(syncState.Buffer(len(page[i].Body)), page[i].Body)
				syncState.Wrote(len(page[i].Body))
			}

			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)
			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)
			syncState.PageOut(&temp)
			// skip 
			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)

			// do we get the expected results/packets? 

			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 0, 0, 0)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 1, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 2, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 98, 3, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 4079, 4, 5000)
			if streamStateDecr.PacketOut(&test) != -1 {
				fmt.Fprintf(os.Stderr, "Error: loss of page did not return error\n")
				os.Exit(1)
			}
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 76, 9, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 34, 10, -1)
			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// Test lost pages on pagein/packetout: rollback with continuation 
		{
			var temp Page
			var test Packet

			fmt.Fprintf(os.Stderr, "Testing loss of pages (rollback required)... ")

			syncState.Reset()
			streamStateDecr.Reset()
			for i = 0; i < 5; i++ {
				copy(syncState.Buffer(len(page[i].Header)), page[i].Header)
				syncState.Wrote(len(page[i].Header))
				copy(syncState.Buffer(len(page[i].Body)), page[i].Body)
				syncState.Wrote(len(page[i].Body))
			}

			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)
			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)
			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)
			syncState.PageOut(&temp)
			// skip 
			syncState.PageOut(&temp)
			streamStateDecr.PageIn(&temp)

			// do we get the expected results/packets? 

			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 0, 0, 0)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 1, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 2, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 98, 3, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 4079, 4, 5000)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 5, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 1, 6, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 2954, 7, -1)
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 2057, 8, 9000)
			if streamStateDecr.PacketOut(&test) != -1 {
				fmt.Fprintf(os.Stderr, "Error: loss of page did not return error\n")
				os.Exit(1)
			}
			if streamStateDecr.PacketOut(&test) != 1 {
				Error()
			}
			checkpacket(&test, 300, 17, 18000)
			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// the rest only test sync
		{
			var pageDec Page
			// Test fractional page inputs: incomplete capture 
			fmt.Fprintf(os.Stderr, "Testing sync on partial inputs... ")
			syncState.Reset()
			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[0:3])
			syncState.Wrote(3)
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			// Test fractional page inputs: incomplete fixed header 
			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[3:3+20])
			syncState.Wrote(20)
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			// Test fractional page inputs: incomplete header 
			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[23:23+5])
			syncState.Wrote(5)
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			// Test fractional page inputs: incomplete body 

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[28:])
			syncState.Wrote(len(page[1].Header) - 28)
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			copy(syncState.Buffer(len(page[1].Body)), page[1].Body[0:1000])
			syncState.Wrote(1000)
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			copy(syncState.Buffer(len(page[1].Body)), page[1].Body[1000:])
			syncState.Wrote(len(page[1].Body) - 1000)
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}

			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// Test fractional page inputs: page + incomplete capture 
		{
			var pageDec Page
			fmt.Fprintf(os.Stderr, "Testing sync on 1+partial inputs... ")
			syncState.Reset()

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header)
			syncState.Wrote(len(page[1].Header))

			copy(syncState.Buffer(len(page[1].Body)), page[1].Body)
			syncState.Wrote(len(page[1].Body))

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[0:20])
			syncState.Wrote(20)
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header[20:])
			syncState.Wrote(len(page[1].Header) - 20)
			copy(syncState.Buffer(len(page[1].Body)), page[1].Body)
			syncState.Wrote(len(page[1].Body))
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}

			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// Test recapture: garbage + page 
		{
			var pageDec Page
			fmt.Fprintf(os.Stderr, "Testing search for capture... ")
			syncState.Reset()

			// 'garbage' 
			copy(syncState.Buffer(len(page[1].Body)), page[1].Body)
			syncState.Wrote(len(page[1].Body))

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header)
			syncState.Wrote(len(page[1].Header))

			copy(syncState.Buffer(len(page[1].Body)), page[1].Body)
			syncState.Wrote(len(page[1].Body))

			copy(syncState.Buffer(len(page[2].Header)), page[2].Header[0:20])
			syncState.Wrote(20)

			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}
			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}

			copy(syncState.Buffer(len(page[2].Header)), page[2].Header[20:])
			syncState.Wrote(len(page[2].Header) - 20)
			copy(syncState.Buffer(len(page[2].Body)), page[2].Body)
			syncState.Wrote(len(page[2].Body))
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}

			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// Test recapture: page + garbage + page 
		{
			var pageDec Page
			fmt.Fprintf(os.Stderr, "Testing recapture... ")
			syncState.Reset()

			copy(syncState.Buffer(len(page[1].Header)), page[1].Header)
			syncState.Wrote(len(page[1].Header))

			copy(syncState.Buffer(len(page[1].Body)), page[1].Body)
			syncState.Wrote(len(page[1].Body))

			copy(syncState.Buffer(len(page[2].Header)), page[2].Header)
			syncState.Wrote(len(page[2].Header))

			copy(syncState.Buffer(len(page[2].Header)), page[2].Header)
			syncState.Wrote(len(page[2].Header))

			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}

			copy(syncState.Buffer(len(page[2].Body)), page[2].Body[0:len(page[2].Body)-5])
			syncState.Wrote(len(page[2].Body) - 5)

			copy(syncState.Buffer(len(page[3].Header)), page[3].Header)
			syncState.Wrote(len(page[3].Header))

			copy(syncState.Buffer(len(page[3].Body)), page[3].Body)
			syncState.Wrote(len(page[3].Body))

			if syncState.PageOut(&pageDec) > 0 {
				Error()
			}
			if syncState.PageOut(&pageDec) <= 0 {
				Error()
			}

			fmt.Fprintf(os.Stderr, "ok.\n")
		}

		// Free page data that was previously copied 
		for i = 0; i < 5; i++ {
			free_page(&page[i])
		}
	}
}
