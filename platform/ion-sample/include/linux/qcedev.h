/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _UAPI_QCEDEV__H
#define _UAPI_QCEDEV__H
#include <linux/types.h>
#include <linux/ioctl.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include "fips_status.h"
#define QCEDEV_MAX_SHA_BLOCK_SIZE 64
#define QCEDEV_MAX_BEARER 31
#define QCEDEV_MAX_KEY_SIZE 64
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QCEDEV_MAX_IV_SIZE 32
#define QCEDEV_MAX_BUFFERS 16
#define QCEDEV_MAX_SHA_DIGEST 32
#define QCEDEV_USE_PMEM 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QCEDEV_NO_PMEM 0
#define QCEDEV_AES_KEY_128 16
#define QCEDEV_AES_KEY_192 24
#define QCEDEV_AES_KEY_256 32
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum qcedev_oper_enum {
  QCEDEV_OPER_DEC = 0,
  QCEDEV_OPER_ENC = 1,
  QCEDEV_OPER_DEC_NO_KEY = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_OPER_ENC_NO_KEY = 3,
  QCEDEV_OPER_LAST
};
enum qcedev_cipher_alg_enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_ALG_DES = 0,
  QCEDEV_ALG_3DES = 1,
  QCEDEV_ALG_AES = 2,
  QCEDEV_ALG_LAST
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum qcedev_cipher_mode_enum {
  QCEDEV_AES_MODE_CBC = 0,
  QCEDEV_AES_MODE_ECB = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_AES_MODE_CTR = 2,
  QCEDEV_AES_MODE_XTS = 3,
  QCEDEV_AES_MODE_CCM = 4,
  QCEDEV_DES_MODE_CBC = 5,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_DES_MODE_ECB = 6,
  QCEDEV_AES_DES_MODE_LAST
};
enum qcedev_sha_alg_enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_ALG_SHA1 = 0,
  QCEDEV_ALG_SHA256 = 1,
  QCEDEV_ALG_SHA1_HMAC = 2,
  QCEDEV_ALG_SHA256_HMAC = 3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  QCEDEV_ALG_AES_CMAC = 4,
  QCEDEV_ALG_SHA_ALG_LAST
};
struct buf_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  union {
    uint32_t offset;
    uint8_t * vaddr;
  };
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t len;
};
struct qcedev_vbuf_info {
  struct buf_info src[QCEDEV_MAX_BUFFERS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct buf_info dst[QCEDEV_MAX_BUFFERS];
};
struct qcedev_pmem_info {
  int fd_src;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct buf_info src[QCEDEV_MAX_BUFFERS];
  int fd_dst;
  struct buf_info dst[QCEDEV_MAX_BUFFERS];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct qcedev_cipher_op_req {
  uint8_t use_pmem;
  union {
    struct qcedev_pmem_info pmem;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    struct qcedev_vbuf_info vbuf;
  };
  uint32_t entries;
  uint32_t data_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint8_t in_place_op;
  uint8_t enckey[QCEDEV_MAX_KEY_SIZE];
  uint32_t encklen;
  uint8_t iv[QCEDEV_MAX_IV_SIZE];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t ivlen;
  uint32_t byteoffset;
  enum qcedev_cipher_alg_enum alg;
  enum qcedev_cipher_mode_enum mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  enum qcedev_oper_enum op;
};
struct qcedev_sha_op_req {
  struct buf_info data[QCEDEV_MAX_BUFFERS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t entries;
  uint32_t data_len;
  uint8_t digest[QCEDEV_MAX_SHA_DIGEST];
  uint32_t diglen;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint8_t * authkey;
  uint32_t authklen;
  enum qcedev_sha_alg_enum alg;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct qfips_verify_t {
  unsigned kernel_size;
  void * kernel;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct file;
#define QCEDEV_IOC_MAGIC 0x87
#define QCEDEV_IOCTL_ENC_REQ _IOWR(QCEDEV_IOC_MAGIC, 1, struct qcedev_cipher_op_req)
#define QCEDEV_IOCTL_DEC_REQ _IOWR(QCEDEV_IOC_MAGIC, 2, struct qcedev_cipher_op_req)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QCEDEV_IOCTL_SHA_INIT_REQ _IOWR(QCEDEV_IOC_MAGIC, 3, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_SHA_UPDATE_REQ _IOWR(QCEDEV_IOC_MAGIC, 4, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_SHA_FINAL_REQ _IOWR(QCEDEV_IOC_MAGIC, 5, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_GET_SHA_REQ _IOWR(QCEDEV_IOC_MAGIC, 6, struct qcedev_sha_op_req)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QCEDEV_IOCTL_LOCK_CE _IO(QCEDEV_IOC_MAGIC, 7)
#define QCEDEV_IOCTL_UNLOCK_CE _IO(QCEDEV_IOC_MAGIC, 8)
#define QCEDEV_IOCTL_GET_CMAC_REQ _IOWR(QCEDEV_IOC_MAGIC, 9, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_UPDATE_FIPS_STATUS _IOWR(QCEDEV_IOC_MAGIC, 10, enum fips_status)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QCEDEV_IOCTL_QUERY_FIPS_STATUS _IOR(QCEDEV_IOC_MAGIC, 11, enum fips_status)
#endif

