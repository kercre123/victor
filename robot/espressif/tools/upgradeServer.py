#!/usr/bin/env python
"""A simple HTTP server + UDP command function to cause the Espressif to upgrade firmware wirelessly."""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, socket, struct, threading, time

if sys.version_info.major > 2:
    from http import server as httpServer
    from urllib.parse import urlparse
else:
    import SimpleHTTPServer as httpServer
    httpServer.HTTPServer = httpServer.BaseHTTPServer.HTTPServer
    httpServer.BaseHTTPRequestHandler = httpServer.BaseHTTPServer.BaseHTTPRequestHandler
    from urlparse import urlparse

COMMAND_SOCKET   = 8580
HTTP_SERVER_PORT = 8580

USAGE = """%s <WiFi IP address> <robot IP address> <upgrade command>\r\n""" % (sys.argv[0])

class UpgradeRequestHandler(httpServer.BaseHTTPRequestHandler):

    def sendContent(self, content):
        if type(content) != bytes:
            content = bytes(content, "UTF8")
        self.wfile.write(content)

    def sendBin(self, pathname):
        try:
            blob = open(pathname, 'rb').read()
        except:
            sys.stderr.write("Couldn't read bin file \"%s\"\r\n" % (pathname))
            self.send_response(500)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b'<html><head><title>Upgrade server error 500</title></head><body><h1>:-(</h1><p>Failed to read bin file "%s"</p></body></html>' % pathname)
        else:
            sys.stdout.write("Loaded bin file \"%s\"\r\n" % (pathname))
            self.send_response(200)
            self.send_header('Content-type', 'application/octet-stream')
            self.end_headers()
            self.sendContent(blob)

    def do_GET(self):
        "Respond to HTTP GET requests"
        urlparts = urlparse(self.path)
        if urlparts.path == "/user1.bin":
            self.sendBin("bin/upgrade/user1.512.new.0.bin")
        elif urlparts.path == "/user2.bin":
            self.sendBin("bin/upgrade/user2.512.new.0.bin")
        else:
            self.send_response(404)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.sendContent("<html><head><title>Upgrade server 404</title></head><body><h1>?:-/</h1><p>I do not know this \"%s\" of which you speak</body></html>" % urlparts.path)


def commandUpgrade(serverIp, robotIp, kind):
    "Command the espressif to do some kind of upgrade"
    command = b"robotsRising" + struct.pack("BBBBHH", serverIp[0], serverIp[1], serverIp[2], serverIp[3], HTTP_SERVER_PORT, kind)
    comsock = socket.socket(type=socket.SOCK_DGRAM)
    comsock.sendto(command, ('%d.%d.%d.%d' % tuple(robotIp), COMMAND_SOCKET))

if __name__ == '__main__':
    if len(sys.argv) != 4:
        sys.stderr.write(USAGE)
        sys.exit(-1)
    try:
        serverIp = [int(o) for o in sys.argv[1].split('.')]
        assert len(serverIp) == 4
    except:
        sys.stderr.write("Couldn't parse server IP address \"%s\"\r\n" % sys.argv[1])
        sys.exit(1)
    try:
        robotIp = [int(o) for o in sys.argv[2].split('.')]
        assert len(robotIp) == 4
    except:
        sys.stderr.write("Couldn't parse robot IP address \"%s\"\r\n" % sys.argv[2])
        sys.exit(2)
    try:
        upgradeCommand = int(sys.argv[3])
        assert upgradeCommand in (0, 1, 2, 3, 4)
    except:
        sys.stderr.write("Couldn't parse robot upgrade command \"%s\"\r\n" % sys.argv[3])
        sys.exit(3)

    httpd = httpServer.HTTPServer(('%d.%d.%d.%d' % tuple(serverIp), HTTP_SERVER_PORT), UpgradeRequestHandler)
    httpdThread = threading.Thread(target=httpd.serve_forever)
    sys.stdout.write("Starting HTTP server\r\n")
    httpdThread.start()

    sys.stdout.write("Commanding upgrade\r\n")
    commandUpgrade(serverIp, robotIp, upgradeCommand)

    sys.stdout.write("^C to quit\r\n")
    try:
        time.sleep(120)
    except KeyboardInterrupt:
        pass

    httpd.server_close()
