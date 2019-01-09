
/*
* This file is part of VL53L1 Protected
*
* Copyright (C) 2016, STMicroelectronics - All Rights Reserved
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0044
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*/







































#ifndef _VL53L1_HIST_PRIVATE_STRUCTS_H_
#define _VL53L1_HIST_PRIVATE_STRUCTS_H_

#include "vl53l1_types.h"
#include "vl53l1_hist_structs.h"

#define VL53L1_DEF_00001         8

#ifdef __cplusplus
extern "C" {
#endif










typedef struct {

	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00020;


	uint8_t  VL53L1_PRM_00021;


	uint8_t  VL53L1_PRM_00029;


	int32_t  VL53L1_PRM_00017;



	int32_t   VL53L1_PRM_00048[VL53L1_HISTOGRAM_BUFFER_SIZE];

	int32_t   VL53L1_PRM_00069[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t   VL53L1_PRM_00043[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00007[VL53L1_HISTOGRAM_BUFFER_SIZE];

	uint16_t  VL53L1_PRM_00016[VL53L1_HISTOGRAM_BUFFER_SIZE];

	uint16_t  VL53L1_PRM_00010[VL53L1_HISTOGRAM_BUFFER_SIZE];


} VL53L1_hist_gen1_algo_private_data_t;










typedef struct {

	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00020;


	uint8_t  VL53L1_PRM_00021;


	uint16_t VL53L1_PRM_00022;


	uint8_t  VL53L1_PRM_00008;


	uint8_t  VL53L1_PRM_00029;


	int32_t  VL53L1_PRM_00028;


	int32_t  VL53L1_PRM_00017;



	int32_t   VL53L1_PRM_00002[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00032[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00001[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t   VL53L1_PRM_00007[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00041[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00039[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00040[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen2_algo_filtered_data_t;










typedef struct {

	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00020;


	uint8_t  VL53L1_PRM_00021;


	int32_t  VL53L1_PRM_00031;



	uint8_t   VL53L1_PRM_00042[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t   VL53L1_PRM_00044[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint32_t  VL53L1_PRM_00016[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint16_t  VL53L1_PRM_00010[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t   VL53L1_PRM_00043[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen2_algo_detection_data_t;










typedef struct {

	uint8_t  VL53L1_PRM_00014;


	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00023;


	uint8_t  VL53L1_PRM_00024;


	uint8_t  VL53L1_PRM_00015;



	uint8_t  VL53L1_PRM_00025;


	uint8_t  VL53L1_PRM_00055;



	int32_t  VL53L1_PRM_00017;


	int32_t  VL53L1_PRM_00018;


	int32_t  VL53L1_PRM_00012;



	uint32_t VL53L1_PRM_00026;


	uint32_t VL53L1_PRM_00013;


	uint32_t VL53L1_PRM_00027;



	uint16_t VL53L1_PRM_00003;



} VL53L1_hist_pulse_data_t;










typedef struct {

	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00020;


	uint8_t  VL53L1_PRM_00021;


	uint8_t  VL53L1_PRM_00030;


	uint8_t  VL53L1_PRM_00045;


	int32_t  VL53L1_PRM_00028;


	int32_t  VL53L1_PRM_00031;



	uint8_t  VL53L1_PRM_00043[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t  VL53L1_PRM_00046[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t  VL53L1_PRM_00047[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t  VL53L1_PRM_00056[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t  VL53L1_PRM_00048[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t  VL53L1_PRM_00007[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t  VL53L1_PRM_00049;


	uint8_t  VL53L1_PRM_00050;


	uint8_t  VL53L1_PRM_00051;



	VL53L1_hist_pulse_data_t  VL53L1_PRM_00005[VL53L1_DEF_00001];






	VL53L1_histogram_bin_data_t   VL53L1_PRM_00009;


	VL53L1_histogram_bin_data_t   VL53L1_PRM_00038;


	VL53L1_histogram_bin_data_t   VL53L1_PRM_00052;


	VL53L1_histogram_bin_data_t   VL53L1_PRM_00053;


	VL53L1_histogram_bin_data_t   VL53L1_PRM_00054;






} VL53L1_hist_gen3_algo_private_data_t;










typedef struct {

	uint8_t  VL53L1_PRM_00019;


	uint8_t  VL53L1_PRM_00020;


	uint8_t  VL53L1_PRM_00021;



	int32_t   VL53L1_PRM_00002[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00032[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00001[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t   VL53L1_PRM_00039[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_PRM_00040[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t  VL53L1_PRM_00043[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen4_algo_filtered_data_t;

#ifdef __cplusplus
}
#endif

#endif

