#!/usr/bin/env python3
"""
Python command line tool for echoing messages coming from the Cozmo robot to the command line
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, argparse
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from <engine>/robot ?")

class TopicPrinter:
    "Class for receiving messages and printing them to stdout."

    def printMsg(self, msg):
        sys.stdout.write(repr(msg))
        sys.stdout.write(os.linesep)
        
    def parseState(self, msg):
        sys.stdout.write("\t".join(["{}: {}".format(key, getattr(msg, key)) for key in self.stateParse if hasattr(msg, key)]))
        sys.stdout.write(os.linesep)
        
    def imageDebug(self, img):
        sys.stdout.write("\tts={0.frameTimeStamp:08d}\tid={0.imageId:08d}\tdb={0.chunkDebug:08x}\tcc={0.imageChunkCount:02d}\tci={0.chunkId:02d}{1}".format(img, os.linesep))
    
    def __init__(self, args):
        if args.state_parse:
            self.stateParse = args.state_parse
            robotInterface.SubscribeToTag(robotInterface.RI.RobotToEngine.Tag.state, self.parseState)
        elif args.image_debug:
            robotInterface.SubscribeToTag(robotInterface.RI.RobotToEngine.Tag.image, self.imageDebug)
        else:
            for t in args.tags:
                if type(t) is int:
                    topic = t
                elif t in robotInterface.RI.RobotToEngine._tags_by_name:
                    topic = robotInterface.RI.RobotToEngine._tags_by_name[t]
                else:
                    sys.exit("Unkown tag \"{}\"".format(t))
                robotInterface.SubscribeToTag(topic, self.printMsg)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="topic",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-l', '--list', action='store_true', help="List available topics and exit")
    parser.add_argument('-a', '--all', action='store_true', help="Print all messages")
    parser.add_argument('--no-sync',   action='store_true', help="Do not send sync time message to robot on startup.")
    parser.add_argument('--sync-time', default=0, type=int, help="Manually specify sync time offset")
    parser.add_argument('-v', '--image-request', action='store_true', help="Request video from the robot")
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    parser.add_argument('--state_parse', nargs='*', help="Print just the specified fields from the robot state message")
    parser.add_argument('--image_debug', action='store_true', help="Print image chunk debugging summary")
    parser.add_argument('tags', nargs='*', help="The tags to subscribe to")
    args = parser.parse_args()

    if args.list:
        sys.stdout.write("Available topics:")
        sys.stdout.write(os.linesep)
        topics = list(robotInterface.RI.RobotToEngine._tags_by_name.keys())
        topics.sort()
        for n in topics:
            sys.stdout.write(n)
            sys.stdout.write(os.linesep)
        sys.exit()

    robotInterface.Init()
    robotInterface.Connect(dest=(args.ip_address, args.port),
                           syncTime = None if args.no_sync else args.sync_time,
                           imageRequest = args.image_request)

    if args.all:
        args.tags = robotInterface.RI.RobotToEngine._tags_by_name.values()
    
    TopicPrinter(args)
    try:
        while True:
            sys.stdout.flush()
            time.sleep(0.03)
    except KeyboardInterrupt:
        robotInterface.Die()
