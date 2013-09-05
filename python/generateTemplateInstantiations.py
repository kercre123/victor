import generateArray_h
import generateArray_cpp
import generatePoint_h
import generatePoint_cpp

def generateAll():
    generateArray_h.GenerateAndWriteFile()
    generateArray_cpp.GenerateAndWriteFile()
    
    generatePoint_cpp.GenerateAndWriteFile()
    generatePoint_cpp.GenerateAndWriteFile()

if __name__ == "__main__":
    generateAll()