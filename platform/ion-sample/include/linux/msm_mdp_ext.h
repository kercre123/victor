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
#ifndef _MSM_MDP_EXT_H_
#define _MSM_MDP_EXT_H_
#include <linux/msm_mdp.h>
#define MDP_IOCTL_MAGIC 'S'
#define MSMFB_ATOMIC_COMMIT _IOWR(MDP_IOCTL_MAGIC, 128, void *)
#define MSMFB_ASYNC_POSITION_UPDATE _IOWR(MDP_IOCTL_MAGIC, 129, struct mdp_position_update)
#define MSMFB_MDP_SET_CFG _IOW(MDP_IOCTL_MAGIC, 130, struct mdp_set_cfg)
#ifdef __LP64
#define MDP_LAYER_COMMIT_V1_PAD 1
#else
#define MDP_LAYER_COMMIT_V1_PAD 3
#endif
#define MDP_LAYER_FLIP_LR 0x1
#define MDP_LAYER_FLIP_UD 0x2
#define MDP_LAYER_ENABLE_PIXEL_EXT 0x4
#define MDP_LAYER_FORGROUND 0x8
#define MDP_LAYER_SECURE_SESSION 0x10
#define MDP_LAYER_SOLID_FILL 0x20
#define MDP_LAYER_DEINTERLACE 0x40
#define MDP_LAYER_BWC 0x80
#define MDP_LAYER_ASYNC 0x100
#define MDP_LAYER_PP 0x200
#define MDP_LAYER_SECURE_DISPLAY_SESSION 0x400
#define MDP_LAYER_ENABLE_QSEED3_SCALE 0x800
#define MDP_DESTSCALER_ENABLE 0x1
#define MDP_DESTSCALER_SCALE_UPDATE 0x2
#define MDP_DESTSCALER_ENHANCER_UPDATE 0x4
#define MDP_LAYER_MULTIRECT_ENABLE 0x1000
#define MDP_LAYER_MULTIRECT_PARALLEL_MODE 0x2000
#define MDP_VALIDATE_LAYER 0x01
#define MDP_COMMIT_WAIT_FOR_FINISH 0x02
#define MDP_COMMIT_SYNC_FENCE_WAIT 0x04
#define MDP_COMMIT_VERSION_1_0 0x00010000
#define OUT_LAYER_COLOR_SPACE
struct mdp_layer_plane {
  int fd;
  uint32_t offset;
  uint32_t stride;
};
struct mdp_layer_buffer {
  uint32_t width;
  uint32_t height;
  uint32_t format;
  struct mdp_layer_plane planes[MAX_PLANES];
  uint32_t plane_count;
  struct mult_factor comp_ratio;
  int fence;
  uint32_t reserved;
};
struct mdp_input_layer {
  uint32_t flags;
  uint32_t pipe_ndx;
  uint8_t horz_deci;
  uint8_t vert_deci;
  uint8_t alpha;
  uint16_t z_order;
  uint32_t transp_mask;
  uint32_t bg_color;
  enum mdss_mdp_blend_op blend_op;
  enum mdp_color_space color_space;
  struct mdp_rect src_rect;
  struct mdp_rect dst_rect;
  void __user * scale;
  struct mdp_layer_buffer buffer;
  void __user * pp_info;
  int error_code;
  uint32_t reserved[6];
};
struct mdp_output_layer {
  uint32_t flags;
  uint32_t writeback_ndx;
  struct mdp_layer_buffer buffer;
  enum mdp_color_space color_space;
  uint32_t reserved[5];
};
struct mdp_destination_scaler_data {
  uint32_t flags;
  uint32_t dest_scaler_ndx;
  uint32_t lm_width;
  uint32_t lm_height;
  uint64_t __user scale;
};
#define MDP_VIDEO_FRC_ENABLE (1 << 0)
struct mdp_frc_info {
  uint32_t flags;
  uint32_t frame_cnt;
  int64_t timestamp;
};
struct mdp_layer_commit_v1 {
  uint32_t flags;
  int release_fence;
  struct mdp_rect left_roi;
  struct mdp_rect right_roi;
  struct mdp_input_layer __user * input_layers;
  uint32_t input_layer_cnt;
  struct mdp_output_layer __user * output_layer;
  int retire_fence;
  void __user * dest_scaler;
  uint32_t dest_scaler_cnt;
  struct mdp_frc_info __user * frc_info;
  uint32_t reserved[MDP_LAYER_COMMIT_V1_PAD];
};
struct mdp_layer_commit {
  uint32_t version;
  union {
    struct mdp_layer_commit_v1 commit_v1;
  };
};
struct mdp_point {
  uint32_t x;
  uint32_t y;
};
struct mdp_async_layer {
  uint32_t flags;
  uint32_t pipe_ndx;
  struct mdp_point src;
  struct mdp_point dst;
  int error_code;
  uint32_t reserved[3];
};
struct mdp_position_update {
  struct mdp_async_layer __user * input_layers;
  uint32_t input_layer_cnt;
};
#define MAX_DET_CURVES 3
struct mdp_det_enhance_data {
  uint32_t enable;
  int16_t sharpen_level1;
  int16_t sharpen_level2;
  uint16_t clip;
  uint16_t limit;
  uint16_t thr_quiet;
  uint16_t thr_dieout;
  uint16_t thr_low;
  uint16_t thr_high;
  uint16_t prec_shift;
  int16_t adjust_a[MAX_DET_CURVES];
  int16_t adjust_b[MAX_DET_CURVES];
  int16_t adjust_c[MAX_DET_CURVES];
};
#define ENABLE_SCALE 0x1
#define ENABLE_DETAIL_ENHANCE 0x2
#define ENABLE_DIRECTION_DETECTION 0x4
#define SCALER_LUT_SWAP 0x1
#define SCALER_LUT_DIR_WR 0x2
#define SCALER_LUT_Y_CIR_WR 0x4
#define SCALER_LUT_UV_CIR_WR 0x8
#define SCALER_LUT_Y_SEP_WR 0x10
#define SCALER_LUT_UV_SEP_WR 0x20
#define FILTER_EDGE_DIRECTED_2D 0x0
#define FILTER_CIRCULAR_2D 0x1
#define FILTER_SEPARABLE_1D 0x2
#define FILTER_BILINEAR 0x3
#define FILTER_ALPHA_DROP_REPEAT 0x0
#define FILTER_ALPHA_BILINEAR 0x1
struct mdp_scale_data_v2 {
  uint32_t enable;
  int32_t init_phase_x[MAX_PLANES];
  int32_t phase_step_x[MAX_PLANES];
  int32_t init_phase_y[MAX_PLANES];
  int32_t phase_step_y[MAX_PLANES];
  uint32_t num_ext_pxls_left[MAX_PLANES];
  uint32_t num_ext_pxls_right[MAX_PLANES];
  uint32_t num_ext_pxls_top[MAX_PLANES];
  uint32_t num_ext_pxls_btm[MAX_PLANES];
  int32_t left_ftch[MAX_PLANES];
  int32_t left_rpt[MAX_PLANES];
  int32_t right_ftch[MAX_PLANES];
  int32_t right_rpt[MAX_PLANES];
  uint32_t top_rpt[MAX_PLANES];
  uint32_t btm_rpt[MAX_PLANES];
  uint32_t top_ftch[MAX_PLANES];
  uint32_t btm_ftch[MAX_PLANES];
  uint32_t roi_w[MAX_PLANES];
  uint32_t preload_x[MAX_PLANES];
  uint32_t preload_y[MAX_PLANES];
  uint32_t src_width[MAX_PLANES];
  uint32_t src_height[MAX_PLANES];
  uint32_t dst_width;
  uint32_t dst_height;
  uint32_t y_rgb_filter_cfg;
  uint32_t uv_filter_cfg;
  uint32_t alpha_filter_cfg;
  uint32_t blend_cfg;
  uint32_t lut_flag;
  uint32_t dir_lut_idx;
  uint32_t y_rgb_cir_lut_idx;
  uint32_t uv_cir_lut_idx;
  uint32_t y_rgb_sep_lut_idx;
  uint32_t uv_sep_lut_idx;
  struct mdp_det_enhance_data detail_enhance;
  uint64_t reserved[8];
};
struct mdp_scale_luts_info {
  uint64_t __user dir_lut;
  uint64_t __user cir_lut;
  uint64_t __user sep_lut;
  uint32_t dir_lut_size;
  uint32_t cir_lut_size;
  uint32_t sep_lut_size;
};
#define MDP_QSEED3_LUT_CFG 0x1
struct mdp_set_cfg {
  uint64_t flags;
  uint32_t len;
  uint64_t __user payload;
};
#endif

