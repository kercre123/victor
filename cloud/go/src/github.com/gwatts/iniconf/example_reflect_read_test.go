package iniconf_test

import "fmt"
import "github.com/gwatts/iniconf"

var inifile = `
[mysection]
value1 = First value
num1 = 123
bool1 = true
subvalue = nested value
`

type section struct {
	Value1 string `iniconf:"value1"`
	Num1   int    `iniconf:"num1"`
	Bool1  bool   `iniconf:"bool1"`
	Sub    struct {
		SubValue string `iniconf:"subvalue"`
	}
}

func ExampleIniConf_ReadSection() {
	var s section
	cfg, _ := iniconf.LoadString(inifile, false)
	if err := cfg.ReadSection("mysection", &s); err != nil {
		fmt.Println("Failed", err)
		return
	}

	fmt.Println("Value1:", s.Value1)
	fmt.Println("Num1:", s.Num1)
	fmt.Println("Bool1:", s.Bool1)
	fmt.Println("Sub:", s.Sub.SubValue)

	// Output:
	// Value1: First value
	// Num1: 123
	// Bool1: true
	// Sub: nested value
}
