# distutils: language = c++

cdef extern from "anki.h":
  object loadBinaryArray_toNumpy(const char * filename, const int maxBufferSize)
  
def loadBinaryArray(const char * filename, const int maxBufferSize):
  return loadBinaryArray_toNumpy(filename, maxBufferSize)
    