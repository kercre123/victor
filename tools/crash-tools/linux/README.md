# Tools for crash reporting

These are Linux executables, but they take VICOS executables and crash dump files as input.  Run these on a Linux VM.

dump_syms dumps the symbols of a binary or library and creates the symbols

minidump_stackwalk takes a set of those symbols, and a crash dump file, and generates a call stack.

The scripts in this folder are rough helper scripts that you may find helpful.
