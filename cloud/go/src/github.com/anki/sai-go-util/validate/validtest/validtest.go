// Test related utilities for validators
package validtest

import (
	"testing"

	"github.com/anki/sai-go-util/validate"
)

func ApplyToTest(prefix string, v *validate.Validator, t *testing.T) (errCount int) {
	if errs := v.Validate(); errs != nil {
		for _, fieldErrs := range errs {
			for _, err := range fieldErrs {
				t.Errorf("%s - Validate failed for field=%q code=%q message=%q", prefix, err.FieldName, err.Code, err.Message)
				errCount++
			}
		}
	}
	return errCount
}
