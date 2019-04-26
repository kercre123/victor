package dynamoutil

import (
	"encoding/json"
	"strconv"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

var (
	locUTC, _ = time.LoadLocation("UTC")
)

// MakeStringValue returns a dynamodb AttributeValue for storing a string.
func MakeStringValue(s string) *dynamodb.AttributeValue {
	return &dynamodb.AttributeValue{
		S: aws.String(s),
	}
}

// GetStringValue extracts a string from a DynamoDB attribute value.
func GetStringValue(a *dynamodb.AttributeValue) string {
	return *a.S
}

// MakeTimeValue returns a dynamodb AttributeValue for storing a time
// value formatted as an RFC3339 string.
func MakeTimeValue(t time.Time) *dynamodb.AttributeValue {
	return MakeStringValue(t.UTC().Format(time.RFC3339Nano))
}

// GetTimeValue extracts and parses a time value from a DynamoDB
// attribute value stored as a formatted time string.
func GetTimeValue(a *dynamodb.AttributeValue) (time.Time, error) {
	return time.ParseInLocation(time.RFC3339Nano, GetStringValue(a), locUTC)
}

// MakeUnixTimestampValue returns a dynamodb AttributeValue for storing a
// time value as a 64 bit Unix timestamp in nanoseconds resolution.
func MakeUnixTimestampValue(t time.Time) *dynamodb.AttributeValue {
	return MakeIntValue(t.UTC().UnixNano())
}

// GetTimestampValue extracts and parses a time value from a DynamoDB
// attribute value stored as a Unix timestamp.
func GetTimestampValue(a *dynamodb.AttributeValue) (time.Time, error) {
	ns, err := GetIntValue(a)
	if err != nil {
		return time.Time{}, err
	}
	return time.Unix(0, ns).UTC(), nil
}

// MakeJSONValue returns a dynamodb AttributeValue for storing an object
// as a JSON formatted string.
func MakeJSONValue(data interface{}) (*dynamodb.AttributeValue, error) {
	if js, err := json.Marshal(data); err != nil {
		return nil, err
	} else {
		return MakeStringValue(string(js)), nil
	}
}

// GetJSONValue extracts a JSON-encoded value from a dynamodb
// attribute value.
func GetJSONValue(a *dynamodb.AttributeValue, data interface{}) error {
	raw := []byte(GetStringValue(a))
	return json.Unmarshal(raw, data)
}

// MakeIntValue returns a dynamodb AttributeValue for storing a 64 bit
// integer value.
func MakeIntValue(v int64) *dynamodb.AttributeValue {
	return &dynamodb.AttributeValue{
		N: aws.String(strconv.FormatInt(v, 10)),
	}
}

// GetIntValue extracts and parses an integer from a DynamoDB integer
// attribute value (which annoying stores integers as
// strings...Thanks, Amazon!)
func GetIntValue(a *dynamodb.AttributeValue) (int64, error) {
	return strconv.ParseInt(*a.N, 10, 64)
}

// MakeFloatValue returns a dynamodb AttributeValue for storing a
// float64 value.
func MakeFloatValue(v float64) *dynamodb.AttributeValue {
	return &dynamodb.AttributeValue{
		N: aws.String(strconv.FormatFloat(v, 'f', -1, 64)),
	}
}

// GetFloatValue extracts and parses a float64 from a DynamoDB number
// attribute value.
func GetFloatValue(a *dynamodb.AttributeValue) (float64, error) {
	return strconv.ParseFloat(*a.N, 64)
}

// MakeBinaryValue returns a dynamodb AttributeValue for storing an
// arbitrary sequence of bytes.
func MakeBinaryValue(data []byte) *dynamodb.AttributeValue {
	return &dynamodb.AttributeValue{
		B: data,
	}
}

// GetBinaryValue extracts the array of bytes from a binary dynamodb
// attribute value.
func GetBinaryValue(a *dynamodb.AttributeValue) []byte {
	return a.B
}
