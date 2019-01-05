package util

import (
	"errors"
	"testing"
)

func TestMultierror(t *testing.T) {
	var e Errors
	if e.Error() != nil {
		t.Fatalf("default error should be nil")
	}
	e.Append(nil)
	if e.Error() != nil {
		t.Fatalf("appending nil should still have nil error")
	}
	e.AppendMulti(nil, nil)
	if e.Error() != nil {
		t.Fatalf("appending nils should still have nil error")
	}
	known := errors.New("known")
	e.Append(known)
	if e.Error() != known {
		t.Fatalf("single error should equal contained error")
	}
	if e.Error().Error() != "known" {
		t.Fatalf("single error should have equal string to original")
	}
	e.Append(known)
	expected := "[{known}, {known}]"
	if e.Error().Error() != expected {
		t.Fatalf("unexpected error format: got %s, expected %s", e.Error().Error(), expected)
	}
	e = Errors{}
	e.AppendMulti(known, known)
	if e.Error().Error() != expected {
		t.Fatalf("unexpected error format: got %s, expected %s", e.Error().Error(), expected)
	}
}

func TestNewErrors(t *testing.T) {
	e := NewErrors(nil)
	if e.Error() != nil {
		t.Fatalf("nil error should not return a non-nil error")
	}

	e = NewErrors(nil, nil)
	if e.Error() != nil {
		t.Fatalf("nil errors should not return a non-nil error")
	}

	real := errors.New("blah")
	e = NewErrors(real)
	if e.Error() != real {
		t.Fatalf("single error should map to original error")
	}

	table := []*Errors{
		NewErrors(real, nil),
		NewErrors(nil, real),
		NewErrors(nil, real, nil),
	}
	for _, e = range table {
		if e.Error() != real {
			t.Fatalf("adding nil should not change above test")
		}
	}

	e = NewErrors(real, real)
	if e.Error() == real {
		t.Fatalf("multiple errors should not map to original error")
	}
}
