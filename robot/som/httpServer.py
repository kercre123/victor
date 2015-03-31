"""
Very basic web server for Cozmo 3
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import os, time, socket, threading, re, subprocess
from subserver import BaseSubServer

class HTTPSubServer(BaseSubServer):

    HOSTNAME = socket.gethostname()
    ERROR_RESPONSE="""HTTP/1.0 404 Not Found\r\n\r\n"""
    CFG_LINKS = '<a href="/cozmo">cozmo</a> ' + ' '.join(['<a href="/cozmo%d">cozmo%d</a>' % (i, i) for i in range(1,10)])
    CFG_RE = re.compile(r'GET /(cozmo\d{0,1}) HTTP')
    WPA_RE = re.compile(r'.*ssid="([a-zA-Z0-9]+)".*psk=([0-9a-f]+).*', re.DOTALL)

    def __init__(self, server, verbose=False, port=80, bind=''):
        BaseSubServer.__init__(self, server, verbose)
        self.acceptSock = None
        self.acceptAddr = (bind, port)

    def stop(self):
        BaseSubServer.stop(self)
        self.acceptSock.close()

    def giveMessage(self, message):
        "HTTPSubServer doesn't process messages from basestation"
        pass

    def step(self):
        "One main loop itteration for the sub server thread"
        if self.acceptSock is None:
            try:
                acceptSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                acceptSock.bind(self.acceptAddr)
                acceptSock.settimeout(1.0)
                acceptSock.listen(1)
            except socket.error:
                time.sleep(1)
                return
            else:
                self.log("Successfully started HTTP server\n")
                self.acceptSock = acceptSock
        try:
            newCliSock, newCliAddr = self.acceptSock.accept()
        except socket.timeout:
            pass # This happens all the time
        except socket.error, e:
            if e.errno == socket.EBADF and self._continue == False: # Happens during shutdown
                return
            else:
                raise e
        else:
            if self.v: self.log("New HTTP client at %s:%d\n" % newCliAddr)
            req = newCliSock.recv(1500)
            if req.startswith('GET / HTTP'):
                newCliSock.send("""HTTP/1.0 200 OK\r
\r
<!doctype html><html>\r
<head><title>%s</title></head>\r
<body>\r
  <h1>%s</h1>\r
  <p><strong>Change Cozmo ID:</strong> %s</p>
  <p><strong>Set FOV:</strong> <a href="/fov_crop_on">Crop</a> / <a href="/fov_crop_off">Wide</a></p>
  <h2>Log</h2>
  <pre>\r
%s\r
</pre>\r
</body>\r
</html>\r
  """ % (self.HOSTNAME, self.HOSTNAME, self.CFG_LINKS, open(self.server.logger.ACTIVE_FILE, 'r').read()))
                newCliSock.close()
            elif self.CFG_RE.match(req):
                self.respondAndChangeID(self.CFG_RE.match(req).groups()[0], newCliSock)
            elif req.startswith('GET /fov_crop_on HTTP'):
                self.respondAndSetFOV(True, newCliSock)
            elif req.startswith('GET /fov_crop_off HTTP'):
                self.respondAndSetFOV(False, newCliSock)
            else:
                newCliSock.send(self.ERROR_RESPONSE + req)
                newCliSock.close()

    def respondAndChangeID(self, newID, cliSock):
        "Change the ID of the cozmo"
        self.log("Changing Cozmo ID from %s to %s\n" % (self.HOSTNAME, newID))
        # Send HTTP response
        cliSock.send("""HTTP/1.0 200 OK\r
\r
<!doctype html><html>\r
<head><title>Changing robot id</title></head>\r
<body>\r
<h1>Changing robot id</h1>\r
<p>From %s to %s</p>
<p>Please wait about 10 seconds and reconnect to the new robot wifi.</p>
</body>
</html>""" % (self.HOSTNAME, newID))
        cliSock.close()
        # Get new wifi config
        ssid = 'AnkiTorpedo' + ('' if newID == 'cozmo' else newID[-1])
        wpaph = subprocess.Popen(['wpa_passphrase', ssid, '2manysecrets'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while wpaph.poll() is None:
            self.threadYield()
        out, err = wpaph.communicate()
        if err:
            self.log("wpa_passphrase error:\n\t%s\n" % err)
            return
        m = self.WPA_RE.match(out)
        if not m:
            self.log("wpa_passphrase didn't produce expected output:\n\t%s\n" % out)
            return
        # Now change the host name
        rcconf = open('/etc/rc.d/rc.conf', 'r').read()
        open('/etc/rc.d/rc.conf', 'w').write(rcconf.replace('"%s"' % self.HOSTNAME, '"%s"' % newID))
        subprocess.call(['hostname', newID])
        # And change the hostAPD config
        open('/etc/hostapd.conf', 'w').write("""interface=wlan0
driver=nl80211
ieee80211n=1
ieee80211d=1
wmm_enabled=1
hw_mode=g
channel=7
ht_capab=[SHORT-GI-20]
ssid=%s
country_code=US
auth_algs=3
wpa=3
wpa_psk=%s
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP CCMP
""" % m.groups())
        # And restart the whole system
        self.server.stop()
        time.sleep(2.0)
        subprocess.call(['/etc/rc.d/init.d/hostapd', 'restart'])
        subprocess.call(['/etc/rc.d/init.d/cozmo_server', 'restart'])

    def respondAndSetFOV(self, crop, cliSock):
        "Change the FOV, respond and restart daemon"
        self.log("Setting crop to %s\n" % crop)
        # Set flags
        if crop:
            subprocess.call(['touch', '/root/som/FOV_CROP'])
        else:
            try:
                os.remove('/root/som/FOV_CROP')
            except:
                pass
            try:
                os.remove('/ramdisk/FOV_CROP')
            except:
                pass
        # Send response
        cliSock.send("""HTTP/1.0 200 OK\r
\r
<!doctype html><html>\r
<head><title>Setting FOV</title></head>\r
<body>\r
<h1>Setting FOV</h1>\r
<p>%s</p>
</body>
</html>""" % ("Crop" if crop else "Wide"))
        cliSock.close()
        # Restart daemon
        self.server.stop()
        time.sleep(2.0)
        subprocess.call(['/etc/rc.d/init.d/cozmo_server', 'restart'])
