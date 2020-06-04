package strutil

import "unicode/utf8"

// Does an in-place replacement of 4 byte unicode characters
// with the unicode replacement character
func StripEmoji(in []byte) []byte {
	b := in
	var offset int
	for len(b) > 0 {
		_, size := utf8.DecodeRune(b)
		if size == 4 {
			// overwrite with replacement character
			b[0] = 0xef
			b[1] = 0xbf
			b[2] = 0xbd
			// shift remaining bytes 1 byte left to make string valid
			in = append(in[:offset+3], in[offset+4:]...)
			// skip the 3 bytes just written
			b = in[offset+3:]
			offset += 3

		} else {
			b = b[size:]
			offset += size
		}
	}
	return in
}
