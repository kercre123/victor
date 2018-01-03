package main

// typedef void (*voidFunc) ();
//
// void runCFunc(voidFunc f)
// {
//   f();
// }
import "C"

func runCFunc(f C.voidFunc) {
	C.runCFunc(f)
}
