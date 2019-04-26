package opus

import (
	"bytes"
	"encoding/binary"
)

const vendorString = "libopus"

// header specification: https://wiki.xiph.org/OggOpus#ID_Header
func (s *OggStream) writeHeader(streambuf *bytes.Buffer) error {
	buf := bytes.Buffer{}
	// 8 byte magic tag
	if n, err := buf.Write([]byte("OpusHead")); n != 8 || err != nil {
		return getWriteError(err, n, "Invalid header write")
	}
	// 1 byte version (fixed 0x01), 1 byte # of channels, 2 byte pre skip (hard code to 0)
	if n, err := buf.Write([]byte{0x01, byte(s.Channels), 0, 0}); n != 4 || err != nil {
		return getWriteError(err, n, "Invalid spec version/# channels/pre-skip write")
	}
	// 4 byte sample rate
	if err := binary.Write(&buf, binary.LittleEndian, uint32(s.SampleRate)); err != nil {
		return err
	}
	// 2 byte output gain (hard code to 0)
	// 1 byte "channel map" (0 for mono or L/R stereo, assume this)
	if n, err := buf.Write([]byte{0, 0, 0}); n != 3 || err != nil {
		return getWriteError(err, n, "Invalid output gain/channel map write")
	}

	// this is the first header packet of two - submit to Ogg stream and add bytes
	{
		packet := s.stream.NewSeqPacket(buf.Bytes(), 0)
		packet.PacketNo = 0
		oggData, count, err := s.stream.SubmitPacket(packet, true)
		if count != 1 || err != nil {
			return getWriteError(err, count, "Invalid header page flush size")
		}
		if n, err := streambuf.Write(oggData); n != len(oggData) || err != nil {
			return getWriteError(err, count, "Invalid header buf write")
		}
	}

	// second header page
	buf.Reset()
	// 8 byte magic tag
	if n, err := buf.Write([]byte("OpusTags")); n != 8 || err != nil {
		return getWriteError(err, n, "Invalid tags write")
	}
	// 4 byte little endian vendor string length
	if err := binary.Write(&buf, binary.LittleEndian, uint32(len(vendorString))); err != nil {
		return err
	}
	// vendor string
	if n, err := buf.Write([]byte(vendorString)); n != len(vendorString) || err != nil {
		return getWriteError(err, n, "Invalid vendor string write")
	}
	// 4 byte number of tag strings that follow (hard code to 0)
	if n, err := buf.Write([]byte{0, 0, 0, 0}); n != 4 || err != nil {
		return getWriteError(err, n, "Invalid tag count write")
	}

	// this is the second header packet of two - submit to Ogg stream and add bytes
	{
		packet := s.stream.NewSeqPacket(buf.Bytes(), 0)
		packet.PacketNo = 0
		oggData, count, err := s.stream.SubmitPacket(packet, true)
		if count != 1 || err != nil {
			return getWriteError(err, count, "Invalid tags page flush size")
		}
		if n, err := streambuf.Write(oggData); n != len(oggData) || err != nil {
			return getWriteError(err, count, "Invalid tags buf write")
		}
	}

	return nil
}
