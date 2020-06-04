// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf_test

import (
	"fmt"
	"io/ioutil"
	"os"

	"github.com/gwatts/iniconf"
)

func ExampleIniConf() {
	// create an example configuration file to work with
	var exampleConf = `
[section one]
key1 = val1
`

	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		fmt.Println("Failed to create tempfile:", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte(exampleConf))
	tf.Close()

	// Load the configuration file
	conf, err := iniconf.LoadFile(tf.Name(), true)
	if err != nil {
		fmt.Println("Failed to load file:", err)
	}

	// Print the current value of section1, key1
	key1, err := conf.EntryString("section one", "key1")
	if err != nil {
		fmt.Println("Couldn't read key1:", err)
		return
	}
	fmt.Println("Key1 is currently set to", key1)

	// Update key1
	conf.SetEntry("section one", "key1", "new value")

	// Add another key in another section
	conf.SetEntry("new section", "some number", 42)

	// Save it
	err = conf.Save()
	if err != nil {
		fmt.Println("Failed to save configuration file:", err)
	}

	fmt.Println("File saved")

	// Output:
	// Key1 is currently set to val1
	// File saved
}

// This example creates two configuration files (though could just as easily
// load one or more from file using LoadFile()) and chains them together
// so that calls to EntryString() will attempt to fetch a value from
// the primary config file first, and then fallback to reading from the default.
//
// There's no limit to the number of config files you can have in a chain
func ExampleConfChain() {
	var primaryConfText = `
[section one]
key1 = val1
`
	var defaultConfText = `
[section one]
key1 = default1
key2 = default2
`
	// Load our primary and default config files
	primaryConf, _ := iniconf.LoadString(primaryConfText, true)
	defaultConf, _ := iniconf.LoadString(defaultConfText, true)

	// create the chain
	chain := iniconf.NewConfChain(primaryConf, defaultConf)

	// key1 will be read from the primary
	key1, _ := chain.EntryString("section one", "key1")

	// primary doesn't have key2 set, so will read from default
	key2, _ := chain.EntryString("section one", "key2")

	fmt.Println("key1:", key1)
	fmt.Println("key2:", key2)

	// Output:
	// key1: val1
	// key2: default2
}
