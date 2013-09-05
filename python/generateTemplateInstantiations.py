import generateArray_h
import generateArray_cpp

def generateAll():
    generateArray_h.GenerateAndWriteFile()
    generateArray_cpp.GenerateAndWriteFile()

if __name__ == "__main__":
    generateAll()