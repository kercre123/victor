import json

jsonKeyEvents = "events"
jsonKeyStates = "states"
jsonKeySwitches = "switches"
jsonKeyGameParameters = "gameParameters"
jsonKeyBanks = "banks"
jsonKeyBuses = "buses"


class ParsingStates:
    Header = 0
    AkNamespace = 1
    Events = 2
    States = 3
    Switches = 4
    GameParameters = 5
    Banks = 6
    Buses = 7


class ParseWWiseIds:
    # Vars
    __parsingState = ParsingStates.Header
    __events = {}
    __states = {}
    __switches = {}
    __gameParameters = {}
    __banks = {}
    __buses = {}

    __currentGroupName = None
    __hasCurrentGroupId = False
    # __inCurrentGroupState = False


    __states["StateGroupType"] = {}
    __switches["SwitchGroupType"] = {}

    # Parse File
    def parseWWiseIdFile(self, inFileName):
        # Vars
        lines = tuple(open(inFileName, 'r'))

        # loop through lines to parse
        for aLine in lines:

            # print "Current state: %s" % self.parsingState
            # print aLine

            words = aLine.split()

            newState = None

            # Skip over header
            if self.__parsingState is ParsingStates.Header:
                newState = self.__phaseHeader(words)

            # Determine what type of Ids to parse
            elif self.__parsingState is ParsingStates.AkNamespace:
                newState = self.__phaseAkNamespace(words)

            # Handle Event phase
            elif self.__parsingState is ParsingStates.Events:
                newState = self.__phaseEvents(words)

            # Handle States phase
            elif self.__parsingState is ParsingStates.States:
                newState = self.__phaseStates(words)

            # Handle Switches phase
            elif self.__parsingState is ParsingStates.Switches:
                newState = self.__phaseSwitches(words)

            # Handle GameParameters phase
            elif self.__parsingState is ParsingStates.GameParameters:
                newState = self.__phaseParameters(words)

            # Handle GameParameters phase
            elif self.__parsingState is ParsingStates.Banks:
                newState = self.__phaseBanks(words)

            # Handle GameParameters phase
            elif self.__parsingState is ParsingStates.Buses:
                newState = self.__phaseBuses(words)

            # Update current state
            if newState != None:
                self.__parsingState = newState

        # Add Json data to container
        jsonData = {}
        if len(self.__events) > 0:
            jsonData[jsonKeyEvents] = self.__events
        if len(self.__states) > 0:
            jsonData[jsonKeyStates] = self.__states
        if len(self.__switches) > 0 :
            jsonData[jsonKeySwitches] = self.__switches
        if len(self.__gameParameters) > 0 :
            jsonData[jsonKeyGameParameters] = self.__gameParameters
        if len(self.__banks) > 0 :
            jsonData[jsonKeyBanks] = self.__banks
        if len(self.__buses) > 0 :
            jsonData[jsonKeyBuses] = self.__buses

        return  jsonData

    # Write File
    def createWWiseIdJsonFile(self, jsonData, outFileName):
        with open(outFileName, 'w') as outfile:
            json.dump(jsonData, outfile)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Parsing methods
    def __parseWwiseIdLine(self, words):
        if len(words) == 6:
            key = words[3]
            value = words[5][:-2]
            # print "key: %s value: %s" % (key, value)
            return (key, value)

    def __parseNamespaceLine(self, words):
        if len(words) == 2 and words[0] == "namespace":
            namespace = words[1]
            # print "Found Namespace: %s" % namespace
            return namespace

    def __parseCloseingBracket(self, words):
        return len(words) > 0 and words[0] == "}"

    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Parsing phases
    def __phaseHeader(self, words):
        # print "State - Header"
        # Looking for AK namespace
        namespaceName = self.__parseNamespaceLine(words)

        # print "pares namespace: %s" % namespaceName

        if namespaceName == "AK":
            # print "Updated State"
            return ParsingStates.AkNamespace  # Update state

    def __phaseAkNamespace(self, words):
        # print "State - AkNamespace"
        # Looking for namespace
        namespaceName = self.__parseNamespaceLine(words)

        if namespaceName == "EVENTS":
            return ParsingStates.Events

        elif namespaceName == "STATES":
            return ParsingStates.States

        elif namespaceName == "SWITCHES":
            return ParsingStates.Switches

        elif namespaceName == "GAME_PARAMETERS":
            return ParsingStates.GameParameters

        elif namespaceName == "BANKS":
            return ParsingStates.Banks

        elif namespaceName == "BUSSES":     # Wwise_IDs.h file uses different spelling of "busses"
            return ParsingStates.Buses

    def __phaseEvents(self, words):
        # print "State - Events"

        result = self.__parseWwiseIdLine(words)
        if result is not None:
            key = result[0].title()
            value = result[1]
            self.__events[key] = int(value)
        elif self.__parseCloseingBracket(words):
            # Exit state
            return ParsingStates.AkNamespace

    def __phaseStates(self, words):
        # print "State - States"

        if self.__currentGroupName == None:
            # Not in group look for Namespace
            groupName = self.__parseNamespaceLine(words)
            if groupName != None:
                self.__currentGroupName = groupName.title()
                self.__states[self.__currentGroupName] = {}
            elif self.__parseCloseingBracket(words):
                # End of Section
                return ParsingStates.AkNamespace

        elif self.__parseCloseingBracket(words):
            if self.__hasCurrentGroupId:
                self.__hasCurrentGroupId = False
            elif self.__currentGroupName != None:
                self.__currentGroupName = None

        else:
            # In a group
            if self.__hasCurrentGroupId == False:
                # Pre & Post state data
                # Pre looking for group Id value
                result = self.__parseWwiseIdLine(words)
                if result != None:
                    value = result[1]
                    self.__states["StateGroupType"][self.__currentGroupName] = int(value)
                    self.__hasCurrentGroupId = True

                elif self.__parseCloseingBracket(words):
                    # Post
                    # This is the end of that state group
                    self.__currentGroupName = None
                    self.__hasCurrentGroupId = False

            else:
                # Parsing group states
                result = self.__parseWwiseIdLine(words)
                if result != None:
                    # Store State Value
                    key = result[0]
                    value = result[1]
                    self.__states[self.__currentGroupName][key] = int(value)



    def __phaseSwitches(self, words):
        # print "State - Switches"
        if self.__currentGroupName == None:
            # Not in group look for Namespace
            groupName = self.__parseNamespaceLine(words)
            if groupName != None:
                self.__currentGroupName = groupName.title()
                self.__switches[self.__currentGroupName] = {}
            elif self.__parseCloseingBracket(words):
                # End of Section
                return ParsingStates.AkNamespace

        elif self.__parseCloseingBracket(words):
            if self.__hasCurrentGroupId:
                self.__hasCurrentGroupId = False
            elif self.__currentGroupName != None:
                self.__currentGroupName = None

        else:
            # In a group
            if self.__hasCurrentGroupId == False:
                # Pre & Post state data
                # Pre looking for group Id value
                result = self.__parseWwiseIdLine(words)
                if result != None:
                    value = result[1]
                    self.__switches["SwitchGroupType"][self.__currentGroupName] = int(value)
                    self.__hasCurrentGroupId = True

                elif self.__parseCloseingBracket(words):
                    # Post
                    # This is the end of that state group
                    self.__currentGroupName = None
                    self.__hasCurrentGroupId = False

            else:
                # Parsing group states
                result = self.__parseWwiseIdLine(words)
                if result != None:
                    # Store State Value
                    key = result[0]
                    value = result[1]
                    self.__switches[self.__currentGroupName][key] = int(value)


    def __phaseParameters(self, words):
        # print "State - Parameters"

        result = self.__parseWwiseIdLine(words)
        if result != None:
            key = result[0]
            value = result[1]
            self.__gameParameters[key] = int(value)
        elif self.__parseCloseingBracket(words):
            # Exit state
            return ParsingStates.AkNamespace

    def __phaseBanks(self, words):
        # print "State - Banks"
        result = self.__parseWwiseIdLine(words)
        if result != None:
            key = result[0]
            value = result[1]
            self.__banks[key] = int(value)
        elif self.__parseCloseingBracket(words):
            # Exit state
            return ParsingStates.AkNamespace

    def __phaseBuses(self, words):
        # print "State - Buses"
        result = self.__parseWwiseIdLine(words)
        if result != None:
            key = result[0]
            value = result[1]
            self.__buses[key] = int(value)
        elif self.__parseCloseingBracket(words):
            # Exit state
            return ParsingStates.AkNamespace
