package date

import (
	"encoding/json"
	"testing"
	"time"
)

func TestNewDateOk(t *testing.T) {
	d, err := New(2014, 3, 4)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	year, month, day := d.Date()
	if year != 2014 || month != 3 || day != 4 {
		t.Errorf("Incorrect date %v", d)
	}
}

func TestNewDateErr(t *testing.T) {
	_, err := New(0, 0, 0)
	if err == nil {
		t.Error("No error generated")
	}
}

func TestMustNewOk(t *testing.T) {
	var didPanic interface{}
	var d Date
	func() {
		defer func() {
			didPanic = recover()
		}()
		d = MustNew(2014, 3, 4)
	}()

	if didPanic != nil {
		t.Errorf("Panic occurred val=%#v", didPanic)
	}
	year, month, day := d.Date()
	if year != 2014 || month != 3 || day != 4 {
		t.Errorf("Incorrect date %v", d)
	}
}

func TestMustNewErr(t *testing.T) {
	var didPanic interface{}
	func() {
		defer func() {
			didPanic = recover()
		}()
		MustNew(2014, 0, 4)
	}()

	if didPanic == nil {
		t.Error("No panic triggered")
	}
}

func TestToday(t *testing.T) {
	tyear, tmonth, tday := time.Now().Date()
	year, month, day := Today().Date()
	if tyear != year {
		t.Error("Bad year", year)
	}
	if int(tmonth) != month {
		t.Error("Bad month", month)
	}
	if tday != day {
		t.Error("Bad day", day)
	}
}

func TestParseOk(t *testing.T) {
	d, err := Parse(Format, "2014-03-04")
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if d.String() != "2014-03-04" {
		t.Errorf("Parse failed, got %q", d.String())
	}
}

func TestParseErr(t *testing.T) {
	_, err := Parse(Format, "Invalid")
	if err == nil {
		t.Fatal("No error generated")
	}
}

func TestMustParseOk(t *testing.T) {
	var didPanic interface{}
	var d Date
	func() {
		defer func() {
			didPanic = recover()
		}()
		d = MustParse(Format, "2014-03-04")
	}()

	if didPanic != nil {
		t.Errorf("Panic occurred val=%#v", didPanic)
	}

	if d.String() != "2014-03-04" {
		t.Errorf("Parse failed, got %q", d.String())
	}
}

func TestMustParseErr(t *testing.T) {
	var didPanic interface{}
	func() {
		defer func() {
			didPanic = recover()
		}()
		MustParse(Format, "Invalid")
	}()

	if didPanic == nil {
		t.Error("No panic triggered")
	}
}

func TestAddDate(t *testing.T) {
	d := MustNew(2014, 1, 2)
	result := d.AddDate(1, 2, 3).String()
	if result != "2015-03-05" {
		t.Error("Incorrect result", result)
	}
}

func TestAddZeroDate(t *testing.T) {
	var d Date
	result := d.AddDate(1, 2, 3)
	if !result.IsZero() {
		t.Error("Result was not a zero date", result)
	}
}

func TestBefore(t *testing.T) {
	d1, _ := New(2014, 1, 2)
	d2, _ := New(2014, 1, 3)
	if !d1.Before(d2) {
		t.Errorf("d1 < d2 did not return true")
	}
	if d2.Before(d1) {
		t.Errorf("d2 < d1 returned true")
	}
}

func TestAfter(t *testing.T) {
	d1, _ := New(2014, 1, 2)
	d2, _ := New(2014, 1, 3)
	if !d2.After(d1) {
		t.Errorf("d2 > d1 did not return true")
	}
	if d1.After(d2) {
		t.Errorf("d2 > d1 returned true")
	}
}

func TestEqual(t *testing.T) {
	d1, _ := New(2014, 1, 2)
	d2, _ := New(2014, 1, 3)

	if d1.Equal(d2) {
		t.Error("d1 == d2 ?")
	}

	if !d1.Equal(d1) {
		t.Errorf("d1 != d1?")
	}
}

func TestIsZero(t *testing.T) {
	var d Date

	if !d.IsZero() {
		t.Errorf("zero date did not return true from IsZero")
	}

	d, _ = New(2014, 1, 1)
	if d.IsZero() {
		t.Errorf("Not zero date returned true for IsZero")
	}
}

func TestString(t *testing.T) {
	d, _ := New(2014, 2, 3)
	result := d.String()
	if result != "2014-02-03" {
		t.Errorf("Bad result format %q", result)
	}
}

func TestZeroString(t *testing.T) {
	var d Date
	result := d.String()
	if result != "" {
		t.Errorf("Bad result format %q", result)
	}
}

func TestTime(t *testing.T) {
	d := MustNew(2014, 2, 3)
	tm := d.Time(0, 0, 0, 0, time.UTC)
	expected := "2014-02-03T00:00:00Z"
	fmt := tm.Format(time.RFC3339Nano)
	if fmt != expected {
		t.Errorf("expected=%q actual=%q", expected, fmt)
	}
}

var rangeTests = []struct {
	year        int
	month       int
	day         int
	expectedStr string
	shouldError bool
}{
	{0, 1, 1, "0000-01-01", false},
	{9999, 12, 31, "9999-12-31", false},
	{-1, 1, 1, "", true},
	{10000, 12, 31, "", true},
}

type loader func(year, month, day int) (Date, error)

func newLoader(year, month, day int) (Date, error) { return New(year, month, day) }
func timeLoader(year, month, day int) (Date, error) {
	return FromTime(time.Date(year, time.Month(month), day, 0, 0, 0, 0, time.UTC))
}
func TestDateRanges(t *testing.T) {
	for _, test := range rangeTests {
		for _, loader := range []loader{newLoader, timeLoader} {
			d, err := loader(test.year, test.month, test.day)
			if test.shouldError {
				if err == nil {
					t.Errorf("year=%d month=%d day=%d No error generated", test.year, test.month, test.day)
				}
			} else {
				if err != nil {
					t.Errorf("year=%d month=%d day=%d Unexpected error=%v", test.year, test.month, test.day, err)
				} else {
					if result := d.String(); result != test.expectedStr {
						t.Errorf("year=%d month=%d day=%d Incorrect result expected=%q actual=%q", test.year, test.month, test.day, test.expectedStr, result)
					}
				}
			}
		}
	}
}

func TestValue(t *testing.T) {
	d, _ := New(2014, 2, 3)
	result, err := d.Value()
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if cresult, ok := result.(string); !ok || cresult != "2014-02-03" {
		t.Errorf("Bad result ok=%t result=%v", ok, result)
	}
}

func TestScan(t *testing.T) {
	var d Date
	err := d.Scan([]byte("2014-02-03"))
	if err != nil {
		t.Error("Scan failed", err)
	}
	result := d.String()
	if result != "2014-02-03" {
		t.Errorf("Scan failed, got result %q", result)
	}
}

func TestZeroScan(t *testing.T) {
	var d Date
	err := d.Scan([]byte(""))
	if err != nil {
		t.Error("Scan failed", err)
	}
	if !d.IsZero() {
		t.Errorf("Scan failed, got result %#v", d)
	}
}

// doing multiple scans into a date should cause the value to be replaced
func TestReuseScan(t *testing.T) {
	var d Date
	if err := d.Scan([]byte("2014-02-03")); err != nil {
		t.Fatal("Error reading initial value", err)
	}
	if d.String() != "2014-02-03" {
		t.Fatal("Step 1 failed", d.String())
	}
	if err := d.Scan([]byte("")); err != nil {
		t.Fatal("Error reading zero value", err)
	}
	if !d.IsZero() {
		t.Fatal("Did not load zero value")
	}
	if err := d.Scan([]byte("2014-03-04")); err != nil {
		t.Fatal("Error reading third value", err)
	}
	if d.String() != "2014-03-04" {
		t.Fatal("Step 3 failed", d.String())
	}
}

func TestJSONMarshal(t *testing.T) {
	d := MustNew(2014, 3, 4)
	result, err := json.Marshal(d)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	j := string(result)
	if j != `"2014-03-04"` {
		t.Error("Invalid result", j)
	}
}

func TestJSONUnmarshal(t *testing.T) {
	var d Date
	err := json.Unmarshal([]byte(`"2014-03-04"`), &d)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if d.String() != "2014-03-04" {
		t.Error("Invalid result", d.String())
	}
}
