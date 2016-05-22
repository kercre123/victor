import sys
import re
import itertools

if len(sys.argv) < 3:
	print "usage: hex2bin.py <start address> <end address> <input file> <output file>"
	sys.exit()

def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i+n]

def to_bytes(hex):
	return [int(x, 16) for x in chunks(hex, 2)]

def to_int(hex):
	bytes = to_bytes(hex)
	bytes.reverse()
	return sum([b << (i * 8) for i, b in enumerate(bytes)])

def crc(hex):
	total = sum(to_bytes(hex))
	return (total ^ 0xFF) + 1 & 0xFF

def decode(lines):
	rows = []
	base_address = 0

	for line in lines:
		m = re.match(r":(([a-f0-9]{2})([a-f0-9]{4})([a-f0-9]{2})(([a-f0-9]{2})+))([a-f0-9]{2})", line, re.I)

		if not m:
			continue

		message, data_len, address, record, data, _, checksum = m.groups()

		if int(checksum, 16) != crc(message):
			print "Record checksum invalid"

		data_len = int(data_len, 16)
		address = to_int(address) + base_address
		record = int(record, 16)

		if record == 0:
			rows += [(address, to_bytes(data))]
		elif record == 4:
			base_address = to_int(data) << 16
		elif record == 5:
			# We don't care about linear address
			pass
		else:
			print line
			raise Exception("Unrecognized record type %2x" % record)

	return rows


_, start_addr, end_addr, inp, outp = sys.argv
start_addr = int(start_addr, 16)
end_addr = int(end_addr, 16)

with file(inp, "r") as fi:
	rows = [(addr, data) for addr, data in decode(fi.readlines()) if addr >= start_addr and addr + len(data) <= end_addr]
	tail = 0

	rom = [0] * end_addr

	for addr, data in rows:
		tail = max(tail, addr+len(data))
		rom[addr:addr+len(data)] = data

 	rom = ''.join([chr(c) for c in rom[start_addr:tail]])


with file(outp, "wb") as fo:
	print outp
	fo.write(rom)
