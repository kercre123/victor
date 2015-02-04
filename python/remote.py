"""
Python based command line remote control for cozmo.
Currently based on Cozmo 3.x architecture
"""
__author__ = "Daniel Casner"
__version__ = "3.0"

import sys, os, socket

try:
    sys.path.append(os.path.join('..', 'robot', 'som'))
    import messages
except Exception, e:
    sys.stderr.write("""%s
%s

Couldn't import python message types :-(
This program expects to be run as
    python %s
from <COZMO_SOURCE_DIRECTORY>/python

""" % (str(e), str(sys.path), sys.argv[0]))
    sys.exit(1)

class Remote(socket.socket):

    def __init__(self, hostname='172.31.1.1', port=5551):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.addr = (hostname, port)

    def wheelCmd(self, left, right):
        "Send a drive wheel command message to the robot"
        self.sendto(messages.DriveWheelsMessage(left, right).serialize(), self.addr)



if __name__ == '__main__':
    r = Remote()
    while True:
        try:
            sys.stdout.write("Cozmo>>> ")
            sys.stdout.flush()
            cmd = raw_input()
        except EOFError, e:
            break
        else:
            if not len(cmd):
                continue
            if cmd[0] in 'lrb':
                try:
                    spd = float(cmd[1:])
                except:
                    sys.stdout.write('Speed must be a float\n')
                    continue
                else:
                    if cmd[0] == 'l':
                        spds = (spd, 0.0)
                    elif cmd[0] == 'r':
                        spds = (0.0, spd)
                    else:
                        spds = (spd, spd)
                    sys.stdout.write("Sending wheel command %f, %f\n" % spds)
                    r.wheelCmd(*spds)
