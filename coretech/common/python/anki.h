
PyObject* LoadEmbeddedArray_toNumpy(const char * filename, const int maxBufferSize);
int SaveEmbeddedArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel);