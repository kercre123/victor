package iniconf_test

import "fmt"
import "github.com/gwatts/iniconf"

type cfgsection struct {
	Value1 string `iniconf:"value1"`
	Num1   int    `iniconf:"num1"`
	Bool1  bool   `iniconf:"bool1"`
	Sub    struct {
		SubValue string `iniconf:"subvalue"`
	}
}

func ExampleIniConf_LoadSection() {
	cfg := iniconf.New(false)
	s := cfgsection{
		Value1: "First value",
		Num1:   456,
		Bool1:  true,
	}
	s.Sub.SubValue = "nested value"

	if err := cfg.UpdateSection("mysection", s); err != nil {
		fmt.Println("Failed", err)
	}

	fmt.Println(cfg.String())

	// Output:
	// [mysection]
	// value1 = First value
	// num1 = 456
	// bool1 = true
	// subvalue = nested value
}
