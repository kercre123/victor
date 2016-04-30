#!/usr/bin/env python3
"""
Python command line interface for Engine, using threadedInput
"""
__author__ = "Mark Wesley"

import sys, os, time
from threadedInput import ThreadedInput    

sys.path.insert(0, os.path.join("tools"))
import engineInterface
from engineInterface import GToEM # to get message definitions too
    
class EngineRemoteCLI:

    def __init__(self):
        engineInterface.Init()
        self.threadedInput = ThreadedInput("CozmoSDK> ")
        self.hasStartedInput = False
        self.keepRunning = True
        self.run()        
        
    def __del__(self):
        engineInterface.Shutdown()
        
    def HandleExitCommand(self, *args):
        self.keepRunning = False
        return False
        
    def HandleHelpSpecificCommand(self, cmd, oneLine = False): 
        if not hasattr(GToEM, cmd):
            sys.stdout.write("No message of type '{}'{}".format(cmd, os.linesep))
            return False

        msgTag = getattr(GToEM.Tag, cmd)
        msgType = GToEM.typeByTag(msgTag)

        # grab the arguments passed into the init function, excluding "self"
        try:
            args = msgType.__init__.__code__.co_varnames[1:]
        except AttributeError:
            # in case it's a primitive type like 'int'
            #TODO: once we have enum to string, use that here and list possible values
            args = [msgType.__name__]

        defaults = []
        try:
            defaults = msgType.__init__.__defaults__
        except AttributeError:
            pass

        if not oneLine:
            print("{}: {} command".format(cmd, msgType.__name__))
            print("usage: {} {}".format(cmd, ' '.join(args)))
            if len(args) > 0:
                print()

            for argnum in range(len(args)):
                arg = args[argnum]
                if len(defaults) > argnum:
                    defaultStr = "default = {}".format(defaults[argnum])
                else:
                    defaultStr = ""

                typeStr = ""
                try:
                    doc = getattr(msgType, arg).__doc__
                except AttributeError:
                    pass
                else:
                    if doc:
                        docSplit = doc.split()
                        if len(docSplit) > 0:
                            typeStr = docSplit[0]

                    print("    {}: {} {}".format(arg, typeStr, defaultStr))
        else:
            print ("  {} {}".format(cmd, ' '.join(args)))

        return True
        
    def HandleHelpCommand(self, *args):
        "Prints out help text on CLI functions"
        if len(args) == 0:
            print("type 'help command' to get help for command")
            print("type 'list' to see a list of commands")
            print("type 'exit' to exit")
            return True

        return self.HandleHelpSpecificCommand(args[0])

    def HandleListCommand(self, *args):
        "Prints out help text on CLI functions"  
        print("Available CLAD message commands (use help for more details):")
        for cmd in GToEM._tags_by_name:
            self.HandleHelpSpecificCommand(cmd, oneLine = True)
        
    def HandlePlayAnimCommand(self, *args):
        engineInterface.gEngineInterfaceInstance.PlayAnim(*args)

    def HandleNextCommand(self):
    
        latestCommand = self.threadedInput.PopCommand()
        if latestCommand != None:
        
            commandName = latestCommand[0]
            commandNameLower = commandName.lower()
            commandArgs = latestCommand[1:]
                                
            if commandNameLower == 'help':
                return self.HandleHelpCommand(*commandArgs)
            elif commandNameLower == 'list':
                return self.HandleListCommand(*commandArgs)
            elif commandNameLower == 'exit':
                return self.HandleExitCommand(*commandArgs)
            elif commandNameLower == 'cv':
                return engineInterface.gEngineInterfaceInstance.HandleCVarCommand(*commandArgs)
            elif commandNameLower == 'cf':
                return engineInterface.gEngineInterfaceInstance.HandleCVarCommand(*commandArgs)
            elif commandNameLower == 'playanim':
                return self.HandlePlayAnimCommand(*commandArgs)
            elif not hasattr(GToEM, commandName):
                sys.stderr.write("Unrecognized command \"{}\", try \"help\"{}".format(commandName, os.linesep))            
                return False
            else:
                try:
                    params = [eval(a) for a in commandArgs]
                except Exception as e:
                    sys.stderr.write("Couldn't parse command arguments for '{0}':{2}\t{1}{2}".format(commandName, e, os.linesep))
                    return False
                else:
                    t = getattr(GToEM.Tag, commandName)
                    y = GToEM.typeByTag(t)                    
                    try:
                        p = y(*params)
                    except Exception as e:
                        sys.stderr.write("Couldn't create {0} message from params *{1}:{3}\t{2}{3}".format(commandName, repr(params), str(e), os.linesep))
                        return False
                    else:
                        m = GToEM(**{commandName: p})
                        sys.stdout.write("[EngineRemoteCLI] Sending: '" + str(m) + "'" + os.linesep)
                        return engineInterface.gEngineInterfaceInstance.sendToEngine(m)
            
    def run(self):
        "Update engine every loop and poll for commands to send"
        while self.keepRunning:
            try:
                engineInterface.Update()
                
                # don't start the CLI until connected successfully 
                if engineInterface.IsConnected() and not self.hasStartedInput:
                    self.threadedInput.start()
                    self.hasStartedInput = True    
               
                self.HandleNextCommand()
                
                time.sleep(0.1) # in seconds
                
            except Exception as e:
                self.keepRunning = False
                sys.stderr.write(str(e) + "\r\n")

        engineInterface.Shutdown()

if __name__ == '__main__':
    EngineRemoteCLI()


