#!/usr/bin/env python3
"""
Python command line interface for talking to SDK engineInterface, using cmd
"""
__author__ = "Mark Wesley"

import sys, os, time    
import cmd
import argparse

import engineInterface
from engineInterface import GToEM # to get message definitions too
from engineInterface import EngineCommand # for enum
from verboseLevel import VerboseLevel

kCliCmdNames = [
    "list",
]


def textStringToByteArrayString(inString, byteArrayStringLength):
    """ Converts a string into a string representation of a fixed-length ASCII-encoded byte array 
        This byte array could then be, for example, cast directly back to a c-string on the C++ side"""

    lenString = len(inString)

    if lenString > byteArrayStringLength:
        sys.stderr.write("[textStringToByteArrayString] inString '" + str(inString) + "' length = " + str(lenString) + " > " + str(byteArrayStringLength) + os.linesep)
        return ""

    outString = "("
    for inChar in inString:
        outString += str(ord(inChar)) + ","

    # pad to byte array length
    for i in range(byteArrayStringLength - lenString):
        outString += "0,"

    # Remove the trailing comma and add closing bracket
    outString = outString[:-1] + ")"

    return outString
    

class EngineRemoteCmd(cmd.Cmd):

    prompt = "CozmoSDK> "
    
        
    def emptyline(self):
        pass # overriden to prevent an empty line entered repeating last command, and instead doing nothing
      
    # ================================================================================  
    # send: Send a CLAD message
        
    def do_send(self, line):
        engineInterface.QueueCommand( (EngineCommand.sendMsg, line.split()) )
        
    def help_send(self):
        self.stdout.write("send MsgName ..MsgArgs... ['send list' to list all messages]" + os.linesep)
        
    def complete_send(self, text, line, start_index, end_index):
        suggestions = []
        
        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)
                    
        for cmd in GToEM._tags_by_name:
            if cmd.startswith(text):
                suggestions.append(cmd)
                
        return suggestions
        
    # ================================================================================  
    # cv: Set a Console Var  

    def do_cv(self, line):
        engineInterface.QueueCommand( (EngineCommand.consoleVar, line.split()) )
        
    def help_cv(self):
        self.stdout.write("cv VariableName ...Args... ['cv list' to list all variables]" + os.linesep)

    def complete_cv(self, text, line, start_index, end_index):
    
        suggestions = []
    
        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)
            
        suggestions += engineInterface.SyncCommand( (EngineCommand.consoleVar, ["get_matching_names", text]) )

        return suggestions

    # ================================================================================  
    # cf: Call a Console Function  

    def do_cf(self, line):
        engineInterface.QueueCommand( (EngineCommand.consoleFunc, line.split()) )
        
    def help_cf(self):
        # [MARKW:TODO] - extend help to pass in args, so we can do e.g. "help cf SomeFunc"
        self.stdout.write("cf FunctionName ...Args... ['cf list' to list all functions]" + os.linesep)

    def complete_cf(self, text, line, start_index, end_index):
    
        suggestions = []
    
        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)
                
        suggestions += engineInterface.SyncCommand( (EngineCommand.consoleFunc, ["get_matching_names", text]) )

        return suggestions
        
    # ================================================================================  
    # Mood  

    def do_setEmotion(self, line):
        "setEmotion emotionName newValue ['setEmotion list' to list all emotions]" 
        engineInterface.QueueCommand( (EngineCommand.setEmotion, line.split()) )

    def complete_setEmotion(self, text, line, start_index, end_index):
    
        suggestions = []

        emotionNames = engineInterface.gEngineInterfaceInstance.moodManager.emotionNames
    
        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)
                
        for emotionName in emotionNames:
            if emotionName.startswith(text):
                suggestions.append(emotionName)

        return suggestions

    # ================================================================================
    # Animations

    def do_getAnimations(self, line):
        "getAnimations - requests a list of available animations from the engine"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["RequestAvailableAnimations"]) )

    def do_playAnimation(self,line):
        "playAnimation animationName optional_numLoops ['playAnimation list' to list all Animations currently stored]"
        engineInterface.QueueCommand( (EngineCommand.playAnimation, line.split()) )

    def complete_playAnimation(self, text, line, start_index, end_index):
        suggestions = []
        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)

        suggestions += engineInterface.SyncCommand( (EngineCommand.playAnimation, ["get_matching_names", text]) )

        return suggestions

    def do_getAnimationGroups(self, line):
        "getAnimationGroups - requests a list of available animation groups from the engine"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["RequestAvailableAnimationGroups"]) )

    def do_playAnimationGroup(self, line):
        "playAnimationGroup animationGroupName optional_numLoops ['playAnimationGroup list' to list all Animation Groups currently stored]"
        engineInterface.QueueCommand( (EngineCommand.playAnimationGroup, line.split()) )

    def complete_playAnimationGroup(self, text, line, start_index, end_index):
        suggestions = []

        for cliCmdName in kCliCmdNames:
            if cliCmdName.startswith(text):
                suggestions.append(cliCmdName)

        suggestions += engineInterface.SyncCommand( (EngineCommand.playAnimationGroup, ["get_matching_names", text]) )

        return suggestions

    # ================================================================================  
    # Start connect etc.  

    def do_startsim(self, line):
        "start sim - starts the engine and connects to a sim robot" 
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["StartEngine"]) )
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["ConnectToRobot", textStringToByteArrayString("127.0.0.1", 16), "1", "1"]) )

    def do_startreal(self, line):
        "start real - starts the engine and connects to a real robot" 
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["StartEngine"]) )
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["ConnectToRobot", textStringToByteArrayString("172.31.1.1", 16), "1", "0"]) )

    def do_requestDataDrivenLists(self, line):
        "requestDataDrivenLists - requests lists of anything non-CLAD data-driven e.g. all console vars, all animations"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["GetAllDebugConsoleVarMessage"]) )  # get all the Console Var/Function entries

    def do_setVerboseLevel(self, line):
        "setVerboseLevel - sets the verbosity level at runtime (0 = Low, 1 = High, 2 = Max)"
        engineInterface.QueueCommand( (EngineCommand.setVerboseLevel, line.split()) )

    def do_resetConnection(self, line):
        "resetConnection - reset the connection - e.g. if underlying engine has restarted and you want to reconnect"
        engineInterface.QueueCommand( (EngineCommand.resetConnection, line.split()) )
        
    # ================================================================================  
    # Common Actions

    def do_SearchForCube(self,line):
        "Search for a light cube by spinning around (optionally accepts a specific objectId to look for)"
        inputSplit = line.split()
        objectId = 0
        matchAnyObjectId = True

        if (len(inputSplit) >= 1):
            objectId = int(inputSplit[0])
            matchAnyObjectId = False
        
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["SearchForObject", "Anki.Cozmo.ObjectFamily.LightCube", str(objectId), str(matchAnyObjectId)]) )

    def do_Turn(self,line):
        "Turn (90 degrees by default, accepts anyup to 4 args for [angleToTurn [radsPerSec [accRPS2 [isAbsolute]]]]"
        inputSplit = line.split()
        angleToTurn = inputSplit[0] if (len(inputSplit) >= 1) else "1.57"
        radsPerSec  = inputSplit[1] if (len(inputSplit) >= 2) else "1.0"
        accRPS2     = inputSplit[2] if (len(inputSplit) >= 3) else "1.0"
        isAbsolute  = inputSplit[3] if (len(inputSplit) >= 4) else "0"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["TurnInPlace", angleToTurn, radsPerSec, accRPS2, isAbsolute, "1" ]) )

    def do_Drive(self,line):
        "Drive (forwards by default, accepts anyup to 4 args for [lWheelSpeed [rWheelSpeed [lWheelAcc [rWheeelAcc]]]]"
        inputSplit = line.split()
        lWheelSpeed = inputSplit[0] if (len(inputSplit) >= 1) else "20.0"
        rWheelSpeed = inputSplit[1] if (len(inputSplit) >= 2) else "20.0"
        lWheelAcc   = inputSplit[2] if (len(inputSplit) >= 3) else "20.0"
        rWheelAcc   = inputSplit[3] if (len(inputSplit) >= 4) else "20.0"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["DriveWheels", lWheelSpeed, rWheelSpeed, lWheelAcc, rWheelAcc ]) )

    def do_StopCozmo(self,line):
        "Stop Cozmo from driving or moving their head"
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["DriveWheels", "0", "0", "0", "0"]) ) 
        engineInterface.QueueCommand( (EngineCommand.sendMsg, ["MoveHead", "0.0"]) ) 
        
    # ================================================================================  
    # quit / exit / etc
    
    def do_quit(self, line):
        "Quit / Exit the command line app"
        sys.exit(1)
      
    do_exit = do_quit
    
    
class EngineRemoteCLI:

    def __init__(self):

        parser = argparse.ArgumentParser()
        parser.add_argument("-udp", dest='udp', action='store_true', default=False, help="Connect over UDP (e.g. for Webots) vs default TCP connection to iOS device")
        parser.add_argument("-v", "--verbosity", dest='verboseLevel', action="count", help="Verbose logging level ('-v' = Med, '-vv' =Max)")
    
        args = parser.parse_args()
        verboseLevel = int(args.verboseLevel) if (args.verboseLevel != None) else 0
        useTcp = not args.udp

        if verboseLevel >= VerboseLevel.Max:
            sys.stdout.write("[CLI] Args = '" + str(args) + "'" + os.linesep)
            sys.stdout.write("[CLI] verboseLevel = '" + str(verboseLevel) + "'" + os.linesep)
            sys.stdout.write("[CLI] useTcp = '" + str(useTcp) + "'" + os.linesep)

        engineInterface.Init(True, useTcp, verboseLevel)
        self.keepRunning = True
        self.run()
        
    def __del__(self):
        engineInterface.Shutdown()        

    def run(self):
        
        engineRemoteCmd = EngineRemoteCmd()
        
        while self.keepRunning:
            try:                                    
                engineRemoteCmd.cmdloop()                
            except Exception as e:
                self.keepRunning = False
                sys.stderr.write(str(e) + os.linesep)
                
        engineInterface.Shutdown()
        
        
if __name__ == '__main__':
    EngineRemoteCLI()


