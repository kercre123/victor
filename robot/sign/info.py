from Crypto.Hash.SHA import SHA1Hash
from Crypto.Cipher.AES import AESCipher
from Crypto.Random import random
from Crypto.Util import number
from Crypto.Cipher import PKCS1_OAEP
from Crypto.PublicKey import RSA

import os.path as path
from sys import argv
from struct import pack

from elftools.elf.elffile import ELFFile

HEADER_LENGTH = 128

print "Building rom"
with file(argv[1], "rb") as fo:
	elffile = ELFFile(fo)

	segments = {}

	# Build rom segments
	print "====="
	for segment in elffile.iter_segments():
		print segment.header

	# Locate vector table + magic header
	for section in elffile.iter_sections():
		print section.name
		print section.header
