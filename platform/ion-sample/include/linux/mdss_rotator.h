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
#ifndef _UAPI_MDSS_ROTATOR_H_
#define _UAPI_MDSS_ROTATOR_H_
#include <linux/msm_mdp_ext.h>
#define MDSS_ROTATOR_IOCTL_MAGIC 'w'
#define MDSS_ROTATION_OPEN _IOWR(MDSS_ROTATOR_IOCTL_MAGIC, 1, struct mdp_rotation_config *)
#define MDSS_ROTATION_CONFIG _IOWR(MDSS_ROTATOR_IOCTL_MAGIC, 2, struct mdp_rotation_config *)
#define MDSS_ROTATION_REQUEST _IOWR(MDSS_ROTATOR_IOCTL_MAGIC, 3, struct mdp_rotation_request *)
#define MDSS_ROTATION_CLOSE _IOW(MDSS_ROTATOR_IOCTL_MAGIC, 4, unsigned int)
#define MDP_ROTATION_NOP 0x01
#define MDP_ROTATION_FLIP_LR 0x02
#define MDP_ROTATION_FLIP_UD 0x04
#define MDP_ROTATION_90 0x08
#define MDP_ROTATION_180 (MDP_ROTATION_FLIP_LR | MDP_ROTATION_FLIP_UD)
#define MDP_ROTATION_270 (MDP_ROTATION_90 | MDP_ROTATION_180)
#define MDP_ROTATION_DEINTERLACE 0x10
#define MDP_ROTATION_BWC_EN 0x40
#define MDP_ROTATION_SECURE 0x80
#define MDSS_ROTATION_REQUEST_VALIDATE 0x01
#define MDP_ROTATION_REQUEST_VERSION_1_0 0x00010000
#define MDSS_ROTATION_HW_ANY 0xFFFFFFFF
struct mdp_rotation_buf_info {
  uint32_t width;
  uint32_t height;
  uint32_t format;
  struct mult_factor comp_ratio;
};
struct mdp_rotation_config {
  uint32_t version;
  uint32_t session_id;
  struct mdp_rotation_buf_info input;
  struct mdp_rotation_buf_info output;
  uint32_t frame_rate;
  uint32_t flags;
  uint32_t reserved[6];
};
struct mdp_rotation_item {
  uint32_t flags;
  struct mdp_rect src_rect;
  struct mdp_rect dst_rect;
  struct mdp_layer_buffer input;
  struct mdp_layer_buffer output;
  uint32_t pipe_idx;
  uint32_t wb_idx;
  uint32_t session_id;
  uint32_t reserved[6];
};
struct mdp_rotation_request {
  uint32_t version;
  uint32_t flags;
  uint32_t count;
  struct mdp_rotation_item __user * list;
  uint32_t reserved[6];
};
#endif

