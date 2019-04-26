package util

import (
	"os"
	"reflect"
	"testing"
)

func TestSetFromEnv(t *testing.T) {
	var target string = "default"
	os.Setenv("SETENV_SET", "foobar")
	SetFromEnv(&target, "SETENV_NOT_SET")
	if target != "default" {
		t.Errorf("target was changed expected=\"default\" actual=%q", target)
	}
	SetFromEnv(&target, "SETENV_SET")
	if target != "foobar" {
		t.Errorf("target not changed to expected value.  expected=\"foobar\" actual=%q", target)
	}
}

type jtest struct {
	Field1 string `db:"foo" json:"field_one,omitempty"`
	Field2 string
	Field3 string `db:"foo" json:"field_three"`
}

func TestStructFieldByJsonField(t *testing.T) {
	val := jtest{"one", "two", "three"}
	result, err := StructFieldByJsonField(val, "field_three")
	if err != nil {
		t.Fatal("Got error instead of value ", err)
	}
	if result.(string) != "three" {
		t.Fatalf("expected \"three\" got %q", result)
	}

	result, err = StructFieldByJsonField(val, "invalid")
	if err == nil {
		t.Fatalf("Didn't get an error; got result=%#v", result)
	}

	result, err = StructFieldByJsonField(val, "field_one")
	if err != nil {
		t.Fatal("Got error instead of value ", err)
	}
	if result.(string) != "one" {
		t.Fatalf("expected \"one\" got %q", result)
	}

	// check struct ptr
	pval := &val
	result, err = StructFieldByJsonField(pval, "field_one")
	if err != nil {
		t.Fatal("Got error instead of value ", err)
	}
	if result.(string) != "one" {
		t.Fatalf("expected \"one\" got %q", result)
	}

}

type structDeltaTest struct {
	Field1       string `json:"field1"`
	AnotherField int    `json:"field2"`
	IgnoredField int
}

func TestUpdateStructDelta(t *testing.T) {
	existing := structDeltaTest{"F1 Val", 1, 10}
	update := structDeltaTest{"F2 Val", 2, 20}

	changed := UpdateStructDelta(map[string]interface{}{"field1": true}, &update, &existing)
	if len(changed) != 1 || changed[0] != "field1" {
		t.Errorf("Inocrrect result for changed: %v", changed)
	}

	expected := structDeltaTest{"F2 Val", 1, 10} // only field1 should have changed
	if !reflect.DeepEqual(existing, expected) {
		t.Errorf("Update did not generated expected result expected=%#v actual=%#v", expected, existing)
	}
}
