"""
DebugConsole support for ConsoleVars and ConsoleFunctions
"""
__author__ = "Mark Wesley"

import sys
import os
    
    
# ================================================================================    
# DebugConsoleVarWrapper
# ================================================================================


class DebugConsoleVarWrapper:
    "Wraps an Anki.Cozmo.ExternalInterface.DebugConsoleVar and supports sorting"

    def __init__(self, debugConsoleVar):
        self.var = debugConsoleVar
        
    @property
    def varName(self):
        return self.var.varName
        
    @property
    def category(self):
        return self.var.category
        
    @property
    def minValue(self):
        return self.var.minValue
        
    @property
    def maxValue(self):
        return self.var.maxValue
        
    @property
    def varValue(self):
        return self.var.varValue
        
    @varValue.setter
    def varValue(self, value):
        self.var.varValue = value
        
    def formatValueToString(self, inVal):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return str(inVal)
        elif varValue.tag == varValue.Tag.varUint:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varInt:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varBool:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varFunction:
            return str(inVal)
        else:
            return "ERROR"
        
    def minValueToString(self):
        return self.formatValueToString(self.minValue)
        
    def maxValueToString(self):
        return self.formatValueToString(self.maxValue)

    def varValueTypeName(self):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return "double"
        elif varValue.tag == varValue.Tag.varUint:
            return "uint32_t"
        elif varValue.tag == varValue.Tag.varInt:
            return "int"
        elif varValue.tag == varValue.Tag.varBool:
            return "bool"
        elif varValue.tag == varValue.Tag.varFunction:
            return "function"
        else:
            return "ERROR"
            
    def varValueContents(self):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return varValue.varDouble
        elif varValue.tag == varValue.Tag.varUint:
            return varValue.varUint
        elif varValue.tag == varValue.Tag.varInt:
            return varValue.varInt
        elif varValue.tag == varValue.Tag.varBool:
            return varValue.varBool
        elif varValue.tag == varValue.Tag.varFunction:
            return varValue.varFunction
        else:
            return "ERROR"
                    
    def varValueToString(self):
        return self.formatValueToString(self.varValueContents())        
    
    def __lt__(self, rhs):
        if (self.category == rhs.category):
            return (self.varName < rhs.varName)
        else:
            return (self.category < rhs.category)


# ================================================================================    
# DebugConsoleVarManager
# ================================================================================


def GetMatchingNamesFromConsoleEntries(consoleEntryList, text):
    "Returns a list of console entries that start with text"
    matchingNames = []
    for consoleEntry in consoleEntryList:
        if consoleEntry.varName.startswith(text):
            matchingNames.append(consoleEntry.varName)
    return matchingNames


class DebugConsoleManager:
    "Console Var Manager / Container"
    
    
    def __init__(self):
            
        self.consoleVars = {}
        self.consoleFuncs = {}
        
    def GetVar(self, varName):
        return self.consoleVars.get(varName)
            
    def GetFunc(self, funcName):
        return self.consoleFuncs.get(funcName)
                
    def HandleInitDebugConsoleVarMessage(self, msg):
    
        for cVar in msg.varData:
            if cVar.varValue.tag == cVar.varValue.Tag.varFunction:
                self.consoleFuncs[cVar.varName] = DebugConsoleVarWrapper(cVar) 
            else:
                self.consoleVars[cVar.varName] = DebugConsoleVarWrapper(cVar)
    
    def GetMatchingVarNames(self, text):
        return GetMatchingNamesFromConsoleEntries(self.consoleVars.values(), text)

    def GetMatchingFuncNames(self, text):
        return GetMatchingNamesFromConsoleEntries(self.consoleFuncs.values(), text)
    
    def ListConsoleVars(self):
    
        sys.stdout.write("List of Console Variables:" + os.linesep)
        
        # 1st sort the vars so we can display them in category and name order
        sortedVars = sorted(self.consoleVars.values())
        
        currentCat = ""
        
        for varValue in sortedVars:
            varName = varValue.varName
            if varValue.category != currentCat:
                currentCat = varValue.category
                sys.stdout.write("  " + currentCat + ":" + os.linesep)
            sys.stdout.write("    " + str(varName) + " = " + varValue.varValueToString() + " (" + varValue.varValueTypeName() + ")(" + varValue.minValueToString() + ".." + varValue.maxValueToString() + ")" + os.linesep)
            
        return True

            
    def ListConsoleFuncs(self):
    
        sys.stdout.write("List of Console Functions:" + os.linesep)
        
        # 1st sort the funcs so we can display them in category and name order
        sortedFuncs = sorted(self.consoleFuncs.values())
        
        currentCat = ""
        
        for funcValue in sortedFuncs:
            funcName = funcValue.varName
            if funcValue.category != currentCat:
                currentCat = funcValue.category
                sys.stdout.write("  " + currentCat + ":" + os.linesep)
            sys.stdout.write("    " + str(funcName) + " : Args = '" + funcValue.varValueToString() + "'" + os.linesep)
            
        return True

