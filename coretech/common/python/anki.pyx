# distutils: language = c++

cdef extern from "anki.h":
  object LoadEmbeddedArray_toNumpy(const char * filename, const int maxBufferSize)
  int SaveEmbeddedArray_fromNumpy(object numpyArray, const char *filename, const int compressionLevel) except -1
  
def loadEmbeddedArray(const char * filename, const int maxBufferSize):
  return LoadEmbeddedArray_toNumpy(filename, maxBufferSize)

def saveEmbeddedArray(object numpyArray, const char * filename, const int compressionLevel):
  try:
    SaveEmbeddedArray_fromNumpy(numpyArray, filename, compressionLevel)
  except:
    raise Exception("Could not save to " + filename)
    