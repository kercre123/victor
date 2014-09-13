# distutils: language = c++

cdef extern from "anki.h":
  object loadBinaryArray_toNumpy(const char * filename, const int maxBufferSize)
  int saveBinaryArray_fromNumpy(object numpyArray, const char *filename, const int compressionLevel) except -1
  
def loadBinaryArray(const char * filename, const int maxBufferSize):
  return loadBinaryArray_toNumpy(filename, maxBufferSize)

def saveBinaryArray(object numpyArray, const char * filename, const int compressionLevel):
  try:
    saveBinaryArray_fromNumpy(numpyArray, filename, compressionLevel)
  except:
    raise Exception("Could not save to " + filename)
    