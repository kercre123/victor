#!/usr/bin/python 
# Must use the Apple installed version of python for PyObjC
"""
Script to configure Cozmo's WiFi over BLE
"""
__author__ = "Daniel Casner <daniel@anki.com>"
import sys, os, time, hashlib, struct, logging, uuid, random
import Adafruit_BluefruitLE

sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI   = robotInterface.RI

COZMO_SERVICE_UUID = uuid.UUID('763dbeef-5df1-405e-8aac-51572be5bab3')
TO_PHONE_UUID      = uuid.UUID('763dbee0-5df1-405e-8aac-51572be5bab3')
TO_ROBOT_UUID      = uuid.UUID('763dbee1-5df1-405e-8aac-51572be5bab3')
BLE_CHUNK_SIZE = 16
BLE_FIRST_CHUNK = 0x01
BLE_LAST_CHUNK  = 0x02
BLE_ENCRYPTED   = 0x04
CONFIG_STRING_LENGTH = 16

def byte(i):
    "Converts an int to a byte in the way appropriate for the python version"
    if sys.version_info.major < 3:
        return chr(i)
    else:
        return bytes([i])

def b2i(byte):
    "Converts a byte to an int if nessisary for python2"
    if type(byte) is str:
        return ord(byte)
    else:
        return byte

def paddedString(s):
    "0 padds a string out to the proper size for config messages"
    if len(s) > CONFIG_STRING_LENGTH:
        raise ValueError("String too long")
    else:
        padded = bytes(s) + (b"\x00" * (CONFIG_STRING_LENGTH - len(s)))
        if sys.version_info.major < 3:
            return [ord(b) for b in padded]
        else:
            return padded

class CozmoBLE:
    "Class for communicating with robot over Bluetooth Low-Energy"

    # Code for the BLE thread
    def bleTask(self):
        # Clear any cached data because both bluez and CoreBluetooth have issues with caching data and it going stale.
        self.ble.clear_cached_data()
        
        # Get the first available BLE network adapter and make sure it's powered on.
        adapter = self.ble.get_default_adapter()
        adapter.power_on()
        print('Using adapter: {0}'.format(adapter.name))
        
        # Disconnect any currently connected UART devices.  Good for cleaning up and starting from a fresh state.
        print('Disconnecting any connected robots...')
        self.ble.disconnect_devices([COZMO_SERVICE_UUID])
        
        # Scan
        try:
            adapter.start_scan()
            # Search for the first device found (will time out after 60 seconds but you can specify an optional timeout_sec
            # parameter to change it).
            while not self.ble.list_devices():
                time.sleep(1)
            self.device = self.ble.find_device(service_uuids=[COZMO_SERVICE_UUID])
            if self.device is None:
                raise RuntimeError("Couldn't find any robots!")
        finally:
            adapter.stop_scan()
            
        # Connect
        print("Connecting to discovered robot...")
        self.device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter to change the timeout.
        
        # Wait for service discovery to complete for at least the specified  service and characteristic UUID lists.
        # Will time out after 60 seconds (specify timeout_sec parameter to override).
        print('Discovering services...')
        self.device.discover([COZMO_SERVICE_UUID], [TO_ROBOT_UUID, TO_PHONE_UUID])
        
        robot = self.device.find_service(COZMO_SERVICE_UUID)
        self.rx = robot.find_characteristic(TO_PHONE_UUID)
        self.tx = robot.find_characteristic(TO_ROBOT_UUID)
        self.rx.start_notify(self.onReceive)
        
        print("Sending data")
        self.send(RI.EngineToRobot(appConCfgString=RI.AppConnectConfigString(
            RI.APConfigStringID.AP_AP_SSID_0,
            paddedString("IM0BB8")
        )))
        if not self.waitForAck(): return
        self.send(RI.EngineToRobot(appConCfgString=RI.AppConnectConfigString(
            RI.APConfigStringID.AP_AP_PSK_0,
            paddedString("2manysecrets")
        )))
        if not self.waitForAck(): return
        
        time.sleep(1.0)

    def __init__(self, key=None):
        self.key = key
        self.ble = Adafruit_BluefruitLE.get_provider()
        self.ble.initialize()
        self.device = None
        self.msgQueue = []
        self._continue = False
        self.dechunkBuffer = b""
        self.acked = False

    def __del__(self):
        self._continue = False
        self.disconnect()

    def start(self):
        """Start the mainloop to process BLE events, and run the provided function in
           a background thread.  When the provided main function stops running, returns
           an integer status code, or throws an error the program will exit."""
        self._continue = True
        self.ble.run_mainloop_with(self.bleTask)

    def disconnect(self):
        if self.device is not None:
            self.device.disconnect()
            self.device = None

    def stop(self):
        "Kill the BLE thread"
        self.disconnect()
        self._continue = False
        self._ready    = False

    def bleChunker(self, cladMessage):
        raw = cladMessage.pack()
        payload = byte(len(raw)) + raw
        while len(payload) % BLE_CHUNK_SIZE:
            payload += byte(int(random.uniform(0, 255)))
        if self.key is not None:
            # Do encryption here
            encrypted = BLE_ENCRYPTED
        else:
            encrypted = 0
        if len(payload) == BLE_CHUNK_SIZE: # Whole thing fits in one message
            yield payload + byte(BLE_FIRST_CHUNK | BLE_LAST_CHUNK | encrypted)
        else:
            yield payload[0:BLE_CHUNK_SIZE] + byte(BLE_FIRST_CHUNK | encrypted)
            payload = payload[BLE_CHUNK_SIZE:]
            while len(payload) > BLE_CHUNK_SIZE:
                yield payload[0:BLE_CHUNK_SIZE] + byte(encrypted)
                payload = payload[BLE_CHUNK_SIZE:]
            assert(len(payload) == BLE_CHUNK_SIZE)
            yield payload + byte(BLE_LAST_CHUNK | encrypted)
            
    def send(self, msg):
        self.acked = False
        for chunk in self.bleChunker(msg):
            self.tx.write_value(chunk)
            
    def waitForAck(self):
        while self.acked is False and self._continue is True:
            time.sleep(0.060)
        return self._continue
            
    def onReceive(self, data):
        "Handler for received data"
        sys.stdout.write("RAW RX: {}{linesep}".format(repr([hex(ord(d)) for d in data]), linesep=os.linesep))
        assert(len(data) == BLE_CHUNK_SIZE + 1)
        flags = b2i(data[BLE_CHUNK_SIZE])
        if flags & BLE_FIRST_CHUNK:
            if len(self.dechunkBuffer):
                sys.stderr.write("Got start of new message while trying to dechunk old one{linesep}".format(linesep=os.linesep))
            self.dechunkBuffer = data[0:16]
        if flags & BLE_LAST_CHUNK:
            if flags & BLE_ENCRYPTED:
                if self.key is None:
                    sys.stderr.write("Received encrypted message{linesep}".format(linesep=os.linesep))
                    self.dechunkBuffer = b""
                    return
                else:
                    pass # Do decryption
            cl = b2i(self.dechunkBuffer[0])
            assert(cl < len(self.dechunkBuffer))
            clad = RI.EngineToRobot.unpack(self.dechunkBuffer[1:cl+2])
            print(repr(clad))
            if clad.tag == clad.Tag.wifiCfgResult:
                if clad.wifiCfgResult.result == 0:
                    self.acked = True
            self.dechunkBuffer = b""

if __name__ == '__main__':
    cble = CozmoBLE()
    cble.start()
