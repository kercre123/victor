import generateArray_vision_h
import generateArray_vision_cpp
import generateFixedLengthList_vision_h
import generateFixedLengthList_vision_cpp

def generateAll():
    generateArray_vision_h.GenerateAndWriteFile()
    generateArray_vision_cpp.GenerateAndWriteFile()
    
    generateFixedLengthList_vision_h.GenerateAndWriteFile()
    generateFixedLengthList_vision_cpp.GenerateAndWriteFile()

if __name__ == "__main__":
    generateAll()