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
    
    def __init__(self, topics=[], periodic_topics={}):
        for t in topics:
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
    parser.add_argument('-a', '--all', action='store_true', help="Print all messages")
    parser.add_argument('--no-sync',   action='store_true', help="Do not send sync time message to robot on startup.")
    parser.add_argument('--sync-time', default=0, type=int, help="Manually specify sync time offset")
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    parser.add_argument('tags', nargs='*', help="The tags to subscribe to")
    args = parser.parse_args()

    robotInterface.Init()
    robotInterface.Connect(dest=(args.ip_address, args.port), syncTime = None if args.no_sync else args.sync_time)

    if args.all:
        args.tags = robotInterface.RI.RobotToEngine._tags_by_name.values()
    
    TopicPrinter(args.tags)
    try:
        while True:
            sys.stdout.flush()
            time.sleep(0.03)
    except KeyboardInterrupt:
        robotInterface.Disconnect()
        sys.exit()
