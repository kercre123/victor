#!/usr/bin/env python3
"""
Anki ESP8266 flasher utility using a RAM loader program.
This file includes parts duplicated from esptool.py and updated as needed.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys,os,serial,struct,math,time,argparse

class ESPROM:

    # These are the currently known commands supported by the ROM
    ESP_FLASH_BEGIN = 0x02
    ESP_FLASH_DATA  = 0x03
    ESP_FLASH_END   = 0x04
    ESP_MEM_BEGIN   = 0x05
    ESP_MEM_END     = 0x06
    ESP_MEM_DATA    = 0x07
    ESP_SYNC        = 0x08
    ESP_WRITE_REG   = 0x09
    ESP_READ_REG    = 0x0a

    # Maximum block sized for RAM and Flash writes, respectively.
    ESP_RAM_BLOCK   = 0x1800
    ESP_FLASH_BLOCK = 0x400

    # Default baudrate. The ROM auto-bauds, so we can use more or less whatever we want.
    ESP_ROM_BAUD    = 230400

    # First byte of the application image
    ESP_IMAGE_MAGIC = 0xe9

    # Initial state for the checksum routine
    ESP_CHECKSUM_MAGIC = 0xef

    # OTP ROM addresses
    ESP_OTP_MAC0    = 0x3ff00050
    ESP_OTP_MAC1    = 0x3ff00054

    # Sflash stub: an assembly routine to read from spi flash and send to host
    SFLASH_STUB     = b"\x80\x3c\x00\x40\x1c\x4b\x00\x40\x21\x11\x00\x40\x00\x80" \
            b"\xfe\x3f\xc1\xfb\xff\xd1\xf8\xff\x2d\x0d\x31\xfd\xff\x41\xf7\xff\x4a" \
            b"\xdd\x51\xf9\xff\xc0\x05\x00\x21\xf9\xff\x31\xf3\xff\x41\xf5\xff\xc0" \
            b"\x04\x00\x0b\xcc\x56\xec\xfd\x06\xff\xff\x00\x00"

    def __init__(self, port = 0, baud = ESP_ROM_BAUD):
        self._port = serial.Serial(port, baud)

    def __del__(self):
        self._port.close()

    def read(self, length = 1):
        """ Read bytes from the serial port while performing SLIP unescaping """
        b = b''
        while len(b) < length:
            c = self._port.read(1)
            if c == b'\xdb':
                c = self._port.read(1)
                if c == b'\xdc':
                    b = b + b'\xc0'
                elif c == '\xdd':
                    b = b + b'\xdb'
                else:
                    raise Exception('Invalid SLIP escape')
            else:
                b = b + c
        return b

    def write(self, packet):
        """ Write bytes to the serial port while performing SLIP escaping """
        buf = b'\xc0'+(packet.replace(b'\xdb',b'\xdb\xdd').replace(b'\xc0',b'\xdb\xdc'))+b'\xc0'
        self._port.write(buf)

    @staticmethod
    def checksum(data, state = ESP_CHECKSUM_MAGIC):
        """ Calculate checksum of a blob, as it is defined by the ROM """
        for b in data:
            state ^= b
        return state

    def command(self, op = None, data = None, chk = 0):
        """ Send a request and read the response """
        if op:
            # Construct and send request
            pkt = struct.pack('<BBHI', 0x00, op, len(data), chk) + data
            self.write(pkt)

        # Read header of response and parse
        for i in range(10):
            if self._port.read(1) == b'\xc0':
                break
        else:
            raise Exception('No packet header found')
        hdr = self.read(8)
        (resp, op_ret, len_ret, val) = struct.unpack('<BBHI', hdr)
        if resp != 0x01 or (op and op_ret != op):
            raise Exception('Invalid response')

        # The variable-length body
        body = self.read(len_ret)

        # Terminating byte
        term = self._port.read(1)
        if term[0] != 0xc0:
            if op != self.ESP_MEM_END:
                raise Exception('Invalid end of packet {}'.format(repr(term)))

        return val, body

    def sync(self):
        """ Perform a connection test """
        self.command(ESPROM.ESP_SYNC, b'\x07\x07\x12\x20'+32*b'\x55')
        for i in range(7):
            self.command()

    def connect(self):
        """ Try connecting repeatedly until successful, or giving up """
        print('Connecting...')

        # RTS = CH_PD (i.e reset)
        # DTR = GPIO0
        self._port.setRTS(True)
        self._port.setDTR(True)
        self._port.setRTS(False)
        time.sleep(0.1)
        self._port.setDTR(False)

        self._port.timeout = 0.1
        for i in range(10):
            try:
                self._port.flushInput()
                self._port.flushOutput()
                self.sync()
                self._port.timeout = 5
                return
            except Exception as e:
                sys.stdout.write(str(e))
                sys.stdout.write(os.linesep)
                time.sleep(0.1)
        raise Exception('Failed to connect')

    def mem_begin(self, size, blocks, blocksize, offset):
        """ Start downloading an application image to RAM """
        if self.command(ESPROM.ESP_MEM_BEGIN,
                struct.pack('<IIII', size, blocks, blocksize, offset))[1] != b"\0\0":
            raise Exception('Failed to enter RAM download mode')

    def mem_block(self, data, seq):
        """ Send a block of an image to RAM """
        if self.command(ESPROM.ESP_MEM_DATA,
                struct.pack('<IIII', len(data), seq, 0, 0)+data, ESPROM.checksum(data))[1] != b"\0\0":
            raise Exception('Failed to write to target RAM')

    def mem_finish(self, entrypoint = 0):
        """ Leave download mode and run the application """
        if self.command(ESPROM.ESP_MEM_END,
                struct.pack('<II', int(entrypoint == 0), entrypoint))[1] != b"\0\0":
            raise Exception('Failed to leave RAM download mode')

class ESPFirmwareImage:

    def __init__(self, filename = None):
        self.segments = []
        self.entrypoint = 0
        self.flash_mode = 0
        self.flash_size_freq = 0

        if filename is not None:
            f = open(filename, 'rb')
            (magic, segments, self.flash_mode, self.flash_size_freq, self.entrypoint) = struct.unpack('<BBBBI', f.read(8))

            # some sanity check
            if magic != ESPROM.ESP_IMAGE_MAGIC or segments > 16:
                raise Exception('Invalid firmware image')

            for i in range(segments):
                (offset, size) = struct.unpack('<II', f.read(8))
                if offset > 0x40200000 or offset < 0x3ffe0000 or size > 65536:
                    raise Exception('Suspicious segment %x,%d' % (offset, size))
                self.segments.append((offset, size, f.read(size)))

            # Skip the padding. The checksum is stored in the last byte so that the
            # file is a multiple of 16 bytes.
            align = 15-(f.tell() % 16)
            f.seek(align, 1)

            self.checksum = f.read(1)[0]

    def add_segment(self, addr, data):
        # Data should be aligned on word boundary
        l = len(data)
        if l % 4:
            data += b"\x00" * (4 - l % 4)
        self.segments.append((addr, len(data), data))

    def save(self, filename):
        f = file(filename, 'wb')
        f.write(struct.pack('<BBBBI', ESPROM.ESP_IMAGE_MAGIC, len(self.segments),
            self.flash_mode, self.flash_size_freq, self.entrypoint))

        checksum = ESPROM.ESP_CHECKSUM_MAGIC
        for (offset, size, data) in self.segments:
            f.write(struct.pack('<II', offset, size))
            f.write(data)
            checksum = ESPROM.checksum(data, checksum)

        align = 15-(f.tell() % 16)
        f.seek(align, 1)
        f.write(struct.pack('B', checksum))

def load_ram(port, baud, filename):
    image = ESPFirmwareImage(filename)
    
    esp = ESPROM(port, baud)
    esp.connect()
    
    sys.stdout.write("Loading RAM")
    sys.stdout.write(os.linesep)
    for (offset, size, data) in image.segments:
        sys.stdout.write('Uploading {:d} bytes to {:08x}...'.format(size, offset))
        esp.mem_begin(size, math.ceil(size / float(esp.ESP_RAM_BLOCK)), esp.ESP_RAM_BLOCK, offset)
        seq = 0
        while len(data) > 0:
            esp.mem_block(data[0:esp.ESP_RAM_BLOCK], seq)
            data = data[esp.ESP_RAM_BLOCK:]
            seq += 1
        sys.stdout.write(' done!')
        sys.stdout.write(os.linesep)

    sys.stdout.write('All segments done, executing at {:08x}{}'.format(image.entrypoint, os.linesep))
    esp.mem_finish(image.entrypoint)
    del esp

class TFI(serial.Serial):
    """Class for communicating with the ESP RAM loader app"""
    
    DEFAULT_BAUD = 230400
    PAYLOAD_LEN = 64
    SUCCESS = b'0'
    SECTOR_SIZE = 0x1000
    
    def __init__(self, port):
        serial.Serial.__init__(self, port, self.DEFAULT_BAUD)
        self.flushOutput()
        self.flushInput()
        self.timeout = 0.01
        while self.read(1) != b'0':
            self.write(b'0')
        self.timeout = None
    
    def command(self, op, addr, payload=None):
        "Sends a command to the loader and returns response code"
        assert addr < (1<<24)
        assert payload is None or len(payload) == self.PAYLOAD_LEN
        self.write(op)
        self.write(struct.pack("I", addr)[:3]) # 24 bits of addr
        if payload:
            self.write(payload)
        return self.read(1)
    
    def erase_sector(self, sector):
        print("Erasing sector 0x{:x}".format(sector))
        rslt = self.command(b'e', sector)
        if rslt == self.SUCCESS:
            return True
        else:
            raise Exception("Error erasing sector 0x{:x}, {:s}".format(sector, chr(rslt)))
    
    def read_flash(self, addr):
        print("Reading back from 0x{:x}".format(addr))
        rslt = self.command(b'r', addr)
        if rslt == self.SUCCESS:
            data = self.read(self.PAYLOAD_LEN)
            return data
        else:
            raise Exception("Error reading from 0x{:x}, {:s}".format(addr, chr(rslt)))
    
    def write_flash(self, startingAddress, data):
        wrAddr = startingAddress
        while data:
            payload = data[:self.PAYLOAD_LEN]
            data = data[self.PAYLOAD_LEN:]
            if len(payload) < self.PAYLOAD_LEN:
                payload += b"\xff" * (self.PAYLOAD_LEN - len(payload))
            print("Writing to flash at 0x{:x}".format(wrAddr))
            rslt = self.command(b'f', wrAddr, payload)
            if rslt == self.SUCCESS:
                readBack = self.read(self.PAYLOAD_LEN)
                if readBack != payload:
                    raise Exception("Flashing error, readback != payload:{linesep}\t{payload:s}{linesep}\t{readBack:s}{linesep}".format(payload=repr(payload), readBack=repr(readBack), linesep=os.linesep))
                wrAddr += self.PAYLOAD_LEN
            else:
                raise Exception("Error flashing to 0x{:x}, {:s}".format(wrAddr, chr(rslt)))

def parseInt(arg):
    return int(eval(arg))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog=sys.argv[0], description="Fast flasher for ESP8266 using RAM loader program")
    parser.add_argument("port", type=str, help="Serial port to connect to the ESP8266")
    parser.add_argument("loader", type=str, help="File for the flash loader RAM program firmware image")
    parser.add_argument("--baud", "-b", type=parseInt, default=230400, help="BAUD rate for loading RAM, doesn't affect flash programming")
    parser.add_argument("--erase", "-e", type=parseInt, action="append", help="Erase a sector of flash")
    parser.add_argument("--read",  "-r", type=parseInt, action="append", help="Read data back from flash")
    parser.add_argument("--prog",  "-f", nargs=2, help="Program firmware file to flash")
    args = parser.parse_args()
    
    load_ram(args.port, args.baud, args.loader)
    
    if args.erase:
        tfi = TFI(args.port)
        for sector in args.erase:
            tfi.erase_sector(sector)
    elif args.read:
        tfi = TFI(args.port)
        for addr in args.read:
            data = tfi.read_flash(addr)
            sys.stdout.write(repr(data))
            sys.stdout.write(os.linesep)
    elif args.prog:
        tfi = TFI(args.port)
        startingAddress, filename = args.prog
        startingAddress = parseInt(startingAddress)
        image = open(filename, 'rb').read()
        startingSector = math.floor(startingAddress/TFI.SECTOR_SIZE)
        sectors = math.ceil(len(image)/TFI.SECTOR_SIZE)
        for s in range(startingSector, startingSector+sectors):
            tfi.erase_sector(s)
        tfi.write_flash(startingAddress, image)
