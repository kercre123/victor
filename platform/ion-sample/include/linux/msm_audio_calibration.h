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
#ifndef _UAPI_MSM_AUDIO_CALIBRATION_H
#define _UAPI_MSM_AUDIO_CALIBRATION_H
#include <linux/types.h>
#include <linux/ioctl.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CAL_IOCTL_MAGIC 'a'
#define AUDIO_ALLOCATE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 200, void *)
#define AUDIO_DEALLOCATE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 201, void *)
#define AUDIO_PREPARE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 202, void *)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDIO_SET_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 203, void *)
#define AUDIO_GET_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 204, void *)
#define AUDIO_POST_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 205, void *)
#define AUDIO_GET_RTAC_ADM_INFO _IOR(CAL_IOCTL_MAGIC, 207, void *)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDIO_GET_RTAC_VOICE_INFO _IOR(CAL_IOCTL_MAGIC, 208, void *)
#define AUDIO_GET_RTAC_ADM_CAL _IOWR(CAL_IOCTL_MAGIC, 209, void *)
#define AUDIO_SET_RTAC_ADM_CAL _IOWR(CAL_IOCTL_MAGIC, 210, void *)
#define AUDIO_GET_RTAC_ASM_CAL _IOWR(CAL_IOCTL_MAGIC, 211, void *)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDIO_SET_RTAC_ASM_CAL _IOWR(CAL_IOCTL_MAGIC, 212, void *)
#define AUDIO_GET_RTAC_CVS_CAL _IOWR(CAL_IOCTL_MAGIC, 213, void *)
#define AUDIO_SET_RTAC_CVS_CAL _IOWR(CAL_IOCTL_MAGIC, 214, void *)
#define AUDIO_GET_RTAC_CVP_CAL _IOWR(CAL_IOCTL_MAGIC, 215, void *)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDIO_SET_RTAC_CVP_CAL _IOWR(CAL_IOCTL_MAGIC, 216, void *)
#define AUDIO_GET_RTAC_AFE_CAL _IOWR(CAL_IOCTL_MAGIC, 217, void *)
#define AUDIO_SET_RTAC_AFE_CAL _IOWR(CAL_IOCTL_MAGIC, 218, void *)
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CVP_VOC_RX_TOPOLOGY_CAL_TYPE = 0,
  CVP_VOC_TX_TOPOLOGY_CAL_TYPE,
  CVP_VOCPROC_STATIC_CAL_TYPE,
  CVP_VOCPROC_DYNAMIC_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CVS_VOCSTRM_STATIC_CAL_TYPE,
  CVP_VOCDEV_CFG_CAL_TYPE,
  CVP_VOCPROC_STATIC_COL_CAL_TYPE,
  CVP_VOCPROC_DYNAMIC_COL_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CVS_VOCSTRM_STATIC_COL_CAL_TYPE,
  ADM_TOPOLOGY_CAL_TYPE,
  ADM_CUST_TOPOLOGY_CAL_TYPE,
  ADM_AUDPROC_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ADM_AUDVOL_CAL_TYPE,
  ASM_TOPOLOGY_CAL_TYPE,
  ASM_CUST_TOPOLOGY_CAL_TYPE,
  ASM_AUDSTRM_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  AFE_COMMON_RX_CAL_TYPE,
  AFE_COMMON_TX_CAL_TYPE,
  AFE_ANC_CAL_TYPE,
  AFE_AANC_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  AFE_FB_SPKR_PROT_CAL_TYPE,
  AFE_HW_DELAY_CAL_TYPE,
  AFE_SIDETONE_CAL_TYPE,
  AFE_TOPOLOGY_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  AFE_CUST_TOPOLOGY_CAL_TYPE,
  LSM_CUST_TOPOLOGY_CAL_TYPE,
  LSM_TOPOLOGY_CAL_TYPE,
  LSM_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ADM_RTAC_INFO_CAL_TYPE,
  VOICE_RTAC_INFO_CAL_TYPE,
  ADM_RTAC_APR_CAL_TYPE,
  ASM_RTAC_APR_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VOICE_RTAC_APR_CAL_TYPE,
  MAD_CAL_TYPE,
  ULP_AFE_CAL_TYPE,
  ULP_LSM_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DTS_EAGLE_CAL_TYPE,
  AUDIO_CORE_METAINFO_CAL_TYPE,
  SRS_TRUMEDIA_CAL_TYPE,
  CORE_CUSTOM_TOPOLOGIES_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ADM_RTAC_AUDVOL_CAL_TYPE,
  ULP_LSM_TOPOLOGY_ID_CAL_TYPE,
  AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE,
  AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  MAX_CAL_TYPES,
};
#define AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE
#define AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  VERSION_0_0,
};
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  PER_VOCODER_CAL_BIT_MASK = 0x10000,
};
#define MAX_IOCTL_CMD_SIZE 512
struct audio_cal_header {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t data_size;
  int32_t version;
  int32_t cal_type;
  int32_t cal_type_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_header {
  int32_t version;
  int32_t buffer_number;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_data {
  int32_t cal_size;
  int32_t mem_handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_alloc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_alloc {
  struct audio_cal_header hdr;
  struct audio_cal_type_alloc cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_dealloc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_dealloc {
  struct audio_cal_header hdr;
  struct audio_cal_type_dealloc cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_prepare {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_prepare {
  struct audio_cal_header hdr;
  struct audio_cal_type_prepare cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_post {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_post {
  struct audio_cal_header hdr;
  struct audio_cal_type_post cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_metainfo {
  uint32_t nKey;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  RX_DEVICE,
  TX_DEVICE,
  MAX_PATH_TYPE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_adm_top {
  int32_t topology;
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t path;
  int32_t app_type;
  int32_t sample_rate;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_info_audproc {
  int32_t acdb_id;
  int32_t path;
  int32_t app_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t sample_rate;
};
struct audio_cal_info_audvol {
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t path;
  int32_t app_type;
  int32_t vol_index;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_info_afe {
  int32_t acdb_id;
  int32_t path;
  int32_t sample_rate;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_afe_top {
  int32_t topology;
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t path;
  int32_t sample_rate;
};
struct audio_cal_info_asm_top {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t topology;
  int32_t app_type;
};
struct audio_cal_info_audstrm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t app_type;
};
struct audio_cal_info_aanc {
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define MAX_HW_DELAY_ENTRIES 25
struct audio_cal_hw_delay_entry {
  uint32_t sample_rate;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t delay_usec;
};
struct audio_cal_hw_delay_data {
  uint32_t num_entries;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_hw_delay_entry entry[MAX_HW_DELAY_ENTRIES];
};
struct audio_cal_info_hw_delay {
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t path;
  int32_t property_type;
  struct audio_cal_hw_delay_data data;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_spkr_prot_states {
  MSM_SPKR_PROT_CALIBRATED,
  MSM_SPKR_PROT_CALIBRATION_IN_PROGRESS,
  MSM_SPKR_PROT_DISABLED,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  MSM_SPKR_PROT_NOT_CALIBRATED,
  MSM_SPKR_PROT_PRE_CALIBRATED,
  MSM_SPKR_PROT_IN_FTM_MODE
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_SPKR_PROT_IN_FTM_MODE MSM_SPKR_PROT_IN_FTM_MODE
enum msm_spkr_count {
  SP_V2_SPKR_1,
  SP_V2_SPKR_2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  SP_V2_NUM_MAX_SPKRS
};
struct audio_cal_info_spk_prot_cfg {
  int32_t r0[SP_V2_NUM_MAX_SPKRS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t t0[SP_V2_NUM_MAX_SPKRS];
  uint32_t quick_calib_flag;
  uint32_t mode;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_info_sp_th_vi_ftm_cfg {
  uint32_t wait_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t ftm_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_sp_ex_vi_ftm_cfg {
  uint32_t wait_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t ftm_time[SP_V2_NUM_MAX_SPKRS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t mode;
};
struct audio_cal_info_sp_ex_vi_param {
  int32_t freq_q20[SP_V2_NUM_MAX_SPKRS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t resis_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t qmct_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t status[SP_V2_NUM_MAX_SPKRS];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_info_sp_th_vi_param {
  int32_t r_dc_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t temp_q22[SP_V2_NUM_MAX_SPKRS];
  int32_t status[SP_V2_NUM_MAX_SPKRS];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_msm_spk_prot_status {
  int32_t r0[SP_V2_NUM_MAX_SPKRS];
  int32_t status;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_sidetone {
  uint16_t enable;
  uint16_t gain;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  int32_t mid;
  int32_t pid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_lsm_top {
  int32_t topology;
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t app_type;
};
struct audio_cal_info_lsm {
  int32_t acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t path;
  int32_t app_type;
};
struct audio_cal_info_voc_top {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t topology;
  int32_t acdb_id;
};
struct audio_cal_info_vocproc {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  int32_t tx_sample_rate;
  int32_t rx_sample_rate;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum {
  DEFAULT_FEATURE_SET,
  VOL_BOOST_FEATURE_SET,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_info_vocvol {
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t feature_set;
};
struct audio_cal_info_vocdev_cfg {
  int32_t tx_acdb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t rx_acdb_id;
};
#define MAX_VOICE_COLUMNS 20
union audio_cal_col_na {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __packed;
struct audio_cal_col {
  uint32_t id;
  uint32_t type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  union audio_cal_col_na na_value;
} __packed;
struct audio_cal_col_data {
  uint32_t num_columns;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_col column[MAX_VOICE_COLUMNS];
} __packed;
struct audio_cal_info_voc_col {
  int32_t table_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  struct audio_cal_col_data data;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_basic {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_basic {
  struct audio_cal_header hdr;
  struct audio_cal_type_basic cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_adm_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_adm_top cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_adm_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_adm_top cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_metainfo {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_metainfo cal_info;
};
struct audio_core_metainfo {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_metainfo cal_type;
};
struct audio_cal_type_audproc {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_audproc cal_info;
};
struct audio_cal_audproc {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_audproc cal_type;
};
struct audio_cal_type_audvol {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_audvol cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_audvol {
  struct audio_cal_header hdr;
  struct audio_cal_type_audvol cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_asm_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_asm_top cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_asm_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_asm_top cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_audstrm {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_audstrm cal_info;
};
struct audio_cal_audstrm {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_audstrm cal_type;
};
struct audio_cal_type_afe {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_afe cal_info;
};
struct audio_cal_afe {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_afe cal_type;
};
struct audio_cal_type_afe_top {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_afe_top cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_afe_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_afe_top cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_aanc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_aanc cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_aanc {
  struct audio_cal_header hdr;
  struct audio_cal_type_aanc cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_fb_spk_prot_cfg {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_spk_prot_cfg cal_info;
};
struct audio_cal_fb_spk_prot_cfg {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_fb_spk_prot_cfg cal_type;
};
struct audio_cal_type_sp_th_vi_ftm_cfg {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_th_vi_ftm_cfg cal_info;
};
struct audio_cal_sp_th_vi_ftm_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_th_vi_ftm_cfg cal_type;
};
struct audio_cal_type_sp_ex_vi_ftm_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_ex_vi_ftm_cfg cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_sp_ex_vi_ftm_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_ex_vi_ftm_cfg cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_hw_delay {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_hw_delay cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_hw_delay {
  struct audio_cal_header hdr;
  struct audio_cal_type_hw_delay cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_sidetone {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_sidetone cal_info;
};
struct audio_cal_sidetone {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_sidetone cal_type;
};
struct audio_cal_type_lsm_top {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_lsm_top cal_info;
};
struct audio_cal_lsm_top {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_lsm_top cal_type;
};
struct audio_cal_type_lsm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_lsm cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_lsm {
  struct audio_cal_header hdr;
  struct audio_cal_type_lsm cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_voc_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_voc_top cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_voc_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_voc_top cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_vocproc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_vocproc cal_info;
};
struct audio_cal_vocproc {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_vocproc cal_type;
};
struct audio_cal_type_vocvol {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_vocvol cal_info;
};
struct audio_cal_vocvol {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_vocvol cal_type;
};
struct audio_cal_type_vocdev_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_vocdev_cfg cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_vocdev_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_vocdev_cfg cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_type_voc_col {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_voc_col cal_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_voc_col {
  struct audio_cal_header hdr;
  struct audio_cal_type_voc_col cal_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct audio_cal_type_fb_spk_prot_status {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_info_msm_spk_prot_status cal_info;
};
struct audio_cal_fb_spk_prot_status {
  struct audio_cal_header hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_fb_spk_prot_status cal_type;
};
struct audio_cal_type_sp_th_vi_param {
  struct audio_cal_type_header cal_hdr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_th_vi_param cal_info;
};
struct audio_cal_sp_th_vi_param {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_th_vi_param cal_type;
};
struct audio_cal_type_sp_ex_vi_param {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_ex_vi_param cal_info;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct audio_cal_sp_ex_vi_param {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_ex_vi_param cal_type;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif

