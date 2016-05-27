from argparse import ArgumentParser
from Crypto.PublicKey import RSA
import sys

parser = ArgumentParser()
parser.add_argument("output", type=str,
                    help="target location header")
parser.add_argument("-k", "--key", type=str,
                    help="source location for certificate")

def long_to_words (val, bits = 16):
    bytes = []
    
    mask = 1 << bits
    while val > 0:
        byte, val = val % mask, val >> bits
        bytes += [byte]
    
    return bytes

args = parser.parse_args()

with open (args.key, "r") as fo:
	key = RSA.importKey(fo.read())
	mod, exp = key.n, key.e

	with open(args.output, "w") as target:
		mod = long_to_words(mod)
		exp = long_to_words(exp)
		format = len(mod), ', '.join(["0x%02X" % b for b in mod]), len(exp), ', '.join(["0x%02X" % b for b in exp])

		target.write ("""
static const big_rsa_t CERT_RSA = {
	/* Modulus for RSA */
	{ false, %i, { %s } },
	/* Public exponent for RSA */
	{ false, %i, { %s } }
};
""" % format)
