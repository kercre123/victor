package dynamoutil

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestStringValue(t *testing.T) {
	v := MakeStringValue("foo")
	assert.NotNil(t, v)
	assert.Equal(t, "foo", *v.S)
	assert.Equal(t, "foo", GetStringValue(v))
}

func TestIntValue(t *testing.T) {
	v := MakeIntValue(100)
	assert.NotNil(t, v)
	assert.Equal(t, "100", *v.N)
	i, e := GetIntValue(v)
	assert.Nil(t, e)
	assert.Equal(t, int64(100), i)
}

func TestFloatValue(t *testing.T) {
	v := MakeFloatValue(100)
	assert.NotNil(t, v)
	assert.Equal(t, "100", *v.N)
	i, e := GetFloatValue(v)
	assert.Nil(t, e)
	assert.Equal(t, float64(100), i)
}

func TestTimeValue(t *testing.T) {
	now := time.Now().UTC()
	v := MakeTimeValue(now)
	assert.Equal(t, now.Format(time.RFC3339Nano), *v.S)
	now2, err := GetTimeValue(v)
	assert.Nil(t, err)
	assert.Equal(t, now.Format(time.RFC3339Nano), now2.Format(time.RFC3339Nano))
	loc, _ := time.LoadLocation("UTC")
	t2, _ := time.ParseInLocation(time.RFC3339Nano, *v.S, loc)
	assert.Equal(t, now, t2)
}

func TestUnixTimestampValue(t *testing.T) {
	now := time.Now().UTC()
	v := MakeUnixTimestampValue(now)
	nano, err := GetIntValue(v)
	assert.Nil(t, err)
	assert.Equal(t, now.UnixNano(), nano)
	ts, err := GetTimestampValue(v)
	assert.Nil(t, err)
	assert.Equal(t, now, ts)
}

func TestJSONValue(t *testing.T) {
	data := map[string]interface{}{"a": "b", "c": float64(10)}
	v, err := MakeJSONValue(data)
	assert.Nil(t, err)
	assert.Equal(t, "{\"a\":\"b\",\"c\":10}", *v.S)
	var data2 map[string]interface{}
	err = GetJSONValue(v, &data2)
	assert.Nil(t, err)
	assert.Equal(t, data, data2)
}
