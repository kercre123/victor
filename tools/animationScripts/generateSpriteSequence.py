
import argparse
import os
import re
import shutil


PATH_TO_SPRITE_SEQ = "EXTERNALS/animation-assets/sprites/spriteSequences/"

preSeqString = '''
{
  "loop" : "doNothing", 
  "sequence" :[
    {
      "segmentType" : "straightThrough",
      // file names relative to folder
      "fileList": [
'''

postSequenceString = '''\n      ]
    }
  ]
}
'''


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a basic JSON file for playing back a sprite sequence')
    parser.add_argument('folder-name',
                        help='name of the folder to re-number')


    args = parser.parse_args()
    folderName = getattr(args, 'folder-name')
    fullPath = PATH_TO_SPRITE_SEQ + folderName + "/"
    print("Generating JSON definition for " + folderName + " at path " + fullPath)

    files = os.listdir(fullPath)
    files = [f for f in files if (re.search(".png", f) != None)]



    fileList = ""

    for entry in files:
        fileList += "        \"" + (entry + "\",\n")


    outString = preSeqString + fileList + postSequenceString

    # remove final comma
    commaIdx = outString.rfind(",")
    outString = outString[:commaIdx] + outString[commaIdx + 1:]

    fullJSONPath = fullPath + "definition.json"
    if os.path.exists(fullJSONPath):
        os.remove(fullJSONPath)

    f = open(fullJSONPath,"w")
    f.write(outString)
    f.close()


