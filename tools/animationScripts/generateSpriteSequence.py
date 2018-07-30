"""
This script can be used to generate the definition.json file.
Run this script from the same directory as the .png image sequence
and it will spit out the definition.json file in the same directory.
"""

import argparse
import os
import glob


HELP_MSG = "Generate a basic JSON file for playing back a sprite sequence"

PATH_TO_SPRITE_SEQ = "EXTERNALS/animation-assets/sprites/spriteSequences"

preSeqString = """
{
  "loop" : "doNothing", 
  "sequence" :[
    {
      "segmentType" : "straightThrough",
      // file names relative to folder
      "fileList": [
"""


postSequenceString = """\n      ]
    }
  ]
}
"""


def writeJsonFile(fullJSONPath, outString):
    if os.path.exists(fullJSONPath):
        os.remove(fullJSONPath)
    f = open(fullJSONPath,"w")
    f.write(outString)
    f.close()


def main(outputFile="definition.json"):
    parser = argparse.ArgumentParser(description=HELP_MSG)
    parser.add_argument('folder-name', help='name of the folder to re-number')
    args = parser.parse_args()
    folderName = getattr(args, 'folder-name')
    fullPath = os.path.join(PATH_TO_SPRITE_SEQ, folderName)
    fullJSONPath = os.path.join(fullPath, outputFile)
    print("Generating JSON definition for " + folderName + " at path " + fullPath)

    # Make a list of all the .png files and sort them alphabetically
    files = glob.glob(os.path.join(fullPath, "*.png"))
    files = map(os.path.basename, files)
    files.sort()

    fileList = ""
    for entry in files:
        fileList += '        "%s",' % entry + os.linesep

    outString = preSeqString + fileList + postSequenceString

    # remove final comma
    commaIdx = outString.rfind(",")
    outString = outString[:commaIdx] + outString[commaIdx + 1:]

    writeJsonFile(fullJSONPath, outString)


if __name__ == "__main__":
    main()


