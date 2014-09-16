/**
File: anki.h
Author: Peter Barnum
Created: 2014-09-12

Python utilities to load and save Embedded Arrays from a file. 
The three files anki.pyx, anki_interface.cpp, and anki.h are compiled by setup.py,
to create a python-compatible shared library.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

PyObject* LoadEmbeddedArray_toNumpy(const char * filename);
int SaveEmbeddedArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel);