// Thumb cruncher/decruncher sample
// This knocks another third off a binary that is already Thumb-compressed
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "crypto.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define IMAGE_START		0x2004
#define IMAGE_SIZE		(TARGET_IMAGE_SIZE+IMAGE_START)

static u16 m_src[IMAGE_SIZE];
static u16 m_dest[IMAGE_SIZE];
static u8 m_outbits[IMAGE_SIZE];
static u8 m_crypted[IMAGE_SIZE];

void bye(char* msg);

// Bitstream tuning parameters - optimized for Thumb opcodes
static const int bulkLen = 256;
static const int shortBits = 6, shortLen = (1<<shortBits);
static const int midBits = 10, midLen = (1<<midBits);
static const int maxLong = 14, minLong = 14;
static int longBits = maxLong;

#define PRECOPY		0	// Copy one word or branch verbatim
#define PREDELTA	2	// Copy one word and apply a 4 bit delta, or one branch with an 8 bit delta
#define PREDELTABIG	6	// Copy one word and apply a 9 bit delta, or one branch with a 15 bit address
#define PRELIT		1	// One word literal
#define PRESHORT	3	// Short distance
#define PREMID		5	// Mid distance
#define PRELONG		7	// Long distance

static const char* PREFIXES[] = { "copy", "lit", "delta", "#short", "?","#mid", "delbig", "#long", "bulk" };

// Put the next N bits (up to 16) to the bit buffer
// At EOF, write 7 1s to force last byte to spill
static u8* m_putp = m_outbits;
static void PutBits(int n, int value)
{
	// Merge the requested bits
	static u32 s_accum = 0, s_head = 0;
	value &= ((1<<n)-1);
	s_accum |= (value << s_head);
	s_head += n;

	// Spill out everything we can
	while (s_head > 7)
	{
		*m_putp++ = (s_accum & 255);
		s_accum >>= 8;
		s_head -= 8;
	}
}

// Put a prefix to the bitstream
// This is a simple task, but the function below adds useful debug/trace outputs
#define TRACING 0		// Set to 1 to see the codestream as it is output
#define STATS 0			// Set to 1 to collect and display stats
int m_pres[8], m_prebits[8][17], m_counts[65536];		// Count number of bits used by each prefix
void PutCode(u16* src, int i, int distance, int prebits, int pre, int valbits, int value)
{
	// This is almost entirely for tracing/stats
	static int depth = 0, countcopy = 0, copydist = 0, copyi = 0, countbits = 0, maxi = 0, reseti = 0, repeats = 0, lastpre = -1, codes = 0;

	// Collapse copies unless we are being called recursively
	if (!depth)
	{
		depth++;

		// Just queue up copy commands until we stop copying
		if (PRECOPY == pre && prebits == 2)
		{
			if (!countcopy)
				copyi = i;
			countcopy++;
			depth--;
			copydist = distance;
			return;
		}

		// As soon as copying is done, dump the prefixes
		while (countcopy > ((longBits+3)/2))
		{
			int maxcopy = countcopy > 127+5 ? 127+5 : countcopy;
			PutCode(src, copyi, copydist, 3, PRELONG, longBits, maxcopy-6);
			copyi += maxcopy;
			countcopy -= maxcopy;
		}
		while (countcopy)
		{
			PutCode(src, copyi++, copydist, 2, PRECOPY, 0, 0);
			countcopy--;		
		}
		depth--;
	}

	// Finally, write the requested prefix
	PutBits(prebits, pre);
	PutBits(valbits, value);

	if (pre == PRELONG && value < 256)
		pre = 8;
	int fixpre = pre<<(3-prebits);		// We can recover the "real" prefix even when outputting half-prefixes
	if (lastpre != fixpre)
		repeats = 0;
	lastpre = fixpre;
	repeats++;

	// Trace what was written
	static int tracei = -1;
	int sbranch = src && 0xf000 == (src[i-distance-1] & 0xf800) && 0xf800 == (src[i-distance] & 0xf800);
	int dbranch = src && 0xf000 == (src[i] & 0xf800) && 0xf800 == (src[i+1] & 0xf800);
	int isdist = ((fixpre & 1) == 1 && fixpre > 1);
	tracei++;
	if (TRACING && prebits && src) while (tracei < i) { printf("%04x: %04x.%04x=%04x\n", tracei, src[tracei], src[tracei-copydist-1], src[tracei]^src[tracei-copydist-1]); tracei++; }
	if (TRACING && src) if (!isdist) printf("%04x: %04x.%04x=%04x  ", i, src[i], src[i-distance-1], src[i]^src[i-distance-1]); else printf("                      ");
	if (TRACING && src) printf("%2db %6s x%-3d%04x", prebits+valbits, prebits ? PREFIXES[fixpre] : "", repeats, value & ((1<<valbits)-1));
	if (TRACING && !isdist && PRELIT != fixpre && src) printf(" (-%04x) %s%s", distance+1, sbranch ? "srcBX" : "", dbranch ? "dstBX" : "");
	if (TRACING) printf("\n");
	//if (tracei != i && src)
	//	m_counts[src[i]^src[i-distance-1]]++;

	if (STATS)
	{
		// Count the bits stored so far
		countbits += prebits + valbits;
		if (i > maxi)
			maxi = i;

		// Count the bits in the argument
		u32 j = value, bits = 0;
		while (j) {
			bits++;
			j >>= 1;
		}
		if (prebits) {
			m_prebits[fixpre][bits]++;
			m_pres[fixpre]++;
			codes++;
		}
		// Display stats so far and reset them
//		if (maxi > reseti+511)		
		if (i == 0 && maxi != 0)
		{
			printf("%04x: %d/%d bits, %0.2f%%\n", maxi, countbits, (maxi-reseti)*16, (countbits*100.0)/((maxi-reseti)*16.0));
			countbits = 0;
			reseti = maxi;

			for (int j = 0; j < 8; j++)
			{
				printf("%6s:", PREFIXES[j]);
				printf("%4d %5.2f%% ", m_pres[j], (m_pres[j]*100.0)/(codes*1.0));
				for (int k = 16; k >= 0; k--)
					printf("%d=%-4d", k, m_prebits[j][k]);
				printf("\n");
			}
			codes = 0;
			for (int j = 0; j < 65536; j++)
				if (m_counts[j])
					printf("/*%4d*/ 0x%04x,\n", m_counts[j], j);
			memset(m_pres, 0, sizeof(m_pres));
			memset(m_prebits, 0, sizeof(m_prebits));
			memset(m_counts, 0, sizeof(m_counts));
		}
	}
	tracei = i;
	copydist = distance;
}

// Shared local state used by compression
static int costs[IMAGE_SIZE];			// Cost in bits from this word to the end of file
static short matchlens[IMAGE_SIZE];	// If non-zero, the length in words of the optimal match starting here
static short dists[IMAGE_SIZE];		// The distance of the optimal match, or -1 if it is a bulk "literal match"

// Compress a Thumb binary found in memory at src with len u16 opcodes
// The first pass finds the optimal matches, the second pass emits the bitstream
void Crunch(u16* src, int len)
{
	u16* end = src + len;
	memset(costs, 0, sizeof(costs));

  // First pass:  Walk through each word in the source, and select most efficient matchpoints
	// Optimal coding requires knowledge of later codes, so we build the codes back to start
	for (int i = len - 1; i >= 0; i--)
	{
		// Attempt a sliding window match against previous data in the source
		int best = 10000000, distance = 0, matchlen = 0;
		int windowStart = i - (1<<longBits) + bulkLen - midLen - shortLen;
		if (windowStart < 0) {
			windowStart = 0;
			while (longBits > minLong && (i - shortLen - midLen) + bulkLen <= (1 << (longBits-1)))
			{
				longBits--;
				printf("At <=%04x, %d bits will do!\n", i, longBits);
			}
		}
		for (int k = windowStart; k < i; k++)
		{
			// Early out to reduce compression time - if no possible match, continue to next slot
			unsigned short* pi = src+i, *pk = src+k;
			if (!(0xf000 == (*pk & 0xf800)) && (*pi ^ *pk) > (511+16))
				continue;

			// Compute cost in bits of the header, minus 1 bit (since we drop the first copy/delta bit)
			int match = 0, cost = 0, dist = (i - k) - 1;
			if (dist < shortLen)
				cost += 3 - 1 + shortBits;
			else if (dist < shortLen + midLen)
				cost += 3 - 1 + midBits;
			else
				cost += 3 - 1 + longBits;

			// Check out the cost for every i and k, scanning forward until costs get out of hand
			while (pk < end)
			{
				int delta = (*pi ^ *pk);
        // XXX: Bail out to avoid a problem where the source word can be MISTAKEN for a branch by the bootloader..
        if (0 == dist && 0xf000 == (pk[0] & 0xf800))    // With distance of -1, next word will look like a branch..
          break;

				// If source word is a branch, enforce fuzzy branch match - do not allow any other type of match
				if (0xf000 == (pk[0] & 0xf800) && 0xf800 == (pk[1] & 0xf800))
				{
					// When source is branch, encoder guarantees decoder can/must perform the inverse of this math
					int offi = ((pi[0] & 0x7ff)<<11) + (pi[1] & 0x7ff);	// Decode Thumb BX format
					int offk = ((pk[0] & 0x7ff)<<11) + (pk[1] & 0x7ff);
					offi = ((offi<<10)>>10) + (pi-src);
					offk = ((offk<<10)>>10) + (pk-src);

					// If we can't encode this as a copy or delta op, try a literal
					if (offi < 0 || offi > 32767 || 0xf000 != (pi[0] & 0xf800) || 0xf800 != (pi[1] & 0xf800)) {
						if (!match)		// Must bail out since we can't encode literal as first output
							break;
						cost += 3 + 16;			// Literal
					} else {
						int howfar = offi-offk;
						if (0 == howfar)
							cost += 2;			// Simple copy
						else if (howfar >= -128 && howfar < 128)
							cost += 3 + 8;		// Standard delta
						else
							cost += 3 + 15;		// Big delta
						pi++;		// Skip second word of branch
						pk++;
						match++;
					}
				} else if (0 == delta) {
					cost += 2;					// Simple copy
				} else if (delta >= 1 && delta <= (511+16)) {	// Max encodable delta is 511+16
					if (delta <= 16)
						cost += 3 + 4;			// Standard delta
					else
						cost += 3 + 9;			// Big delta
				} else {
					if (!match)		// Must bail out since we can't encode literal as first output
						break;
					cost += 3 + 16;				// Literal
				}
				pi++;
				pk++;
				match++;

				// Take cost of all future codes into account, then pick this one if it's better
				int total = cost + costs[i+match];
				if (total <= best) {
					best = total;
					distance = dist;
					matchlen = match;
				}

				// Bail out once our costs appear to be growing past the point of no return (well above ~200, which catches everything)
				if (total > costs[i+match]+200)
					break;
			}
		}

		// If a literal is as cheap or cheaper, go with that
		int cost = 3 + 16 + costs[i+1];
		if (cost <= best)
		{
			best = cost;
			matchlen = distance = 0;	// Clear the match
		}

		// Test all size literal pools - if any are cheaper, just do that
		for (int k = 4+127; k >= 4; k--)
		{
			int cost = 3 + longBits + 16 * k + costs[i+k];
			if (cost <= best)
			{
				best = cost;
				matchlen = k;
				distance = -1;	// Lit pool match
			}
		}
		
		// Save the winner
		costs[i] = best;
		matchlens[i] = matchlen;
		dists[i] = distance;
	}

	// Second pass:  Using the matches selected in the first pass, emit the bits from start to end
	int matchend = 0, distance = 0;
	for (int i = 2; i < len; i++)   // Skip over canary (first 4 bytes)
	{
		int mustCopy = 0, matchlen = matchlens[i];

		while (longBits < maxLong && (i - shortLen - midLen) + bulkLen > (1 << (longBits)))
		{
			longBits++;
			printf("At >=%04x, %d bits will do!\n", i, longBits);
		}

		// If we are done with the last match, and time to start a new one, emit it
		if (i >= matchend && matchlen)
		{
			matchend = matchlen + i;	// Next end of match
			if (dists[i] == -1)			// If it's a lit pool match, just do it now
			{
				PutCode(src, i, distance, 3, PRELONG, longBits, matchlen-4 + 128);
				while (matchlen--)
					PutCode(src, i, distance, 0, PRELIT, 16, src[i++]);
				i--;					// Point to the next word after the copy
				continue;				// And go on
			}
			else if (distance != dists[i])		// If it's not redundant, output it
			{
				// Set the new distance and output the correct length code
				distance = dists[i];
				if (distance < shortLen)
					PutCode(src, i, distance, 3, PRESHORT, shortBits, distance);
				else if (distance < shortLen + midLen)
					PutCode(src, i, distance, 3, PREMID, midBits, distance - shortLen);
				else
					PutCode(src, i, distance, 3, PRELONG, longBits, distance - shortLen - midLen + bulkLen);
				mustCopy = 1;				// Must emit copy as next word
			}
		}
		// Set up source pointer and dest pointer
		u16 *pk = src+i-distance-1, *pi = src+i;
		int deltalen = (*pi ^ *pk) - 1;

		// If the source is a branch, try to copy it.
		if (0xf000 == (pk[0] & 0xf800) && 0xf800 == (pk[1] & 0xf800))
		{
			// When source match is branch, encoder guarantees decoder
			// can/must perform the inverse of this math
			int offi = ((pi[0] & 0x7ff)<<11) + (pi[1] & 0x7ff);	// Decode Thumb BX format
			int offk = ((pk[0] & 0x7ff)<<11) + (pk[1] & 0x7ff);
			offi = ((offi<<10)>>10) + (pi-src);
			offk = ((offk<<10)>>10) + (pk-src);		
			// If it is a valid match, try to copy it
			if (0xf000 == (pi[0] & 0xf800) && 0xf800 == (pi[1] & 0xf800))
			{
				// Easiest way, just copy it
				int howfar = offi - offk;
				if (0 == howfar) {
					if (mustCopy)
						PutCode(src, i, distance, 1, PRECOPY>>1, 0, 0);
					else
						PutCode(src, i, distance, 2, PRECOPY, 0, 0);
					i++;			// Skip next word after branch..
					continue;		// and go on
				} else if (howfar < 128 && howfar >= -128) {
					if (mustCopy)
						PutCode(src, i, distance, 2, PREDELTA>>1, 8, howfar);
					else
						PutCode(src, i, distance, 3, PREDELTA, 8, howfar);
					i++;			// Skip next word after branch..
					continue;		// and go on
				} else if (offi >= 0 && offi <= 32767) {
					if (mustCopy)
						PutCode(src, i, distance, 2, PREDELTABIG>>1, 15, offi);
					else
						PutCode(src, i, distance, 3, PREDELTABIG, 15, offi);
					i++;			// Skip next word after branch..
					continue;		// and go on
				}
			}

			// Can't safely copy from branch source, must handle as a literal
			if (mustCopy)
				bye("First and second pass lost sync on bad branch!\n");
			PutCode(src, i, distance, 3, PRELIT, 16, *pi);

		// If the source is not a branch, try a simple copy
		} else if (*pi == *pk) {
			if (mustCopy)
				PutCode(src, i, distance, 1, PRECOPY>>1, 0, 0);
			else
				PutCode(src, i, distance, 2, PRECOPY, 0, 0);
		// Not a branch, how about a delta copy?
		// Max encodable delta is 511+16 (9-bits + 4-bits)
		} else if (deltalen >= 0 && deltalen <= (511+16)
			&& !(0xf000 == (pk[0] & 0xf800) && 0xf800 == (pk[1] & 0xf800))	// Not a branch
			&& !(0xf000 == (pk[0] & 0xf800) && distance < 1))	// Nor branch look-alike (next source is still 0xffff/unset)
		{
			if (deltalen < 16)
			{
				if (mustCopy)
					PutCode(src, i, distance, 2, PREDELTA>>1, 4, deltalen);
				else
					PutCode(src, i, distance, 3, PREDELTA, 4, deltalen);
			} else {
				if (mustCopy)
					PutCode(src, i, distance, 2, PREDELTABIG>>1, 9, deltalen-16);
				else
					PutCode(src, i, distance, 3, PREDELTABIG, 9, deltalen-16);
			}
		// None of the above, write out the literal bits
		} else {
			if (mustCopy)
				bye("First and second pass lost sync!\n");
			else
				PutCode(src, i, distance, 3, PRELIT, 16, *pi);
		}
	}
	PutCode(NULL, 0, 0, 0, 0, 0, 0);	// Flush any latent copies
	PutBits(7, -1);						// Flush bit buffer

	//printf("%d: %02d/%02d: %d expected, %d actual\n", (int)(m_putp-m_outbits), shortBits, midBits, (costs[0]+7)/8, (int)(m_putp-m_outbits));	
}

// This stub mimics the embedded/flash version of the decruncher
static u16* m_destp = m_dest + 2;   // Skip past canary
static void Output(u16 word)
{
	*m_destp++ = word;
}

static int m_head = sizeof(ota_header);
CryptoContext m_ctx;
int m_destlen;

// This stub mimics the embedded/flash version of the decruncher
static u8 GetByte()
{
  // Feed 1s after end of source
  if (m_head >= m_destlen)    // This is srclen in car
    return -1;

  // Decrypt next 4 bytes in-place
  if (0 == (m_head & 3))
    opensafeDecrypt(&m_ctx, (u32p)(m_crypted + m_head), 4);
	return m_crypted[m_head++];
}

// Read the next N bits (up to 16) from the bit buffer
static int GetBits(int n)
{
	// Prefill the accumulator with new bits
	static u32 s_accum = 0, s_head = 0;
	while (s_head < 24)
	{
		s_accum |= (GetByte() << s_head);
		s_head += 8;
	}

	// Drain out the requested bits (LSB first)
	int ret = s_accum & ((1<<n)-1);
	s_accum >>= n;
	s_head -= n;
	return ret;
}

// Decompress the bits into the destination until 'len' words are written
// This code is optimized for size - it must fit in under 500 bytes on Thumb
void Decrunch(int len)
{
	// Continue parsing until EOF is reached
	int distance = 1;	// Encoder makes this assumption
	while (m_destp < m_dest + len)
	{
		int bulkcount = 0;
		// If it's a complex code, parse it first
		if (GetBits(1))
			switch (GetBits(2) & 3)
			{
			case PRELIT >> 1:		// Literals are simply copied verbatim
				Output(GetBits(16));
				continue;	// Next prefix
			case PRESHORT >> 1:		// Short distance
				distance = GetBits(shortBits) + 1;
				break;		// Fall through to copy logic
			case PREMID >> 1:		// Mid distance
				distance = GetBits(midBits) + shortLen + 1;
				break;		// Fall through to copy logic
			case PRELONG >> 1:		// Long distance or bulk operator
				int count = GetBits(longBits);
				if (count < bulkLen) {	// Bulk operator?
					if (count >= 128) {		// Yes, copy bulk lits
						while (count-- > 128-4)	
							Output(GetBits(16));
						continue;			// Next token
					}
					bulkcount = count + 5;	// Bulk simple copy
				} else				// Long distance
					distance = count + shortLen + midLen + 1 - bulkLen;
				break;		// Fall through to copy logic
			}

		// Is there a delta code?
		int hasDelta = 0;
		if (!bulkcount && GetBits(1))	// Bulk copies can't have a delta code
			hasDelta = 1;

		// Handle each word of the copy, potentially a bulk copy
		do {
			u16* src = m_destp - distance;
			if (0xf000 == (src[0] & 0xf800) && 0xf800 == (src[1] & 0xf800)) {
				// Parse source opcode and insert a relocated branch
				int target = ((src[0] & 0x7ff) << 11) + (src[1] & 0x7ff) - distance;
				if (hasDelta)
					if (GetBits(1))
						target = GetBits(15) - (m_destp - m_dest);	// Long delta is an absolute offset
					else
						target += ((GetBits(8)<<24)>>24);			// Short delta is an 8-bit (sign extended) offset
				// Output relocated branch opcode
				Output(0xf000 | ((target >> 11) & 0x7ff));
				Output(0xf800 | (target & 0x7fff));

			// Otherwise, just copy the word
			} else {
				if (hasDelta)		// Copy the word plus a variable length delta
					if (GetBits(1))
						Output(*src ^ (GetBits(9) + 17));
					else
						Output(*src ^ (GetBits(4) + 1));
				else
					Output(*src);	// Just copy
			}
		} while (bulkcount--);
	}
}

int TestCrunch(char* dest, unsigned short* src, int srclen, int guid)
{
  // Make fresh copies of everything, with 0xff as a canary to find overflows
  memset(src, 0xff, 4);                         // First four bytes should be 0xff
  memset(m_outbits, 0xff, sizeof(m_outbits));   // Leftover bits should be all 1s
  memset(m_src, 0xff, sizeof(m_src));
  memset(m_dest, 0xff, sizeof(m_dest));
  memcpy(m_src, src, srclen*2);

  // Crunch and encrypt the input
  Crunch(m_src, srclen);
  // Round up to nearest 20-byte boundary, since OTA files are sent that way
  m_destlen = (((m_putp-m_outbits)+sizeof(ota_header)+19) / 20) * 20;

  // Fill in the header/nonce
  ota_header *head = (ota_header*)m_crypted;
  head->tag = OTA_PLATFORM_TAG;
  head->srclen = m_destlen;                 // src and dest depend on your perspective..
  head->destlen = srclen;                   // In this header, they're from the decoder perspective
  head->guid = guid;
  head->millis = 0;
  head->who = 'x';
  head->version = 1;

  // Encrypt
  CryptoContext ctx;
  CryptoSetupNonce(&ctx, (u08p)head);          // The nonce is the first 16-bytes of the header
  CryptoEncryptBytes(&ctx, (u08p)m_outbits, (u08p)(head + 1), m_destlen - sizeof(ota_header));
  CryptoFinalize(&ctx, (u08p)head->mac);       // Fill in MAC

	// Copy the bits back to the destination
	memcpy(dest, m_crypted, m_destlen);

  opensafeSetupNonce(&m_ctx, (c32p)head);
	Decrunch(srclen);

  // XXX: Bootloader misbehavior - the encrypted length for MAC purposes, is dependent on Decrunch m_head, not m_destlen
  // So, we just recompute the MAC here using m_head length - if you change Decrunch in bootloader, fix this!
  CryptoSetupNonce(&ctx, (u08p)head);          // The nonce is the first 16-bytes of the header
  int maclen = ((m_head+3 - sizeof(ota_header)) & (~3));
  CryptoEncryptBytes(&ctx, (u08p)m_outbits, (u08p)(head + 1), maclen);
  CryptoFinalize(&ctx, (u08p)head->mac);       // Re-fill MAC
	memcpy(dest, m_crypted, m_destlen);          // Re-copy bits
  if (0 != opensafeCheckMac(&m_ctx, head->mac))
    bye("Programmer error: MAC mismatch in OTA file, .OTA file unusable!\n");

	// Binary compare
	int match = 1;
	for (int i = 0; i < srclen; i++)
		if (m_dest[i] != m_src[i])
		{
			printf("%04x: %04x != %04x\n", i, m_dest[i], m_src[i]);
			match = 0;
		}
  int safelen = ((srclen*2+2047)/2048) * 2080;
	if (match)
		printf("OTA: Crunch/decrunch OK, compressed to %d/%d bytes, %03.2f%%\n",
			m_destlen, safelen, m_destlen*100.0/(float)safelen);
	else
		bye("OTA: Programmer error: Decrunched file doesn't match!\n");

	return m_destlen;
}

// Unit test for PutBits/GetBits
void TestBits()
{
	for (int i = 0; i < 20; i++)
	{
		PutBits(3, 6);
		PutBits(2, i);
		PutBits(15, 1234);
		PutBits(1, 0);
		PutBits(1, 1);
		PutBits(1, 0);
	}
	PutBits(7, -1);
	for (int i = 0; i < 20; i++)
	{
		printf("%d ", GetBits(3));
		printf("%d ", GetBits(2));
		printf("%d ", GetBits(15));
		printf("%d ", GetBits(1));
		printf("%d ", GetBits(1));
		printf("%d\n", GetBits(1));
	}
}
