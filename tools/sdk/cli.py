#!/usr/bin/env python3
"""
Python command line interface for Engine, using cmd
"""
__author__ = "Mark Wesley"

import sys, os, time    
import cmd

sys.path.insert(0, os.path.join("tools"))
import engineInterface
from engineInterface import GToEM # to get message definitions too
from engineInterface import EngineCommand # for enum

kCliCmdNames = [
    "list",
]

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
    # quit / exit / etc
    
    def do_quit(self, line):
        "Quit / Exit the command line app"
        sys.exit(1)
      
    do_exit = do_quit
    
    
class EngineRemoteCLI:

    def __init__(self):
        engineInterface.Init(True)
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


