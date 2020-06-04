package requestrouter

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

type checkTestStruct struct {
	r   []int
	err error
}

func TestCheck(t *testing.T) {
	testCases := []checkTestStruct{
		{r: []int{100, 0, 0}, err: nil},
		{r: []int{80, 20, 0}, err: nil},
		{r: []int{81, 9, 10}, err: nil},
		{r: []int{100, 10, 0}, err: ErrorRatioNot100},
		{r: []int{99, 0, 10}, err: ErrorRatioNot100},
		{r: []int{80, 10, 0}, err: ErrorRatioNot100},
		{r: []int{70, 0, 10}, err: ErrorRatioNot100},
	}

	for _, tc := range testCases {
		r, err := NewRatio(tc.r[0], tc.r[1], tc.r[2], "test")
		assert.Equal(t, tc.err, err)
		if err != nil {
			assert.Nil(t, r)
		} else {
			assert.Equal(t, tc.r[0], r.Value.Dialogflow)
		}
	}
}
