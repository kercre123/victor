# distutils: language = c++

cdef extern from "anki.h":
  object LoadEmbeddedArray_toNumpy(const char * filename)
  int SaveEmbeddedArray_fromNumpy(object numpyArray, const char *filename, const int compressionLevel) except -1

def loadEmbeddedArray(const char * filename):
  """
  Load the file "filename" into a numpy array. 
  """
  
  try:
    return LoadEmbeddedArray_toNumpy(filename)
  except:
    raise Exception("Could not load from " + filename)  
    
  return None

def saveEmbeddedArray(object numpyArray, const char * filename, const int compressionLevel):
  """
  Save the numpy array "numpyArray" to "filename"
  "compressionLevel" can be from 0 (no compression) to 9 (best compression). 6 is a good choice.
  """
  
  try:
    SaveEmbeddedArray_fromNumpy(numpyArray, filename, compressionLevel)
  except:
    raise Exception("Could not save to " + filename)
