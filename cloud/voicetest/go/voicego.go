package main

import "C"
import "fmt"

//export GoAudioCallback
func GoAudioCallback(cSamples []int16) {
	// this is temporary memory in C; need to bring it into Go's world
	samples := make([]int16, len(cSamples))
	copy(samples, cSamples)
	fmt.Println("Samples received:", len(samples))
}

//export GoMain
func GoMain() {
	fmt.Println("Hello, C!")
}

// apparently when doing -buildmode=c-archive, we need main()
// I have no clue why
func main() {
}
