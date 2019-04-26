package strutil

import "testing"

var stripEmojiTests = []struct {
	input    string
	expected string
}{
	{"plain ascii", "plain ascii"},
	{"ascii with \u30c6\u30b9\u30c8 japanese", "ascii with \u30c6\u30b9\u30c8 japanese"},
	{"ascii with \u30c6\u30b9\u30c8 japanese and \U0001f604 emoji", "ascii with \u30c6\u30b9\u30c8 japanese and \ufffd emoji"},
	{"ascii with \u30c6\u30b9\u30c8 japanese and \U0001f604 emoji1 and \U0001f604 emoji2", "ascii with \u30c6\u30b9\u30c8 japanese and \ufffd emoji1 and \ufffd emoji2"},
	{"ascii with \U0001f604\U0001f604 back to back emoji", "ascii with \ufffd\ufffd back to back emoji"},
}

func TestStripEmoji(t *testing.T) {
	for _, test := range stripEmojiTests {
		in := []byte(test.input)
		result := StripEmoji(in)
		rstr := string(result)
		if rstr != test.expected {
			t.Errorf("Mismatch\nin=      %+q\nexpected=%+q\nactual=  %+q", test.input, test.expected, rstr)
		}
	}
}
