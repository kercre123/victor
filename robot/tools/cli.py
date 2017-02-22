#!/usr/bin/env python3
"""
Python command line interface for interacting with the robot.
New version that uses robotInterface class.
"""
__author__ = "Daniel Casner <daniel@anki.com>"
__version__ = "0.2.0"

import sys, os, time, argparse
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from :/robot ?")
else:
    Anki = robotInterface.Anki
    RI = robotInterface.RI
    import faceImage
    import animationStreamer

class CLI:
    "Robot command line interface"

    def __init__(self):
        self.anim = None

    def listMessages(self, starts_with="", ends_with=""):
        """Lists all engine to robot messages
        """
        msgs = [m for m in RI.EngineToRobot._tags_by_name.keys() if m.startswith(starts_with) and m.endswith(ends_with)]
        msgs.sort()
        sys.stdout.write(os.linesep.join(msgs))
        sys.stdout.write(os.linesep)

    def makeMessage(self, tagName, *args):
        "Returns an EngineToRobot message generated from the given tagName and args"
        cmdType = RI.EngineToRobot.typeByTag(RI.EngineToRobot._tags_by_name[tagName])
        cmd = cmdType(*[eval(a) for a in args])
        return RI.EngineToRobot(**{tagName: cmd})

    def showImage(self, png, invert=False):
        """Display image on robot's face for 30 seconds
        png    File to load
        invert Invert the image, default False
        """
        img = faceImage.loadImage(png, invert)
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animStartOfAnimation=Anki.Cozmo.AnimKeyFrame.StartOfAnimation()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animFaceImage=img))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animEndOfAnimation=Anki.Cozmo.AnimKeyFrame.EndOfAnimation()))

    def tone(self, frequency=440):
        "Play tone of specified frequency"
        if self.anim is None:
            self.anim = animationStreamer.ToneStreamer()
        if not self.anim.haveStarted:
            self.anim.start(int(frequency))
        else:
            self.anim.stop()

    def showHelp(self, cmd=None, oneLine=False):
        """Show CLI basic commands
        do `help <MESSAGE>` to get get details on a specific message
        """
        if cmd is None:
            for i in self.replCommands.items():
                sys.stdout.write("{0:s}: {1.__doc__}{ls}".format(*i, ls=os.linesep))
        elif cmd in RI.EngineToRobot._tags_by_name:
            outgoingMessageType = RI.EngineToRobot
            msgTag = getattr(outgoingMessageType.Tag, cmd)
            msgType = outgoingMessageType.typeByTag(msgTag)
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
        else:
            sys.stdout.write("No help available for {}{}".format(cmd, os.linesep))

    def exitRepl(self):
        """Exit CLI REPL
        """
        raise StopIteration

    def execCommand(self, command, *args, **kwargs):
        "Execute a command to the robot"
        if command in RI.EngineToRobot._tags_by_name:
            return robotInterface.Send(self.makeMessage(command, *args))
        elif command in self.replCommands:
            return self.replCommands[command](self, *args, **kwargs)
        else:
            raise ValueError("Unknown command " + command)

    def repl(self):
        "Read eval print loop for interacting with the robot"
        while True:
            try:
                cmd = input("COZMO>>> ").split(' ')
            except EOFError:
                break
            except KeyboardInterrupt:
                break
            try:
                self.execCommand(*cmd)
            except StopIteration:
                break
            except Exception as e:
                sys.stdout.write(str(e))
                sys.stdout.write(os.linesep)

    replCommands = {
        'showImage': showImage,
        'tone': tone,
        'list': listMessages,
        'help': showHelp,
        'exit': exitRepl,
        '': lambda self: None,
    }

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="topic",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('command', nargs='*', help="Send single command to the robot and then exit. First arg is command, following are arguments to initalize the message.")
    parser.add_argument('-w', '--wait', type=int, default=5, help="Number of seconds to wait before exiting when sending single command")
    parser.add_argument('-l', '--list', action='store_true', help="List the commands which can be sent to the robot.")
    parser.add_argument('--dry-run', action='store_true', help="Launch the RELP but don't connect to the robot")
    parser.add_argument('--no-sync',   action='store_true', help="Do not send sync time message to robot on startup.")
    parser.add_argument('--sync-time', default=0, type=int, help="Manually specify sync time offset")
    parser.add_argument('-b', '--no-blinkers', action='store_true', help="Do not turn on backpack light test pattern")
    parser.add_argument('-v', '--image-request', action='store_true', help="Request video from the robot")
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    args = parser.parse_args()
    
    cli = CLI()
    
    if args.list:
        cli.listMessages()
        sys.exit()
    
    robotInterface.Init()
    if not args.dry_run:
        robotInterface.Connect(dest=(args.ip_address, args.port),
                               syncTime = None if args.no_sync else args.sync_time,
                               imageRequest = args.image_request,
                               blinkers = not args.no_blinkers)
    
    if args.command:
        while not robotInterface.GetConnected():
            time.sleep(0.030)
        cli.execCommand(*args.command)
        time.sleep(args.wait)
    else:
        cli.repl()
    
    if not args.dry_run:
        robotInterface.Die()
