#!/usr/bin/env python3
def gen(b):
	prev = -1
	for i in range(6,-1,-1):
		now = (b >> i) & 1

		if prev != now:
			if now:
				print ("    LED_DAT::reset();")
			else:
				print ("    LED_DAT::set();")
			prev = now

		print ('    wait(); LED_CLK::set(); wait(); LED_CLK::reset();')


def led(x, y):
	pattern = (8 << x) | (0x7 ^ y)

	print ("static void led_%sx%i(void) {" % (y, x))
	gen (pattern)
	print ("    kick_off();")
	print ("}")

for x in range(4):
	for y in range(8):
		led(x, y)
