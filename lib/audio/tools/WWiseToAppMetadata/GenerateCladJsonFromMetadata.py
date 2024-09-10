import os
import sys
import json
import GenerateAudioMetadataSheet
import MetadataKeys
import CladJsonKeys

from GenerateAudioMetadataSheet import ItemRowData


class GenerateCladJsonFromMetadata:

    __eventEnumData = {}
    __wwiseIdsJsonData = None

    __invalidStateName = "Invalid"
    __invalidStateValue = 0

    __invalidGroupMember = {}
    __invalidGroupMember[CladJsonKeys.cladJsonKeyMemberName] = __invalidStateName
    __invalidGroupMember[CladJsonKeys.cladJsonKeyMemberValue] = __invalidStateValue
    __invalidGroupMember[CladJsonKeys.cladJsonKeyMemberComment] = "Use this to check for Invalid events"

    def importMetadata(self, metadata, wwiseIdsJson):

        # Use Metadata csv to genterate event data
        # Create Group to define Event Groups
        eventsGroupName = "GenericEvent"
        eventGroupEnums = []
        self.__eventEnumData[eventsGroupName] = eventGroupEnums
        # Add Invalid enum
        self.__eventEnumData[eventsGroupName].append(self.__invalidGroupMember)

        for anItem in metadata:
            if anItem.itemType == MetadataKeys.dataTypeKeyEvent:
                enumData = self.__importEvent(anItem, eventGroupEnums)
                if enumData != None:
                    self.__eventEnumData[anItem.groupName].append(enumData)


        # TODO: Add other types of metadata
        self.__wwiseIdsJsonData = wwiseIdsJson


    def generateCladJsonData(self, namespaceList):

        # Add file to base container
        outFile = {}
        outFile[CladJsonKeys.cladJsonKeyFiles] = []

        # Event data
        # Event enum data
        eventFileJson = {}
        eventFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioEventTypes.clad"]  # Use array to create path
        eventFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available Event in project"
        eventFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["GameEvent"]
        eventFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateEventCladJsonData()
        outFile[CladJsonKeys.cladJsonKeyFiles].append(eventFileJson)


        # State enum data
        stateFileJson = {}
        stateFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioStateTypes.clad"]  # Use array to create path
        stateFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available Game States in project"
        stateFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["GameState"]
        stateFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateAudioStateEnumCladJsonData(self.__wwiseIdsJsonData["states"], "GenericState")
        outFile[CladJsonKeys.cladJsonKeyFiles].append(stateFileJson)

        # Switch enum data
        switchFileJson = {}
        switchFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioSwitchTypes.clad"]  # Use array to create path
        switchFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available Game Switch States in project"
        switchFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["SwitchState"]
        switchFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateAudioStateEnumCladJsonData(self.__wwiseIdsJsonData["switches"], "GenericSwitch")
        outFile[CladJsonKeys.cladJsonKeyFiles].append(switchFileJson)

        # Game Parameter enum data
        parameterFileJson = {}
        parameterFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioParameterTypes.clad"]
        parameterFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available Game parameters in project"
        parameterFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["GameParameter"]
        parameterFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateAudioEnumCladJsonData(self.__wwiseIdsJsonData["gameParameters"], "ParameterType")
        outFile[CladJsonKeys.cladJsonKeyFiles].append(parameterFileJson)

        # Bank enum data
        bankFileJson = {}
        bankFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioSoundbanks.clad"]  # Use array to create path
        bankFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available Soundbanks in project"
        bankFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["SoundBank"]
        bankFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateAudioEnumCladJsonData(self.__wwiseIdsJsonData["banks"], "BankType")
        outFile[CladJsonKeys.cladJsonKeyFiles].append(bankFileJson)

        # Bus enum data
        busesFileJson = {}
        busesFileJson[CladJsonKeys.cladJsonKeyFileName] = ["audioBusTypes.clad"]  # Use array to create path
        busesFileJson[CladJsonKeys.cladJsonKeyHeaderComment] = "Available audio buses in project"
        busesFileJson[CladJsonKeys.cladJsonKeyNamespaces] = namespaceList + ["Bus"]
        busesFileJson[CladJsonKeys.cladJsonKeyEnum] = self.__generateAudioEnumCladJsonData(self.__wwiseIdsJsonData["buses"], "BusType")
        outFile[CladJsonKeys.cladJsonKeyFiles].append(busesFileJson)

        return outFile

    # Special case, we merge in metadata
    def __generateEventCladJsonData(self):
        # Make enum for all the groups
        enums = []
        enumGroups = {}
        enumGroups[CladJsonKeys.cladJsonKeyType] = "uint_32"
        enumGroups[CladJsonKeys.cladJsonKeyTitle] = "EventGroupType"
        enumGroups[CladJsonKeys.cladJsonKeyMembers] = []
        enums.append(enumGroups)

        for aKey, aGroup in self.__eventEnumData.iteritems():
            enumGroup = {}
            enumGroup[CladJsonKeys.cladJsonKeyType] = "uint_32"
            enumGroup[CladJsonKeys.cladJsonKeyTitle] = aKey
            enumGroup[CladJsonKeys.cladJsonKeyMembers] = aGroup
            enums.append(enumGroup)

            groupMember = {}
            groupMember[CladJsonKeys.cladJsonKeyMemberName] = aKey
            enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

        return enums



    def __generateAudioStateEnumCladJsonData(self, stateGroup, genericTypeName = None):

        enums = []

        for aGroupKey, aGroupStates in stateGroup.iteritems():
            # Make enum for all the groups
            enumGroups = {}
            enumGroups[CladJsonKeys.cladJsonKeyType] = "uint_32"
            enumGroups[CladJsonKeys.cladJsonKeyTitle] = aGroupKey
            enumGroups[CladJsonKeys.cladJsonKeyMembers] = []
            enums.append(enumGroups)

            # Add invalid state
            groupMember = {}
            groupMember[CladJsonKeys.cladJsonKeyMemberName] = GenerateCladJsonFromMetadata.__invalidStateName
            groupMember[CladJsonKeys.cladJsonKeyMemberValue] = GenerateCladJsonFromMetadata.__invalidStateValue
            enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

            # Iterate through group states
            for aStateKey, aStateVal in aGroupStates.iteritems():
                groupMember = {}
                groupMember[CladJsonKeys.cladJsonKeyMemberName] = aStateKey.title()
                groupMember[CladJsonKeys.cladJsonKeyMemberValue] = aStateVal
                enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

        # Add Generic Type
        if genericTypeName != None:
            enumGroups = {}
            enumGroups[CladJsonKeys.cladJsonKeyType] = "uint_32"
            enumGroups[CladJsonKeys.cladJsonKeyTitle] = genericTypeName
            enumGroups[CladJsonKeys.cladJsonKeyMembers] = []
            enums.append(enumGroups)

            # Add invalid state
            groupMember = {}
            groupMember[CladJsonKeys.cladJsonKeyMemberName] = GenerateCladJsonFromMetadata.__invalidStateName
            groupMember[CladJsonKeys.cladJsonKeyMemberValue] = GenerateCladJsonFromMetadata.__invalidStateValue
            enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

        return enums


    def __generateAudioEnumCladJsonData(self, enumPairs, enumTitle):
        # Sort By Key
        keys = enumPairs.keys()
        keys.sort()

        enums = []
        enumGroups = {}
        enumGroups[CladJsonKeys.cladJsonKeyType] = "uint_32"
        enumGroups[CladJsonKeys.cladJsonKeyTitle] = enumTitle
        enumGroups[CladJsonKeys.cladJsonKeyMembers] = []
        enums.append(enumGroups)

        # Add invalid state
        groupMember = {}
        groupMember[CladJsonKeys.cladJsonKeyMemberName] = GenerateCladJsonFromMetadata.__invalidStateName
        groupMember[CladJsonKeys.cladJsonKeyMemberValue] = GenerateCladJsonFromMetadata.__invalidStateValue
        enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

        for aKey in keys:
            aValue = enumPairs[aKey]
            groupMember = {}
            groupMember[CladJsonKeys.cladJsonKeyMemberName] = aKey.title()
            groupMember[CladJsonKeys.cladJsonKeyMemberValue] = aValue
            enumGroups[CladJsonKeys.cladJsonKeyMembers].append(groupMember)

        return  enums



    def __importEvent(self, anItem, eventGroupsEnums):

        # Event type enums
        rootEventMember = {}
        rootEventMember[CladJsonKeys.cladJsonKeyMemberName] = anItem.wwiseIdKey
        rootEventMember[CladJsonKeys.cladJsonKeyMemberValue] = anItem.wwiseIdValue
        if len(anItem.comment) > 0:
            rootEventMember[CladJsonKeys.cladJsonKeyMemberComment] = anItem.comment

        eventGroupsEnums.append(rootEventMember)

        # Enum group
        if len(anItem.groupName) > 0:
            groupMemeber = {}
            groupMemeber[CladJsonKeys.cladJsonKeyMemberName] = anItem.aliasName
            groupMemeber[CladJsonKeys.cladJsonKeyMemberValue] = anItem.wwiseIdValue
            if len(anItem.comment) > 0:
                groupMemeber[CladJsonKeys.cladJsonKeyMemberComment] = anItem.comment

            # Add group to evnet enums
            if anItem.groupName not in self.__eventEnumData:
                self.__eventEnumData[anItem.groupName] = []
                self.__eventEnumData[anItem.groupName].append(self.__invalidGroupMember)

            return  groupMemeber
