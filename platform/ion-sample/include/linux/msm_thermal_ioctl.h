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
#ifndef _MSM_THERMAL_IOCTL_H
#define _MSM_THERMAL_IOCTL_H
#include <linux/ioctl.h>
#define MSM_THERMAL_IOCTL_NAME "msm_thermal_query"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_IOCTL_FREQ_SIZE 16
struct __attribute__((__packed__)) cpu_freq_arg {
  uint32_t cpu_num;
  uint32_t freq_req;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct __attribute__((__packed__)) clock_plan_arg {
  uint32_t cluster_num;
  uint32_t freq_table_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t set_idx;
  unsigned int freq_table[MSM_IOCTL_FREQ_SIZE];
};
struct __attribute__((__packed__)) voltage_plan_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t cluster_num;
  uint32_t voltage_table_len;
  uint32_t set_idx;
  uint32_t voltage_table[MSM_IOCTL_FREQ_SIZE];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct __attribute__((__packed__)) msm_thermal_ioctl {
  uint32_t size;
  union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    struct cpu_freq_arg cpu_freq;
    struct clock_plan_arg clock_freq;
    struct voltage_plan_arg voltage;
  };
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum {
  MSM_SET_CPU_MAX_FREQ = 0x00,
  MSM_SET_CPU_MIN_FREQ = 0x01,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  MSM_SET_CLUSTER_MAX_FREQ = 0x02,
  MSM_SET_CLUSTER_MIN_FREQ = 0x03,
  MSM_GET_CLUSTER_FREQ_PLAN = 0x04,
  MSM_GET_CLUSTER_VOLTAGE_PLAN = 0x05,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  MSM_CMD_MAX_NR,
};
#define MSM_THERMAL_MAGIC_NUM 0xCA
#define MSM_THERMAL_SET_CPU_MAX_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM, MSM_SET_CPU_MAX_FREQ, struct msm_thermal_ioctl)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_THERMAL_SET_CPU_MIN_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM, MSM_SET_CPU_MIN_FREQ, struct msm_thermal_ioctl)
#define MSM_THERMAL_SET_CLUSTER_MAX_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM, MSM_SET_CLUSTER_MAX_FREQ, struct msm_thermal_ioctl)
#define MSM_THERMAL_SET_CLUSTER_MIN_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM, MSM_SET_CLUSTER_MIN_FREQ, struct msm_thermal_ioctl)
#define MSM_THERMAL_GET_CLUSTER_FREQUENCY_PLAN _IOR(MSM_THERMAL_MAGIC_NUM, MSM_GET_CLUSTER_FREQ_PLAN, struct msm_thermal_ioctl)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_THERMAL_GET_CLUSTER_VOLTAGE_PLAN _IOR(MSM_THERMAL_MAGIC_NUM, MSM_GET_CLUSTER_VOLTAGE_PLAN, struct msm_thermal_ioctl)
#endif

