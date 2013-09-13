import generateArray_h
import generateArray_cpp
import generatePoint_h
import generatePoint_cpp
import generateMexEmbeddedWrappers_h
import generateMexEmbeddedWrappers_cpp
import generateFixedLengthList_h
import generateFixedLengthList_cpp

def generateAll():
    generateArray_h.GenerateAndWriteFile()
    generateArray_cpp.GenerateAndWriteFile()

    generatePoint_h.GenerateAndWriteFile()
    generatePoint_cpp.GenerateAndWriteFile()

    generateMexEmbeddedWrappers_h.GenerateAndWriteFile()
    generateMexEmbeddedWrappers_cpp.GenerateAndWriteFile()
    
    generateFixedLengthList_h.GenerateAndWriteFile()
    generateFixedLengthList_cpp.GenerateAndWriteFile()

if __name__ == "__main__":
    generateAll()