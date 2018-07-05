#!/usr/bin/env python3
from argparse import ArgumentParser
from cert import loadCert
import os
import sys

parser = ArgumentParser()
parser.add_argument("output", type=str,
										help="target location header")
parser.add_argument("-k", "--key", type=str,
										help="source location for certificate")

CELL_BITS = 16

def toHex(iter):
	return ', '.join(["0x%02x" % b for b in iter])

def long_to_words (val, bits = 16):
		bytes = []

		mask = 1 << bits
		while val > 0:
				byte, val = val % mask, val >> bits
				bytes += [byte]

		return bytes

args = parser.parse_args()

def ffs(num):
	if not num:
		return -1

	lsb = 0
	while not (num >> lsb) & 1:
		lsb += 1

	return lsb

def invm(a_, b_):
	b = b_
	x1 = 1
	x2 = 0

	if a_ < 0:
		a = a_ % b_
	else:
		a = a_

	while a > 1 and b > 1:
		a_lsb = ffs(a)

		if a_lsb > 0:
			a = a >> a_lsb

			while a_lsb > 0:
				if x1 & 1:
					x1 = x1 + b_
				x1 = x1 >> 1
				a_lsb = a_lsb - 1

		b_lsb = ffs(b)

		if b_lsb > 0:
			b = b >> b_lsb

			while b_lsb > 0:
				if x2 & 1:
					x2 = x2 + b_
				x2 = x2 >> 1
				b_lsb -= 1

		if a >= b:
			a = a - b
			x1 = x1 - x2
		else:
			b = b - a
			x2 = x2 - x1

	if a == 1:
		out = x1
	else:
		out = x2

	if out < 0:
		out = out + b

	return out

def writeLong(num):
	num = long_to_words(num)
	return "{ false, 0x%x, { %s } }" % (len(num), toHex(num))

def calcmont(key):
	modulo = key.n
	shift = modulo.bit_length()

	if shift % CELL_BITS:
		shift += CELL_BITS - shift % CELL_BITS

	r = 1 << shift
	r2 = (r * r) % modulo
	rinv = invm(r, modulo)
	minv = r - ((rinv * r - 1) // modulo) % r
	one = (1 << shift) % modulo

	target.write ("""static const big_mont_t RSA_CERT_MONT =
{
	0x%x,
	%s,
	%s,
	%s,
	%s
};
""" % (shift, writeLong(one), writeLong(modulo), writeLong(rinv), writeLong(minv)))


def calccert(key):
	target.write ("""
static const big_num_micro_t CERT_RSA_EXP = %s;
""" % writeLong(key.e))

with open(args.output, "w") as target:
	key = loadCert(args.key if args.key else None)
	calcmont(key)
	calccert(key)
