// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - CMXDMA controller
//
//
// -----------------------------------------------------------------------------

#include "CmxDma.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

CmxDmaCtrlr cmxDma;  //create an instance of the CMXDMA controller

CmxDmaCtrlr::CmxDmaCtrlr(std::string name) {
	this->name    = name;

	crt_lane = crt_agent = 0;
	end_of_linked_cmds = skip_crt_cmd = wrappable_src = wrappable_dst = false;
}

void CmxDmaCtrlr::SetCMXBaseAddress(uint32_t val) {
	cmx_base_address = val;
}

void CmxDmaCtrlr::AhbWrite (int addr, uint64_t data, void *ptr) {
	switch (addr) {
	case CDMA_CFG_LINK_ADR :
		SetRCAFirstSlice      ((data >>  0) & 0xf);
		SetRCALastSlice       ((data >>  4) & 0xf);
		SetRCAWrapEnable      ((data >>  8) & 0x1);
		SetTransactionType    ((data >> 32) & 0x3);
		SetTransactionPriority((data >> 34) & 0x3);
		SetBurstLength        ((data >> 36) & 0xf);
		SetTransactionID      ((data >> 40) & 0xf);
		SetIrqSel             ((data >> 44) & 0xf);
		SetPartition          ((data >> 48) & 0xf);
		SetIrqDisable         ((data >> 52) & 0x1);
		break;
	case CDMA_DST_SRC_ADR :
		SetSourceAddress     ((data >>  0) & 0xffffffff);
		SetDestinationAddress((data >> 32) & 0xffffffff);
		break;
	case CDMA_LEN_ADR :
		SetTransactionLength((data >>  0) & 0xffffff);
		SetNumberOfPlanes   ((data >> 32) & 0x0000ff);
		break;
	case CDMA_SRC_STRIDE_WIDTH_ADR :
		SetSourceWidth ((data >>  0) & 0x00ffffff);
		SetSourceStride((data >> 32) & 0xffffffff);
		break;
	case CDMA_DST_STRIDE_WIDTH_ADR :
		SetDestinationWidth ((data >>  0) & 0x00ffffff);
		SetDestinationStride((data >> 32) & 0xffffffff);
		break;
	case CDMA_PLANE_STRIDE_ADR :
		SetSourcePlaneStride     ((data >>  0) & 0xffffffff);
		SetDestinationPlaneStride((data >> 32) & 0xffffffff);
		break;
	case CDMA_CTRL_ADR :
		SetStartControlRegisterAgent((data >> 0) & 0x1);
		SetDMAEnable                ((data >> 2) & 0x1);
		SetSoftwareReset            ((data >> 3) & 0x1);
		SetHighPriorityLane         ((data >> 4) & 0x3);
		SetArbitrationMode          ((data >> 7) & 0x1);
		if(data & DMA_START_CTRL_REG_AGENT_BIT_MASK)
			SetUpAndRun();
		break;
	case CDMA_INT_EN_ADR :
		SetInterruptEnable(data & 0xffff);
		break;
	case CDMA_INT_FREQ0_ADR :
	case CDMA_INT_FREQ1_ADR :
	case CDMA_INT_FREQ2_ADR :
	case CDMA_INT_FREQ3_ADR :
	case CDMA_INT_FREQ4_ADR :
	case CDMA_INT_FREQ5_ADR :
	case CDMA_INT_FREQ6_ADR :
	case CDMA_INT_FREQ7_ADR :
	case CDMA_INT_FREQ8_ADR :
	case CDMA_INT_FREQ9_ADR :
	case CDMA_INT_FREQ10_ADR :
	case CDMA_INT_FREQ11_ADR :
	case CDMA_INT_FREQ12_ADR :
	case CDMA_INT_FREQ13_ADR :
	case CDMA_INT_FREQ14_ADR :
	case CDMA_INT_FREQ15_ADR : 
	{
		int base = CDMA_INT_FREQ0_ADR;
		int diff = addr - base;
		int irq = diff >> 3;
		SetInterruptFrequency(data & 0xff, irq);
		break;
	}
	case CDMA_INT_RESET_ADR :
		SetInterruptReset(data & 0xffff);
		break;
	case CDMA_END_PTR_ADR :
		SetEndPointer(data & 0xffffffff);
		break;
	case CDMA_LINK_CFG0_ADR :
	case CDMA_LINK_CFG1_ADR :
	case CDMA_LINK_CFG2_ADR :
	case CDMA_LINK_CFG3_ADR :
	{
		int base = CDMA_LINK_CFG0_ADR;
		int diff = addr - base;
		int agent = diff >> 3;
		SetLinkedCommandAgentStartAddress((data >>  0) & 0xffffffff, agent);
		SetStartLinkedCommandAgent       ((data >> 32) & 0x00000001, agent);
		SetLinkedCommandAgentFirstSlice  ((data >> 33) & 0x0000000f, agent);                
		SetLinkedCommandAgentLastSlice   ((data >> 37) & 0x0000000f, agent);
		SetLinkedCommandAgentWrapEnable  ((data >> 41) & 0x00000001, agent);
		if(data & DMA_START_LNK_CMD_AGENT_BIT_MASK)
			LinkedRun(agent);
		break;
	}
	case CDMA_SKIP_DES_ADR :
		SetSkipDescriptor(data & 0xffffffff);
		break;
	case CDMA_SLICE_SIZE_ADR :
		SetSliceSize(data & 0x78000);
		break;
	}
	
}

uint64_t CmxDmaCtrlr::AhbRead(int addr) {
	uint64_t data = 0;

	switch(addr) {
	case CDMA_CFG_LINK_ADR :
		data  = (uint64_t)GetRCAFirstSlice      () <<  0;
		data |= (uint64_t)GetRCALastSlice       () <<  4;
		data |= (uint64_t)GetRCAWrapEnable      () <<  8;
		data |= (uint64_t)GetTransactionType    () << 32;
		data |= (uint64_t)GetTransactionPriority() << 34;
		data |= (uint64_t)GetBurstLength        () << 36;
		data |= (uint64_t)GetTransactionID      () << 40;
		data |= (uint64_t)GetIrqSel             () << 44;
		data |= (uint64_t)GetPartition          () << 48;
		data |= (uint64_t)GetIrqDisable         () << 52;
		break;
	case CDMA_DST_SRC_ADR :
		data  = (uint64_t)GetSourceAddress     () <<  0;
		data |= (uint64_t)GetDestinationAddress() << 32;
		break;
	case CDMA_LEN_ADR :
		data  = (uint64_t)GetTransactionLength() <<  0;
		data |= (uint64_t)GetNumberOfPlanes   () << 32;
		break;
	case CDMA_SRC_STRIDE_WIDTH_ADR :
		data  = (uint64_t)GetSourceWidth () <<  0;
		data |= (uint64_t)GetSourceStride() << 32;
		break;
	case CDMA_DST_STRIDE_WIDTH_ADR :
		data  = (uint64_t)GetDestinationWidth () <<  0;
		data |= (uint64_t)GetDestinationStride() << 32;
		break;
	case CDMA_PLANE_STRIDE_ADR :
		data  = (uint64_t)GetSourcePlaneStride     () <<  0;
		data |= (uint64_t)GetDestinationPlaneStride() << 32;
		break;
	case CDMA_CTRL_ADR :
		data  = (uint64_t)GetStartControlRegisterAgent() << 0;
		data |= (uint64_t)GetDMAEnable                () << 2;
		data |= (uint64_t)GetSoftwareReset            () << 3;
		data |= (uint64_t)GetHighPriorityLane         () << 4;
		data |= (uint64_t)GetArbitrationMode          () << 7;
		break;
	case CDMA_STATUS_ADR :
		data  = (uint64_t)GetDMABusyFlag          ( ) <<  0;
		data |= (uint64_t)GetRCABusyFlag          ( ) <<  1;
		data |= (uint64_t)GetLCABusyFlag          (0) <<  3;
		data |= (uint64_t)GetLCABusyFlag          (1) <<  4;
		data |= (uint64_t)GetLCABusyFlag          (2) <<  5;
		data |= (uint64_t)GetLCABusyFlag          (3) <<  6;
		data |= (uint64_t)GetLaneBusyFlag         (0) <<  7;
		data |= (uint64_t)GetLaneBusyFlag         (1) <<  8;
		data |= (uint64_t)GetLaneBusyFlag         (2) <<  9;
		data |= (uint64_t)GetLaneBusyFlag         (3) << 10;
		data |= (uint64_t)GetLastFinishedCommandID(0) << 16;
		data |= (uint64_t)GetLastFinishedCommandID(1) << 20;
		data |= (uint64_t)GetLastFinishedCommandID(2) << 24;
		data |= (uint64_t)GetLastFinishedCommandID(3) << 28;
		data |= (uint64_t)GetCurrentBusyCommandID (0) << 32;
		data |= (uint64_t)GetCurrentBusyCommandID (1) << 36;
		data |= (uint64_t)GetCurrentBusyCommandID (2) << 40;
		data |= (uint64_t)GetCurrentBusyCommandID (3) << 44;
		break;
	case CDMA_LANE0_STATUS_ADR :
		data  = (uint64_t)GetByteCount  (0) <<  0;
		data |= (uint64_t)GetPlaneCount (0) << 24;
		data |= (uint64_t)GetStrideCount(0) << 32;
		break;
	case CDMA_LANE1_STATUS_ADR :
		data  = (uint64_t)GetByteCount  (1) <<  0;
		data |= (uint64_t)GetPlaneCount (1) << 24;
		data |= (uint64_t)GetStrideCount(1) << 32;
		break;
	case CDMA_LANE2_STATUS_ADR :
		data  = (uint64_t)GetByteCount  (2) <<  0;
		data |= (uint64_t)GetPlaneCount (2) << 24;
		data |= (uint64_t)GetStrideCount(2) << 32;
		break;
	case CDMA_LANE3_STATUS_ADR :
		data  = (uint64_t)GetByteCount  (3) <<  0;
		data |= (uint64_t)GetPlaneCount (3) << 24;
		data |= (uint64_t)GetStrideCount(3) << 32;
		break;
	case CDMA_TOP0_ADR :
		data  = (uint64_t)GetLinkAddr(0);
		break;
	case CDMA_TOP1_ADR :
		data  = (uint64_t)GetLinkAddr(1);
		break;
	case CDMA_TOP2_ADR :
		data  = (uint64_t)GetLinkAddr(2);
		break;
	case CDMA_TOP3_ADR :
		data  = (uint64_t)GetLinkAddr(3);
		break;
	case CDMA_INT_EN_ADR :
		data  = (uint64_t)GetInterruptEnable();
		break;
	case CDMA_INT_FREQ0_ADR :
	case CDMA_INT_FREQ1_ADR :
	case CDMA_INT_FREQ2_ADR :
	case CDMA_INT_FREQ3_ADR :
	case CDMA_INT_FREQ4_ADR :
	case CDMA_INT_FREQ5_ADR :
	case CDMA_INT_FREQ6_ADR :
	case CDMA_INT_FREQ7_ADR :
	case CDMA_INT_FREQ8_ADR :
	case CDMA_INT_FREQ9_ADR :
	case CDMA_INT_FREQ10_ADR :
	case CDMA_INT_FREQ11_ADR :
	case CDMA_INT_FREQ12_ADR :
	case CDMA_INT_FREQ13_ADR :
	case CDMA_INT_FREQ14_ADR :
	case CDMA_INT_FREQ15_ADR :
	{
		int base = CDMA_INT_FREQ0_ADR;
		int diff = addr - base;
		int irq = diff >> 3;
		data = GetInterruptFrequency(irq);
		break;
	}
		break;
	case CDMA_INT_STATUS_ADR :
		data  = (uint64_t)GetInterruptStatus();
		break;
	case CDMA_INT_RESET_ADR :
		data  = (uint64_t)GetInterruptReset();
		break;
	case CDMA_END_PTR_ADR :
		data  = (uint64_t)GetEndPointer();
		break;
	case CDMA_LINK_CFG0_ADR :
	case CDMA_LINK_CFG1_ADR :
	case CDMA_LINK_CFG2_ADR :
	case CDMA_LINK_CFG3_ADR :
	{
		int base = CDMA_LINK_CFG0_ADR;
		int diff = addr - base;
		int agent = diff >> 3;
		data  = (uint64_t)GetLinkedCommandAgentStartAddress(agent) <<  0;
		data |= (uint64_t)GetStartLinkedCommandAgent       (agent) << 32;
		data |= (uint64_t)GetLinkedCommandAgentFirstSlice  (agent) << 33;
		data |= (uint64_t)GetLinkedCommandAgentLastSlice   (agent) << 37;
		data |= (uint64_t)GetLinkedCommandAgentWrapEnable  (agent) << 41;
		break;
	}
	case CDMA_SKIP_DES_ADR :
		data  = (uint64_t)GetSkipDescriptor();
		break;
	case CDMA_SLICE_SIZE_ADR :
		data  = (uint64_t)GetSliceSize();
		break;
	}

	return data;
}

void CmxDmaCtrlr::SetRCAFirstSlice(uint32_t val) {
	rca_first_slice = val;
}

void CmxDmaCtrlr::SetRCALastSlice(uint32_t val) {
	rca_last_slice = val;
}

void CmxDmaCtrlr::SetRCAWrapEnable(uint32_t val) {
	rca_wrap_en = val;
}

void CmxDmaCtrlr::SetTransactionType(uint32_t val) {
	transaction_type = val;
}

void CmxDmaCtrlr::SetTransactionPriority(uint32_t val) {
	transaction_priority = val;
}

void CmxDmaCtrlr::SetBurstLength(uint32_t val) {
	burst_length = val;
}

void CmxDmaCtrlr::SetTransactionID(uint32_t val) {
	transaction_id = val;
}

void CmxDmaCtrlr::SetIrqSel(uint32_t val) {
	irq_sel = val;
}

void CmxDmaCtrlr::SetPartition(uint32_t val) {
	partition = val;
}

void CmxDmaCtrlr::SetIrqDisable(uint32_t val) {
	irq_disable = val;
}

void CmxDmaCtrlr::SetSourceAddress(uint32_t val) {
	src_addr = val;
}

void CmxDmaCtrlr::SetDestinationAddress(uint32_t val) {
	dst_addr = val;
}

void CmxDmaCtrlr::SetTransactionLength(uint32_t val) {
	transaction_length = val;
}

void CmxDmaCtrlr::SetNumberOfPlanes(uint32_t val) {
	nplanes = val;
}

void CmxDmaCtrlr::SetSourceWidth(uint32_t val) {
	src_width = val;
}

void CmxDmaCtrlr::SetSourceStride(int32_t val) {
	src_stride = val;
}

void CmxDmaCtrlr::SetDestinationWidth(uint32_t val) {
	dst_width = val;
}

void CmxDmaCtrlr::SetDestinationStride(int32_t val) {
	dst_stride = val;
}

void CmxDmaCtrlr::SetSourcePlaneStride(int32_t val) {
	src_plane_stride = val;
}

void CmxDmaCtrlr::SetDestinationPlaneStride(int32_t val) {
	dst_plane_stride = val;
}

void CmxDmaCtrlr::SetStartControlRegisterAgent(uint32_t val) {
	start_ctrl_reg_agent = val;
}

void CmxDmaCtrlr::SetDMAEnable(uint32_t val) {
	dma_en = val;
}

void CmxDmaCtrlr::SetSoftwareReset(uint32_t val) {
	software_reset = val;
}

void CmxDmaCtrlr::SetHighPriorityLane(uint32_t val) {
	high_priority_lane = val;
}

void CmxDmaCtrlr::SetArbitrationMode(uint32_t val) {
	arbitration_mode = val;
}

void CmxDmaCtrlr::SetInterruptEnable(uint32_t val) {
	irq_en = val;
}

void CmxDmaCtrlr::SetInterruptFrequency(uint32_t val, int irq) {
	irq_freq[irq] = val;
}

void CmxDmaCtrlr::SetInterruptReset(uint32_t val) {
	irq_reset = val;
}

void CmxDmaCtrlr::SetEndPointer(uint32_t val) {
	end_ptr = val;
}

void CmxDmaCtrlr::SetLinkedCommandAgentStartAddress(uint32_t val, int agent) {
	lca_start_addr[agent] = val;
}

void CmxDmaCtrlr::SetStartLinkedCommandAgent(uint32_t val, int agent) {
	start_linked_cmd_agent[agent] = val;
}

void CmxDmaCtrlr::SetLinkedCommandAgentFirstSlice(uint32_t val, int agent) {
	lca_first_slice[agent] = val;
}

void CmxDmaCtrlr::SetLinkedCommandAgentLastSlice(uint32_t val, int agent) {
	lca_last_slice[agent] = val;
}

void CmxDmaCtrlr::SetLinkedCommandAgentWrapEnable(uint32_t val, int agent) {
	lca_wrap_en[agent] = val;
}

void CmxDmaCtrlr::SetSkipDescriptor(uint32_t val) {
	skip_descriptor = val;
}

void CmxDmaCtrlr::SetSliceSize(uint32_t val) {
	slice_size = val;
}

void CmxDmaCtrlr::SetInputPointer(uint8_t **in_ptr) {
	int lpstride = src_plane_stride % slice_size;
	int ss_in_ps = src_plane_stride / slice_size;

	if(wrap_en && wrappable_src) {
		*in_ptr  = (uint8_t *)src_addr;
		for(uint32_t pl = 0; pl < plane_cnt[crt_lane]; pl++)
			*in_ptr  += (*in_ptr + ss_in_ps * slice_size >= (uint8_t *)top_of_last_slice) ? (first_slice - last_slice + ss_in_ps - 1) * slice_size + lpstride : src_plane_stride;
	} else {
		*in_ptr  = (uint8_t *)src_addr + plane_cnt[crt_lane] * src_plane_stride;
	}
}

void CmxDmaCtrlr::SetOutputPointer(uint8_t **out_ptr) {
	int lpstride = dst_plane_stride % slice_size;
	int ss_in_ps = dst_plane_stride / slice_size;

	if(wrap_en && wrappable_src) {
		*out_ptr = (uint8_t *)dst_addr;
		for(uint32_t pl = 0; pl < plane_cnt[crt_lane]; pl++)
			*out_ptr += (*out_ptr + ss_in_ps * slice_size >= (uint8_t *)top_of_last_slice) ? (first_slice - last_slice + ss_in_ps - 1) * slice_size + lpstride : dst_plane_stride;
	} else {
		*out_ptr = (uint8_t *)dst_addr + plane_cnt[crt_lane] * dst_plane_stride;
	}
}

void CmxDmaCtrlr::SetStatus() {
	dma_busy = 1;
	if(start_ctrl_reg_agent) {
		rca_busy = 1;
		first_slice = rca_first_slice;
		last_slice  = rca_last_slice;
		wrap_en     = rca_wrap_en;
	} else {
		lca_busy[crt_agent] = 1;
		first_slice = lca_first_slice[crt_agent];
		last_slice  = lca_last_slice [crt_agent];
		wrap_en     = lca_wrap_en    [crt_agent];
	}
	bottom_of_first_slice = cmx_base_address + (first_slice   ) * slice_size;
	top_of_last_slice     = cmx_base_address + (last_slice + 1) * slice_size;

	lane_busy[crt_lane] = 1;
	crt_busy_cmd_id[crt_lane] = transaction_id;
}

void CmxDmaCtrlr::SetWrappableFlags() {
	wrappable_src = (bottom_of_first_slice <= src_addr && src_addr <= top_of_last_slice);
	wrappable_dst = (bottom_of_first_slice <= dst_addr && dst_addr <= top_of_last_slice);
}

void CmxDmaCtrlr::SetLastFinishedCommandID() {
	last_finished_cmd_id[crt_lane] = transaction_id;
}

void CmxDmaCtrlr::AdvanceSourceAddress(uint8_t **src) {
	if(src_idx >= src_width) {
		if(wrap_en && wrappable_src)
			*src += (*src + src_stride >= (uint8_t *)top_of_last_slice) ? (first_slice - last_slice) * slice_size : src_stride;
		else
			*src += src_stride;
		src_idx = 0;
		byte_cnt[crt_lane] = 0;
		stride_cnt[crt_lane]++;
	}
}

void CmxDmaCtrlr::AdvanceDestinationAddress(uint8_t **dst) {
	if(dst_idx >= dst_width) {
		if(wrap_en && wrappable_dst)
			*dst += (*dst + dst_stride >= (uint8_t *)top_of_last_slice) ? (first_slice - last_slice) * slice_size : dst_stride;
		else
			*dst += dst_stride;
		dst_idx = 0;
	}
}

void CmxDmaCtrlr::GetFirstCommand() {
	command = (CmxDmaCmd *)lca_start_addr[crt_agent];
	GetCommand();
}

void CmxDmaCtrlr::GetCommand() {
	link_address         = command->link_address;
	transaction_type     = command->transaction_type;
	transaction_priority = command->transaction_priority;
	burst_length         = command->burst_length;
	transaction_id       = command->transaction_id;
	irq_sel              = command->irq_sel;
	partition            = command->partition;
	irq_disable          = command->irq_disable;
	skip_nr              = command->skip_nr;
	src_addr             = command->src_addr;
	dst_addr             = command->dst_addr;
	transaction_length   = command->transaction_length;
	nplanes              = command->nplanes;

	switch(transaction_type) {
		case TRANSACTION_1D :
			// Nothing to be done - no additional parameters
			break;
		case TRANSACTION_1D_CHUNKING :
			src_plane_stride = command->transaction_dependent_field0;
			dst_plane_stride = command->transaction_dependent_field1;
			break;
		case TRANSACTION_2D :
			src_width        = command->transaction_dependent_field0 & CDMA_WIDTH_MASK;
			src_stride       = command->transaction_dependent_field1;
			dst_width        = command->transaction_dependent_field2 & CDMA_WIDTH_MASK;
			dst_stride       = command->transaction_dependent_field3;
			break;
		case TRANSACTION_2D_CHUNKING :
			src_width        = command->transaction_dependent_field0 & CDMA_WIDTH_MASK;
			src_stride       = command->transaction_dependent_field1;
			dst_width        = command->transaction_dependent_field2 & CDMA_WIDTH_MASK;
			dst_stride       = command->transaction_dependent_field3;
			src_plane_stride = command->transaction_dependent_field4;
			dst_plane_stride = command->transaction_dependent_field5;
			break;
		default :
			std::cerr << "Invalid transaction type." << std::endl;
			abort();
	}	

	if(link_address) {
		command = (CmxDmaCmd *)link_address;
		end_of_linked_cmds = false;
	} else {
		end_of_linked_cmds = true;
	}
	if(skip_descriptor & (1 << skip_nr))
		skip_crt_cmd = true;
	else
		skip_crt_cmd = false;
}

void CmxDmaCtrlr::SetUpAndRun(void) {
	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t * in_ptr = 0;
	uint8_t *out_ptr = 0;

	// Set the appropriate flags
	SetStatus();

	// Determine which of the addresses needs to be wrapped
	SetWrappableFlags();

	// Start a transaction on each plane
	for(plane_cnt[crt_lane] = 0; plane_cnt[crt_lane] <= nplanes; plane_cnt[crt_lane]++) {
		SetInputPointer  (&in_ptr);
		SetOutputPointer(&out_ptr);
		
		Run((void *)in_ptr, (void *)out_ptr);
	}
	// Clear RCA bit and keep track of the last finished command's ID
	ClrStartControlRegisterAgent();
	SetLastFinishedCommandID();
}

void CmxDmaCtrlr::LinkedRun(int agent) {
	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t * in_ptr = 0;
	uint8_t *out_ptr = 0;

	// Parse the first command
	crt_agent = agent;
	GetFirstCommand();

	// Set the appropriate flags
	SetStatus();

	// Start chained transactions
	while(1) {
		// Determine which of the addresses needs to be wrapped
		SetWrappableFlags();

		// Start a transaction on each plane unless it's not valid
		if(!skip_crt_cmd) {
			for(plane_cnt[crt_lane] = 0; plane_cnt[crt_lane] <= nplanes; plane_cnt[crt_lane]++) {
				SetInputPointer  (&in_ptr);
				SetOutputPointer(&out_ptr);
				
				Run((void *)in_ptr, (void *)out_ptr);
			}
		}

		// End of chain
		if(end_of_linked_cmds)
			break;

		// Fetch the next command
		GetCommand();
	}
	// Clear LCA bit and keep track of the last finished command's ID
	ClrStartLinkedCommandAgent();
	SetLastFinishedCommandID();
}

void CmxDmaCtrlr::Run(void *in_ptr, void *out_ptr) {
	uint8_t *src;
	uint8_t *dst;
	uint32_t transaction_idx;

	src = (uint8_t *) in_ptr;
	dst = (uint8_t *)out_ptr;

	// Bytewise transfer from source to destionation
	transaction_idx = src_idx = dst_idx = 0;
	while(transaction_idx++ < transaction_length) {
		dst[dst_idx++] = src[src_idx++];

		if(IS_2D_TRANSACTION(transaction_type)) {
			AdvanceSourceAddress(&src);
			AdvanceDestinationAddress(&dst);
		}
	}
}
