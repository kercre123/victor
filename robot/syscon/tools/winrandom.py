# This makes python3 work with pycrypto on windows without having to go through
# a ton of garbage
import os

class WinRandom:
	def get_bytes(self, n):
		return os.urandom(n)

def new(*args, **kwargs):
	return WinRandom(*args, **kwargs)
