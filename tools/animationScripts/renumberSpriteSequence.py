
import argparse
import os
import re
import shutil


PATH_TO_SPRITE_SEQ = "EXTERNALS/animation-assets/sprites/spriteSequences/"


def NumberOfZeroesToInsert(numDigits, number):
    digitsInNum = 0
    while number/10 > 0:
        number = number/10
        digitsInNum += 1
    return numDigits - digitsInNum

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Re-number a folder of sprites to start at zero')
    parser.add_argument('folder-name',
                        help='name of the folder to re-number')
    parser.add_argument('output-folder-name',
                        help='name of the folder to write the new files to')


    args = parser.parse_args()
    folderName = getattr(args, 'folder-name')
    fullPath = PATH_TO_SPRITE_SEQ + folderName + "/"
    outputFolderName = getattr(args, 'output-folder-name')
    fullOutputFolderName = PATH_TO_SPRITE_SEQ + outputFolderName + "/"
    print("Will re-number sprites in folder " + folderName + " at path " + fullPath)

    files = os.listdir(fullPath)
    numberToNameMap = {}
    for name in files:
        sequence_number_list = re.findall(r'\d*\.', name)
        if len(sequence_number_list) != 1:
            print("ERROR: More than one number in file name " + name)
            continue
        sequence_number = int(sequence_number_list[0][:-1])
        numberToNameMap[sequence_number] = name

    sortedMap = sorted(numberToNameMap.items(), key=lambda x: x)
    
    # determine the number of digits to keep files orederd in file browser
    numberImgs = len(sortedMap)
    numDigits = 1
    while numberImgs/10 > 0:
        numberImgs = numberImgs/10
        numDigits += 1

    newNumber = 0
    if os.path.isdir(fullOutputFolderName):
      shutil.rmtree(fullOutputFolderName)
    os.mkdir(fullOutputFolderName)
    for entry in sortedMap:
        finalUnderscore = entry[1].rfind("_")
        fileNoNumber = entry[1][:finalUnderscore]
        howManyZeros = NumberOfZeroesToInsert(numDigits, newNumber)
        zerosStr = ""
        for i in range(howManyZeros):
            zerosStr += "0"

        newNumberedFile = fileNoNumber + "_" + zerosStr + str(newNumber) + ".png"
        newNumber += 1

        shutil.copy(fullPath + entry[1],  fullOutputFolderName + newNumberedFile)

