
import CladJsonKeys

from os import path

class JsonToCladGenerator:


    def generateCladFile(self, jsonData, outFileDirectory):

        for aFile_Json in jsonData[CladJsonKeys.cladJsonKeyFiles]:
            if CladJsonKeys.cladJsonKeyFileName in aFile_Json:
                fileStr = self.__writeCladFile(aFile_Json)
                filePath = path.join(outFileDirectory, *aFile_Json[CladJsonKeys.cladJsonKeyFileName])
                aFile = open(filePath, 'w')
                aFile.write(fileStr)
                aFile.close()

            else:
                print "No File Name!!!! Major error!"
                exit(1)


        # Generate a clad file from json data
    def __writeCladFile(self, file_json):
        # Write file into string
        cladFileStr = ""

        if CladJsonKeys.cladJsonKeyHeaderComment in file_json:
            cladFileStr += "// " + file_json[CladJsonKeys.cladJsonKeyHeaderComment] + "\n\n"

        cladFileStr += self.__generateNamespaceOpening(file_json[CladJsonKeys.cladJsonKeyNamespaces]) + "\n"

        # Sort Enum types by Title
        enumTypesDict = {}
        for aEnum in file_json[CladJsonKeys.cladJsonKeyEnum]:
            enumTypesDict[aEnum[CladJsonKeys.cladJsonKeyTitle]] = aEnum

        sortedKeys = enumTypesDict.keys()
        sortedKeys.sort()

        # Generate Clad Enums
        for aKey in sortedKeys:
            aEnum = enumTypesDict[aKey]
            # Get Clad File
            cladFileStr += self.__generateCladEnum(aEnum) + "\n"

        cladFileStr += self.__generateNamespaceClosing(file_json[CladJsonKeys.cladJsonKeyNamespaces])

        return cladFileStr


    # Generate Namespace Opening code String
    def __generateNamespaceOpening(self, namespace_json_data):
        namespaceStr = ""
        for aNamespace in namespace_json_data:
            namespaceStr += "namespace %s {\n" % aNamespace

        return namespaceStr

    # Generate Namespace Closing code String
    def __generateNamespaceClosing(self, namespace_json_data):
        namespaceStr = ""
        for aNamespace in reversed(namespace_json_data):
            namespaceStr += "} // %s\n" % aNamespace

        return namespaceStr

    # Generate Clad Enum String
    def __generateCladEnum(self, enum_json_data):
        # Define enum signature
        cladStr = "enum %s %s\n" % (
        enum_json_data[CladJsonKeys.cladJsonKeyType], enum_json_data[CladJsonKeys.cladJsonKeyTitle])
        cladStr += "{\n"

        # Define Members
        # Sort Members by Name
        membersDict = {}
        for aMember in enum_json_data[CladJsonKeys.cladJsonKeyMembers]:
            membersDict[aMember[CladJsonKeys.cladJsonKeyMemberName]] = aMember

        sortedNames = membersDict.keys()
        sortedNames.sort()

        for aMemberKey in sortedNames:
            aMember = membersDict[aMemberKey]

        # for aMember in enum_json_data[CladJsonKeys.cladJsonKeyMembers]:

            cladStr += "  %s" % aMember[CladJsonKeys.cladJsonKeyMemberName]

            if "value" in aMember:
                cladStr += " = %s" % aMember[CladJsonKeys.cladJsonKeyMemberValue]

            cladStr += ","

            if CladJsonKeys.cladJsonKeyMemberComment in aMember:
                cladStr += "  // %s" % aMember[CladJsonKeys.cladJsonKeyMemberComment]

            cladStr += "\n"

        # Close enum member list
        cladStr += "}\n"

        return cladStr
