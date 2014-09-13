
PyObject* loadBinaryArray_toNumpy(const char * filename, const int maxBufferSize);
int saveBinaryArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel);