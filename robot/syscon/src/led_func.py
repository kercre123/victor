def generate(data, color, group):
	print "// {0:08b}".format(data)
	print "static void led_%ix%i(void) {" % (color, group)

	last_bit = None # Forces first bit to be set
	for i in range(8):
		next_bit = bool(data & (0x80 >> i))

		if next_bit != last_bit:
			if next_bit:
				print "    LED_DAT::set();   ",
			else:
				print "    LED_DAT::reset(); ",
		else:
			print "    wait();           ",
		last_bit = next_bit

		print "LED_CLK::set(); wait(); LED_CLK::reset();"

	print "    kick_off();"
	print "}"

for group, mask in enumerate([0b11000000, 0b10100000, 0b01100000]):
	for rgb in range(1, 8):
		generate(rgb | mask, rgb, group)

for i in range(1,4):
	generate(0b11100000 | (i << 3), i, 3)
