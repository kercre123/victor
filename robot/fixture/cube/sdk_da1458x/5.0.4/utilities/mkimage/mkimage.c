/**
 ****************************************************************************************
 *
 * @file mkimage.c
 *
 * @brief Utility for creating a firmware image for DA14580.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
#define _XOPEN_SOURCE 700
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef __linux__
#include <endian.h>
#endif
#include <string.h>
#include <errno.h>
#include <time.h>

#include "image.h"
#include "sw_aes.h"

#ifdef _MSC_VER
#  define RW_RET_TYPE	int
#  define snprintf	_snprintf
#  define S_IRUSR	S_IREAD
#  define S_IWUSR	S_IWRITE
#else
#  define RW_RET_TYPE	ssize_t
#endif

static AES_CTX aes_ctx;
/* pre-determined cryptography key and IV */
static uint8_t def_key[16] = {
	0x06, 0xa9, 0x21, 0x40, 0x36, 0xb8, 0xa1, 0x5b,
	0x51, 0x2e, 0x03, 0xd5, 0x34, 0x12, 0x00, 0x06
};
static uint8_t def_iv[16] = {
	0x3d, 0xaf, 0xba, 0x42, 0x9d, 0x9e, 0xb4, 0x30,
	0xb4, 0x22, 0xda, 0x80, 0x2c, 0x9f, 0xac, 0x41
};


#define MKIMAGE_VERSION "1.10"

/* uncomment to store multi-byte values in little-endian order */
#define MKIMAGE_LITTLE_ENDIAN


static void usage(const char* my_name)
{
	fprintf(stderr,
		"Version: " MKIMAGE_VERSION "\n"
		"\n"
		"Usage case #1:\n"
		"  %s single in_file version_file out_file [enc [key iv]]\n"
		"\n"
		"  Prepend header to raw binary image 'in_file' (e.g. .bin file).\n"
		"  The header contains versioning, timestamping and housekeeping\n"
		"  information, which is partially extracted from header file\n"
		"  'version_file'. The resulting image (e.g. .img file) is\n"
		"  written to 'out_file'.\n"
		"  Ecryption of raw binary image 'in_file' may be enabled by\n"
		"  providing the enc option at the end of the command.\n"
		"  The user may provide the encryption key ('key') and\n"
		"  initialisation vector ('iv'), as a string of 32 hex\n"
		"  characters (without any prefix). If no key and IV are\n"
		"  given, the default values will be used.\n"
		"\n"
		"  The 'version_file' is normaly stored at\n"
		"  sdk/platform/include/ble_580_sw_version.h and this\n"
		"  program looks in it for definitions like below:\n"
		"\t#define DA14580_SW_VERSION \"v_3.0.3.221\"\n"
		"\t#define DA14580_SW_VERSION_DATE \"2014-06-30 17:56 \"\n"
		"\n"
		"\n"
		"Usage case #2:\n"
		"  %s multi spi|eeprom [bloader] in_img1 off1 in_img2 off2 off3 [cfg off4[,bdaddr]] out_file\n"
		"\n"
		"  Create a multi-part image 'out_file' for SPI flash (spi) or\n"
		"  for EEPROM (eeprom).\n"
		"  The multi-part image consists of:\n"
		"   * AN-B-001 header + 'bloader' at offset 0, if 'bloader' is provided\n"
		"   * 'img1' (.img image)         at offset 'off1'\n"
		"   * 'img2' (.img image)         at offset 'off2'\n"
		"   * product header              at offset 'off3'\n"
		"\n"
		"  The offsets can be given either as decimal or hex numbers.\n"
		"  The BD address 'bdaddr' can be given as XX:XX:XX:XX:XX:XX where X is a hex digit. E.g. 80:EA:CA:01:02:03.\n"
		"\n"

		" Product header field configuration:\n"
		"   * 'Configuration Offset' is initialized from 'off4'. If 'off4' is not provided then it is set to 0xFFFFFFFF.\n"
		"   * 'BD Address'           is initialized from 'bdaddr'. If 'bdaddr' is not provided then it is set to FF:FF:FF:FF:FF:FF.\n"
		"\n",

		my_name, my_name);
}

#ifdef _MSC_VER
__inline static void store32(uint8_t* buf, uint32_t val)
#else
static inline void store32(uint8_t* buf, uint32_t val)
#endif
{
#ifdef MKIMAGE_LITTLE_ENDIAN
	buf[0] = val & 0xff;
	val >>= 8;
	buf[1] = val & 0xff;
	val >>= 8;
	buf[2] = val & 0xff;
	val >>= 8;
	buf[3] = val & 0xff;
#else
	buf[3] = val & 0xff;
	val >>= 8;
	buf[2] = val & 0xff;
	val >>= 8;
	buf[1] = val & 0xff;
	val >>= 8;
	buf[0] = val & 0xff;
#endif
}

#ifdef _MSC_VER
__inline static void store_crc(struct image_header* hdr, uint32_t crc)
#else
static inline void store_crc(struct image_header* hdr, uint32_t crc)
#endif
{
	uint32_t val = crc;
	store32(hdr->CRC, val);
}


static int safe_write(int fd, const void* buf, size_t len)
{
	RW_RET_TYPE n;
	const uint8_t* _buf = buf;

	do {
		n = write(fd, _buf, len);
		if (n > 0) {
			len -= n;
			_buf += n;
		} else if (n < 0  &&  errno != EINTR)
			return -1;
	} while (len);

	return 0;
}


static int safe_read(int fd, void* buf, size_t len)
{
	RW_RET_TYPE n;
	uint8_t* _buf = buf;

	do {
		n = read(fd, _buf, len);
		if (n > 0) {
			len -= n;
			_buf += n;
		} else if (n == 0) {
			return len;  /* EOF: return number of bytes missing */
		} else if (n < 0  &&  errno != EINTR) {
			return -1;
		}
	} while (len);

	return 0;
}


static int set_active_image(int img,unsigned char active){
	struct image_header hdr;
	if (safe_read(img,&hdr,sizeof(struct image_header))<0)
		return -1;
	hdr.image_id = active;
	
	if (lseek(img, 0, SEEK_SET)<0)
		return -1;
	if (safe_write(img,&hdr,sizeof(struct image_header))<0)
		return -1;
	if (lseek(img, 0, SEEK_SET)<0)
		return -1;
	return 0;
}



/*
 * Look for a C string (i.e. string enclosed in "") inside text
 * NOTE: \" is not handled as escaped double quote
 *
 * Return a strdup'ed string; the caller SHOULD free it.
 */
char* find_cstring(const char* text)
{
	char* from, * to, * s;

	from = strchr(text, '"');
	if (!from)
		return NULL;

	to = strchr(++from, '"');
	if (!to)
		return NULL;

#ifdef __linux__
	s = strndup(from, to - from);
#else
	s = strdup(from);
	if (strlen(s)  > (unsigned) (to - from))
		s[to - from] = '\0';
#endif

	return s;
}


static void write_img_version(struct image_header* hdr, const char* s)
{
	/* skip any leading v_ */
	if ('v' == s[0]  &&  '_' == s[1])
		s += 2;
	snprintf((char*)hdr->version, sizeof hdr->version, "%s", s);
}

static void write_img_date(struct image_header* hdr, const char* s)
{
	struct tm tm;
	time_t secs;

	memset(&tm, 0, sizeof tm);
	tm.tm_isdst = -1;
#ifdef __linux__
	strptime(s, "%Y-%m-%d %H:%M", &tm);
#else
	sscanf(s, "%d-%d-%d %d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min);
	tm.tm_year -= 1900;
	tm.tm_mon--;
#endif
	secs = mktime(&tm);
	if (-1 == secs)
		perror("mktime");
	store32(hdr->timestamp, (uint32_t)secs);
}


/*
 * Parse f line by line and try to find the following patterns
 *
 * #define DA14580_SW_VERSION "v_3.0.3.221"
 * #define DA14580_SW_VERSION_DATE "2014-06-30 17:56 "
 *
 * We are interested in the two strings.
 */
static int parse_version_file(FILE* f, struct image_header* hdr)
{
	enum patterns {
		IMG_VERSION = 1,
		IMG_DATE = 2
	};
	unsigned patterns_left = IMG_VERSION | IMG_DATE;

	do {
		char* from, * s;
#ifdef __linux__
		size_t n, len;
		char* line;
#else
		char line[100];
#endif

#ifdef __linux__
		line = NULL;
		len = 0;
		n = getline(&line, &len, f);
		if (n < 0) {
			n = errno;
			free(line);
			return -n;
		}
#else
		if (fgets(line, sizeof line, f) == NULL) {
			return -errno;
		}
#endif

		if (patterns_left & IMG_VERSION) {
			from = strstr(line, "DA14580_SW_VERSION ");
			if (from) {
				s = find_cstring(from);
				if (s)
					patterns_left &= ~IMG_VERSION;
				write_img_version(hdr, s);
				free(s);
			}
		}
		if (patterns_left & IMG_DATE) {
			from = strstr(line, "DA14580_SW_VERSION_DATE ");
			if (from) {
				s = find_cstring(from);
				if (s)
					patterns_left &= ~IMG_DATE;
				write_img_date(hdr, s);
				free(s);
			}
		}

#ifdef __linux__
		free(line);
#endif
	} while (patterns_left);

	return 0;
}


static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*
 * Encryption is done in blocks of AES_BLOCKSIZE bytes.
 * To keep things simple we also maintain AES_BLOCKSIZE buffers.
 */
static uint8_t copy_buf_clr[AES_BLOCKSIZE];
static uint8_t copy_buf_enc[AES_BLOCKSIZE];
static int append_file_crc32(int outf, int inf, uint32_t* _crc32, int encrypt)
{
	RW_RET_TYPE n;
	uint32_t crc32 = ~0;
	const uint8_t* buf;

	do {
		size_t count;
		int size;
		const uint8_t* data;

		memset(copy_buf_clr, 0, sizeof copy_buf_clr);
		memset(copy_buf_enc, 0, sizeof copy_buf_enc);
		count = sizeof copy_buf_clr;
		n = safe_read(inf, copy_buf_clr, count);
		if (n < 0) {
			perror("reading input image");
			return -1;
		}
		/* convert from bytes missing to bytes read */
		n = count - n;
		if (n == 0)
			break;  /* EOF */

		if (encrypt) {
			AES_cbc_encrypt(&aes_ctx, copy_buf_clr,
					copy_buf_enc, count);
			buf = copy_buf_enc;
			size = count;
		} else {
			buf = copy_buf_clr;
			size = n;
		}
		if (safe_write(outf, buf, size)) {
			perror("writing image");
			return -1;
		}

		data = copy_buf_clr;
		while (size--) {
			crc32 = crc32_tab[(crc32 ^ *data++) & 0xff]
				^ (crc32 >> 8);
		}
	} while (n != 0);

	*_crc32 = crc32 ^ ~0;

	return 0;
}

static int append_file_csum(int outf, int inf, uint8_t* _csum)
{
	RW_RET_TYPE n;
	uint8_t csum = 0;

	do {
		size_t count;
		int size;
		const uint8_t* data;

		count = sizeof copy_buf_clr;
		n = read(inf, copy_buf_clr, count);
		if (n < 0) {
			if (EINTR == errno)
				continue;
			else {
				perror("Error while reading input image");
				return -1;
			}
		}
		if (safe_write(outf, copy_buf_clr, n)) {
			perror("writing image");
			return -1;
		}

		size = n;
		data = copy_buf_clr;
		while (size--)
			csum ^= *data++;
	} while (n != 0);

	if (_csum != NULL)
		*_csum = csum;

	return 0;
}

static int parse_hex_string(const char s[], uint8_t buf[], const int len)
{
	int s_i, buf_i;
	char digit[3];
	unsigned long val;
	char* end_ptr;

	digit[2] = '\0';
	for (s_i = buf_i = 0; buf_i < len; s_i += 2, buf_i++) {
		digit[0] = s[s_i];
		digit[1] = s[s_i + 1];
		val = strtol(digit, &end_ptr, 16);
		if (*end_ptr)
			return -1;  /* should be non-NIL */
		buf[buf_i] = (uint8_t)val;
	}

	return 0;
}

static int create_single_image(int argc, const char* argv[])
{
	int inf = -1, outf = -1;
	FILE* verf = NULL;
	int oflags, n, res = EXIT_FAILURE;
	uint32_t crc32;
	struct stat sbuf;
	struct image_header hdr;
	int encrypt = 0;
	off_t size;

	if (argc != 5  &&  argc != 6  && argc != 8 ) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (argc > 5) {
		uint8_t user_key[16];
		uint8_t user_iv[16];
		const uint8_t* key;
		const uint8_t* iv;

		if (strcmp(argv[5], "enc")) {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
		encrypt = 1;
		if (argc == 6) {
			key = def_key;
			iv = def_iv;
		} else {
			if (strlen(argv[6]) != 32  ||
					strlen(argv[7]) != 32) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			if (parse_hex_string(argv[6], user_key,
					sizeof(user_key))) {
				fprintf(stderr, "Invalid key\n");
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			if (parse_hex_string(argv[7], user_iv,
					sizeof(user_iv))) {
				fprintf(stderr, "Invalid iv\n");
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			key = user_key;
			iv = user_iv;
		}
		AES_set_key(&aes_ctx, key, iv, AES_MODE_128);
	}

	memset(&hdr, 0xff, sizeof hdr);
	hdr.signature[0] = 0x70;
	hdr.signature[1] = 0x51;
	hdr.valid_flag = 0xaa;
	hdr.image_id = 0xff;
	hdr.flags = 0;

	/* open input file and get its size */
	oflags = O_RDONLY;
#ifdef O_BINARY
	oflags |= O_BINARY;
#endif
	inf = open(argv[2], oflags);
	if (-1 == inf) {
		perror(argv[2]);
		return EXIT_FAILURE;
	}
	if (fstat(inf, &sbuf)) {
		perror(argv[2]);
		goto cleanup_and_exit;
	}
	size = sbuf.st_size;
	if (encrypt) {
		hdr.flags |= IMG_ENCRYPTED;
		if ( size % AES_BLOCKSIZE){
			size += AES_BLOCKSIZE - (size % AES_BLOCKSIZE) ;
		}
	}
	store32(hdr.code_size, size);

	/* open the version file and read version and timestamp */
	verf = fopen(argv[3], "r");
	if (NULL == verf) {
		perror(argv[3]);
		goto cleanup_and_exit;
	}
	n = parse_version_file(verf, &hdr);
	if (n < 0) {
		fprintf(stderr, "Could not parse %s: %s\n",
				argv[3], strerror(-n) );
		goto cleanup_and_exit;
	}

	/* open the output file */
	oflags = O_RDWR | O_CREAT | O_TRUNC;
#ifdef O_BINARY
	oflags |= O_BINARY;
#endif
	outf = open(argv[4], oflags, S_IRUSR | S_IWUSR);
	if (-1 == outf) {
		perror(argv[4]);
		goto cleanup_and_exit;
	}

	/* write current header, we'll update it before exiting */
	if (safe_write(outf, &hdr, sizeof hdr)) {
		perror(argv[4]);
		goto cleanup_and_exit;
	}

	if( append_file_crc32(outf, inf, &crc32, encrypt) )
		goto cleanup_and_exit;
	store_crc(&hdr, crc32);

	/* write the header again, to set the checksum */
	if (lseek(outf, 0, SEEK_SET) == -1) {
		perror(argv[4]);
		goto cleanup_and_exit;
	}
	if (safe_write(outf, &hdr, sizeof hdr)) {
		perror(argv[4]);
		goto cleanup_and_exit;
	}

	res = EXIT_SUCCESS;

cleanup_and_exit:
	if (outf != -1) {
		if (close(outf))
			perror(argv[4]);
	}

	if (verf != NULL) {
		if (fclose(verf))
			perror(argv[3]);
	}

	if (inf != -1) {
		if (close(inf))
			perror(argv[2]);
	}

	return res ;
}


static int parse_offset(const char* s, unsigned* offset)
{
	long val;
	char* end_ptr;

	/* define a "rational" upper limit of the multi-part image size */
#ifndef MULTI_IMAGE_LIMIT
#  define MULTI_IMAGE_LIMIT	0x100000
#endif

	val = strtol(s, &end_ptr, 0);
	if (end_ptr == s) {
		fprintf(stderr, "Invalid offset '%s'.\n", s);
		return -1;
	}

	if (val > MULTI_IMAGE_LIMIT) {
		fprintf(stderr, "Offset '%ld' is suspiciously high and is "
				"rejected.\n"
				"If you really need such an offset, set "
				"MULTI_IMAGE_LIMIT appropriately.\n", val);
		return -1;
	}

	*offset = val;

	return 0;
}

/**
 ****************************************************************************************
 * @brief Parses a BD address.
 *
 * @param[in]   s           String containing a BD address formated as XX:XX:XX:XX:XX:XX
 *                          where X is a HEX digit.
 * @param[out]  out_bd_addr Output array containing the 6 bytes of the BD address.
 *                          Lest significant byte comes first.
 *
 * @return int  0 on success,
 ****************************************************************************************
 */
static int parse_bd_addr(const char* s, unsigned char* out_bd_addr)
{
    int rc;
    uint32_t bdaddr[6];

    rc = sscanf(s, "%02X:%02X:%02X:%02X:%02X:%02X" ,  &bdaddr[5], &bdaddr[4], &bdaddr[3], &bdaddr[2], &bdaddr[1], &bdaddr[0] );

    if (rc != 6) {
        return EXIT_FAILURE;
    }

    out_bd_addr[0] = bdaddr[0] &0xFF;
    out_bd_addr[1] = bdaddr[1] &0xFF;
    out_bd_addr[2] = bdaddr[2] &0xFF;
    out_bd_addr[3] = bdaddr[3] &0xFF;
    out_bd_addr[4] = bdaddr[4] &0xFF;
    out_bd_addr[5] = bdaddr[5] &0xFF;

    return EXIT_SUCCESS;
}

/**
 ****************************************************************************************
 * @brief Parses the argument list of the cfg option
 *
 * @param[in]   s           String containing a comma separated argument list
 * @param[out]  cfg_offset  Accepts application specific configuration offset value.
 * @param[out]  bd_addr     Accepts the 6 byte BD address value (least significant byte first).
 *
 * @return int  0 on success,
 ****************************************************************************************
 */
static int parse_prod_header_cfg(const char* s, unsigned* cfg_offset, unsigned char* bd_addr)
{
    const char DELIMETER[] = ",";
    char *buffer = NULL;
    char *token = NULL;
    int rc = EXIT_SUCCESS; // return code

    // copy parameter string s to a dynamically allocated buffer
    buffer = malloc(strlen(s));
    memcpy(buffer, s, strlen(s));

    //
    // 1st Configuration Value: application specific configuration offset
    //
    token = strtok(buffer, DELIMETER); // get 1st token

    if (!token) {
        rc = EXIT_FAILURE; // failure if not found
        goto cleanup_and_return;
    }

    if (parse_offset(token , cfg_offset)) {
        fprintf(stderr, "The parameter list of the cfg option contains an "
                        "invalid configuration offset value: '%s' \n", token);
        rc = EXIT_FAILURE;
        goto cleanup_and_return;
    }

    //
    // 2nd Configuration Value: BD address (optional)
    //
    token = strtok(NULL, DELIMETER); // get next token

    if (!token) {
        rc = EXIT_SUCCESS; // it is optional
        goto cleanup_and_return;
    }

    if (parse_bd_addr(token, bd_addr)) {
        fprintf(stderr, "The parameter list of the cfg option contains an"
                        "invalid BD address value: '%s' \n", token);
        rc = EXIT_FAILURE;
        goto cleanup_and_return;
    }

    //
    // No more values are expected
    //
    token = strtok(NULL, DELIMETER); // get next token

    // if another value exists then it is an error.
    if (token) {
        fprintf(stderr, "The parameter list of the cfg option must contain "
                        "2 values at most.\n");
        rc = EXIT_FAILURE;
        goto cleanup_and_return;
    }

cleanup_and_return:
    free(buffer);

    return rc;
}

static int add_padding(int outf, const unsigned count, const uint8_t pad)
{
	unsigned i;
    #ifdef _MSC_VER
	size_t n;
	#else
	ssize_t n;
    #endif

	for (i = 0; i < count; i++) {
		do {
			n = write(outf, &pad, 1);
			if (n < 0  &&  errno != EINTR)
				return -1;
		} while (n < 1);
	}

	return 0;
}



static int create_multi_image(int argc, const char* argv[])
{
	enum multi_options {
		SPI = 1,
		EEPROM = 2,
		BOOTLOADER = 4,
		CONFIG = 8
	};
	unsigned options = 0;
	int oflags, arg_base = 3, arg_off = 0, res = EXIT_FAILURE;
	int outf = -1, bloader = -1, img1 = -1, img2 = -1;
	unsigned off1, off2, off3, cfg_off;
	off_t offset;
	struct an_b_001_spi_header spi_hdr;
	struct an_b_001_i2c_header i2c_hdr;
	struct product_header p_hdr;
	struct stat sbuf;
	uint8_t pad_byte;
	unsigned char bd_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	/* determine if
	 *  - bootloader image is given
	 *  - configuration offset is defined
	 */
	if (9 == argc) {
		options &= ~(BOOTLOADER | CONFIG);
	} else if (10 == argc) {
		options |= BOOTLOADER;
		arg_base++;
	} else if (11 == argc) {
		options |= CONFIG;
		arg_off += 2;
	} else if (12 == argc) {
		options |= BOOTLOADER | CONFIG;
		arg_base++;
		arg_off += 2;
	} else {
		fprintf(stderr, "Invalid number of arguments.\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* determine type of multi-part image */
	if (!strcmp(argv[2], "spi"))
		options |= SPI;
	else if (!strcmp(argv[2], "eeprom"))
		options |= EEPROM;
	else {
		fprintf(stderr, "Unkwown multi-part image type '%s'.\n",
				argv[2]);
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (options & SPI)
		pad_byte = 0xff;
	else
		pad_byte = 0xff;

	/* parse offsets */
	if (parse_offset(argv[arg_base + 1], &off1))
		return EXIT_FAILURE;
	if (parse_offset(argv[arg_base + 3], &off2))
		return EXIT_FAILURE;
	if (parse_offset(argv[arg_base + 4], &off3))
		return EXIT_FAILURE;
	if (!(off1 < off2  &&  off2 < off3)) {
		fprintf(stderr, "Inconsistent offsets 'off1'=%u, 'off2'=%u, "
				"'off3'=%u\n", off1, off2, off3);
		return EXIT_FAILURE;
	}

	/* parse product header configuration values */
	if (options & CONFIG) {
		if (strcmp(argv[arg_base + 5], "cfg")) {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
		if (parse_prod_header_cfg(argv[arg_base + 6], &cfg_off, bd_addr)) {
			return EXIT_FAILURE;
		}
	} else {
		cfg_off = 0xffffffff;
	}


	/* open the input files */
	oflags = O_RDWR;
#ifdef O_BINARY
	oflags |= O_BINARY;
#endif

	if (options & BOOTLOADER) {
		bloader = open(argv[3], oflags);
		if (-1 == bloader) {
			perror(argv[3]);
			goto cleanup_and_exit;
		}
	}

	img1 = open(argv[arg_base], oflags);
	if (-1 == img1) {
		perror(argv[arg_base]);
		goto cleanup_and_exit;
	}

	img2 = open(argv[arg_base + 2], oflags);
	if (-1 == img2) {
		perror(argv[arg_base + 2]);
		goto cleanup_and_exit;
	}

	/* open the output file */
	oflags = O_RDWR | O_CREAT | O_TRUNC;
#ifdef O_BINARY
	oflags |= O_BINARY;
#endif
	outf = open(argv[arg_base + 5 + arg_off], oflags, S_IRUSR | S_IWUSR);
	if (-1 == outf) {
		perror(argv[arg_base + 5]);
		goto cleanup_and_exit;
	}

	printf("Creating image '%s'...\n", argv[arg_base + 5]);
	if (options & BOOTLOADER) {
		off_t bloader_size;
		uint8_t csum;

		/*
		 * build AN-B-001 header
		 * we write it here, but it may be rewritten later on
		 */
		if (fstat(bloader, &sbuf)) {
			perror(argv[3]);
			goto cleanup_and_exit;
		}
		bloader_size = sbuf.st_size;
		if (options & SPI) {
			spi_hdr.preamble[0] = 0x70;
			spi_hdr.preamble[1] = 0x50;
			memset(spi_hdr.empty, 0, sizeof spi_hdr.empty);
			spi_hdr.len[0] = (uint8_t)((bloader_size & 0xff00) >> 8);
			spi_hdr.len[1] = (uint8_t)((bloader_size & 0xff));
			if (safe_write(outf, &spi_hdr, sizeof spi_hdr)) {
				perror(argv[arg_base + 5]);
				goto cleanup_and_exit;
			}
			printf("[%08x] AN-B-001 SPI header\n", 0);
		} else if (options & EEPROM) {
			i2c_hdr.preamble[0] = 0x70;
			i2c_hdr.preamble[1] = 0x50;
			i2c_hdr.len[0] = (uint8_t)((bloader_size & 0xff00) >> 8);
			i2c_hdr.len[1] = (uint8_t)((bloader_size & 0xff));
			memset(i2c_hdr.dummy, 0, sizeof i2c_hdr.dummy);
			if (safe_write(outf, &i2c_hdr, sizeof i2c_hdr)) {
				perror(argv[arg_base + 5]);
				goto cleanup_and_exit;
			}
			printf("[%08x] AN-B-001 I2C header\n", 0);
		}
		offset = lseek(outf, 0, SEEK_CUR);
		if (-1 == offset) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
		if (append_file_csum(outf, bloader, &csum))
			goto cleanup_and_exit;
		printf("[%08x] Bootloader\n", (unsigned)offset);
		if (options & EEPROM)
			i2c_hdr.crc = csum;  /* we'll re-write the header */
	}

	/* now place img1 at offset off1 */
	offset = lseek(outf, 0, SEEK_CUR);
	if (-1 == offset) {
		perror(argv[arg_base + 5]);
		goto cleanup_and_exit;
	}
	if ((unsigned)offset > off1) {
		fprintf(stderr, "'off1'=%s is too low.\n", argv[arg_base + 1]);
		goto cleanup_and_exit;
	}
	if (off1 > (unsigned)offset) {
		if (add_padding(outf, off1 - offset, pad_byte)) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
		printf("[%08x] Padding (%02X's)\n", (unsigned)offset, pad_byte);
	}

	if (set_active_image(img1,0x01)<0){
			goto cleanup_and_exit;
	}
	
	

	if (append_file_csum(outf, img1, NULL))
		goto cleanup_and_exit;
	printf("[%08x] '%s'\n", off1, argv[arg_base]);

	/* then goes img2 at offset off2 */
	offset = lseek(outf, 0, SEEK_CUR);
	if (-1 == offset) {
		perror(argv[arg_base + 5]);
		goto cleanup_and_exit;
	}
	if ((unsigned)offset > off2) {
		fprintf(stderr, "'off2'=%s is too low.\n", argv[arg_base + 3]);
		goto cleanup_and_exit;
	}
	if (off2 >(unsigned) offset) {
		if (add_padding(outf, off2 - offset, pad_byte)) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
		printf("[%08x] Padding (%02X's)\n", (unsigned)offset, pad_byte);
	}
	
	if (set_active_image(img2,0x00)<0){
			goto cleanup_and_exit;
	}
	
	if (append_file_csum(outf, img2, NULL))
		goto cleanup_and_exit;
	printf("[%08x] '%s'\n", off2, argv[arg_base + 2]);

	/* finally, the product header goes at off3 */
	offset = lseek(outf, 0, SEEK_CUR);
	if (-1 == offset) {
		perror(argv[arg_base + 5]);
		goto cleanup_and_exit;
	}
	if ((unsigned) offset > off3) {
		fprintf(stderr, "'off3'=%s is too low.\n", argv[arg_base + 4]);
		goto cleanup_and_exit;
	}
	if (off3 > (unsigned) offset) {
		if (add_padding(outf, off3 - offset, pad_byte)) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
		printf("[%08x] Padding (%02X's)\n", (unsigned)offset, pad_byte);
	}
	memset(&p_hdr, 0, sizeof p_hdr);
	/* no version for now */
	p_hdr.signature[0] = 0x70;
	p_hdr.signature[1] = 0x52;
	store32(p_hdr.offset1, off1);
	store32(p_hdr.offset2, off2);
	memcpy(p_hdr.bd_address, bd_addr, sizeof(p_hdr.bd_address));
	memset(p_hdr.pad, 0xff, sizeof(p_hdr.bd_address));
	store32(p_hdr.cfg_offset, cfg_off);
	if (safe_write(outf, &p_hdr, sizeof p_hdr)) {
		perror(argv[arg_base + 5]);
		goto cleanup_and_exit;
	}
	printf("[%08x] Product header\n", off3);

	if ((options & BOOTLOADER) && (options & EEPROM))  {
		/* write the header again, to set the checksum */
		if (lseek(outf, 0, SEEK_SET) == -1) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
		if (safe_write(outf, &i2c_hdr, sizeof i2c_hdr)) {
			perror(argv[arg_base + 5]);
			goto cleanup_and_exit;
		}
	}

	res = EXIT_SUCCESS;

cleanup_and_exit:
	if (img2 != -1) {
		if (close(img2))
			perror(argv[arg_base + 2]);
	}

	if (img1 != -1) {
		if (close(img1))
			perror(argv[arg_base]);
	}

	if (bloader != -1) {
		if (close(bloader))
			perror(argv[3]);
	}

	return res;
}


int main(int argc, const char* argv[])
{
	int res = EXIT_FAILURE;

	if (argc < 2 ) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "single"))
		res = create_single_image(argc, argv);
	else if (!strcmp(argv[1], "multi"))
		res = create_multi_image(argc, argv);
	else
		usage(argv[0]);

	exit(res);
}

