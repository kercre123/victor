import generateArray_vision_h
import generateArray_vision_cpp

def generateAll():
    generateArray_vision_h.GenerateAndWriteFile()
    generateArray_vision_cpp.GenerateAndWriteFile()

if __name__ == "__main__":
    generateAll()