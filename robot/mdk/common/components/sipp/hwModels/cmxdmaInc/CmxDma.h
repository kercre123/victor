// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - CMXDMA controller
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_CMX_DMA_H__
#define __SIPP_CMX_DMA_H__

#include "registersMyriad2.h"
#include "sippHwCommon.h"

#include <iostream>
#include <string>
#include <stdint.h>

// CMXDMA control bits
#define DMA_START_CTRL_REG_AGENT_BIT       0
#define DMA_START_LNK_CMD_AGENT_BIT        32
#define DMA_ENABLE_BIT                     2
#define DMA_START_CTRL_REG_AGENT_BIT_MASK (1 << DMA_START_CTRL_REG_AGENT_BIT)
#define DMA_START_LNK_CMD_AGENT_BIT_MASK  ((uint64_t)1 << DMA_START_LNK_CMD_AGENT_BIT)
#define DMA_ENABLE_BIT_MASK               (1 << DMA_ENABLE_BIT)

// CMXDMA controller transaction types
#define TRANSACTION_1D                     0
#define TRANSACTION_1D_CHUNKING            1
#define TRANSACTION_2D                     2
#define TRANSACTION_2D_CHUNKING            3
#define IS_2D_TRANSACTION(x)              (x & 2)
#define IS_CHUNKING_TRANSACTION(x)        (x & 1)
#define IS_STRIDE_COMPLETED               (crt_istride_complete && crt_ostride_complete)

// CMXDMA width mask
#define CDMA_WIDTH_MASK                    0xffffff

typedef struct {
	uint32_t link_address                 : 32;
	uint32_t transaction_type             :  2;
	uint32_t transaction_priority         :  2;
	uint32_t burst_length                 :  4;
	uint32_t transaction_id               :  4;
	uint32_t irq_sel                      :  4;
	uint32_t partition                    :  4;
	uint32_t irq_disable                  :  1;
	uint32_t unused0                      :  6;
	uint32_t skip_nr                      :  5;
	uint32_t src_addr                     : 32;
	uint32_t dst_addr                     : 32;
	uint32_t transaction_length           : 24;
	uint32_t unused1                      :  8;
	uint32_t nplanes                      :  8;
	uint32_t unused2                      : 24;
	uint32_t transaction_dependent_field0 : 32;
	uint32_t transaction_dependent_field1 : 32;
	uint32_t transaction_dependent_field2 : 32;
	uint32_t transaction_dependent_field3 : 32;
	uint32_t transaction_dependent_field4 : 32;
	uint32_t transaction_dependent_field5 : 32;
} CmxDmaCmd;

class CmxDmaCtrlr {

public:
	CmxDmaCtrlr(std::string name = "CMXDMA Controller");
	~CmxDmaCtrlr() {
	}

	// AHB interface methods
	void AhbWrite (int, uint64_t, void * = 0);
	uint64_t AhbRead (int);

	// NOTE: following method is meaningful only in the model
	// Workaround to access the emulated CMX base address
	void SetCMXBaseAddress(uint32_t);

	void SetRCAFirstSlice(uint32_t); 
	void SetRCALastSlice(uint32_t);
	void SetRCAWrapEnable(uint32_t);
	void SetTransactionType(uint32_t);
	void SetTransactionPriority(uint32_t);
	void SetBurstLength(uint32_t);
	void SetTransactionID(uint32_t);
	void SetIrqSel(uint32_t);
	void SetPartition(uint32_t);
	void SetIrqDisable(uint32_t);
	void SetSourceAddress(uint32_t);
	void SetDestinationAddress(uint32_t);
	void SetTransactionLength(uint32_t);
	void SetNumberOfPlanes(uint32_t);
	void SetSourceWidth(uint32_t);
	void SetSourceStride(int32_t);
	void SetDestinationWidth(uint32_t);
	void SetDestinationStride(int32_t);
	void SetSourcePlaneStride(int32_t);
	void SetDestinationPlaneStride(int32_t);
	void SetStartControlRegisterAgent(uint32_t);
	void ClrStartControlRegisterAgent() { start_ctrl_reg_agent = 0; };
	void SetDMAEnable(uint32_t);
	void SetSoftwareReset(uint32_t);
	void SetHighPriorityLane(uint32_t);
	void SetArbitrationMode(uint32_t);
	void SetInterruptEnable(uint32_t);
	void SetInterruptFrequency(uint32_t, int);
	void SetInterruptReset(uint32_t);
	void SetEndPointer(uint32_t);
	void SetLinkedCommandAgentStartAddress(uint32_t, int);
	void SetStartLinkedCommandAgent(uint32_t, int);
	void ClrStartLinkedCommandAgent() { start_linked_cmd_agent[crt_agent] = 0; };
	void SetLinkedCommandAgentFirstSlice(uint32_t, int); 
	void SetLinkedCommandAgentLastSlice (uint32_t, int);
	void SetLinkedCommandAgentWrapEnable(uint32_t, int);
	void SetSkipDescriptor(uint32_t);
	void SetSliceSize(uint32_t);

	uint32_t GetRCAFirstSlice()                           {return rca_first_slice; };
	uint32_t GetRCALastSlice()                            {return rca_last_slice; };
	uint32_t GetRCAWrapEnable()                           {return rca_wrap_en; };
	uint32_t GetTransactionType()                         {return transaction_type; };
	uint32_t GetTransactionPriority()                     {return transaction_priority; };
	uint32_t GetBurstLength()                             {return burst_length; };
	uint32_t GetTransactionID()                           {return transaction_id; };
	uint32_t GetIrqSel()                                  {return irq_sel; };
	uint32_t GetPartition()                               {return partition; };
	uint32_t GetIrqDisable()                              {return irq_disable; };
	uint32_t GetSourceAddress()                           {return src_addr; };
	uint32_t GetDestinationAddress()                      {return dst_addr; };
	uint32_t GetTransactionLength()                       {return transaction_length; };
	uint32_t GetNumberOfPlanes()                          {return nplanes; };
	uint32_t GetSourceWidth()                             {return src_width; };
	uint32_t GetSourceStride()                            {return src_stride; };
	uint32_t GetDestinationWidth()                        {return dst_width; };
	uint32_t GetDestinationStride()                       {return dst_stride; };
	uint32_t GetSourcePlaneStride()                       {return src_plane_stride; };
	uint32_t GetDestinationPlaneStride()                  {return dst_plane_stride; };
	uint32_t GetStartControlRegisterAgent()               {return start_ctrl_reg_agent; };
	uint32_t GetDMAEnable()                               {return dma_en; };
	uint32_t GetSoftwareReset()                           {return software_reset; };
	uint32_t GetHighPriorityLane()                        {return high_priority_lane; };
	uint32_t GetArbitrationMode()                         {return arbitration_mode; };
	uint32_t GetDMABusyFlag()                             {return dma_busy; };
	uint32_t GetRCABusyFlag()                             {return rca_busy; };
	uint32_t GetLaneBusyFlag(int lane)                    {return lane_busy[lane]; };
	uint32_t GetLCABusyFlag(int agent)                    {return lca_busy[agent]; };
	uint32_t GetLastFinishedCommandID(int lane)           {return last_finished_cmd_id[lane]; };
	uint32_t GetCurrentBusyCommandID(int lane)            {return crt_busy_cmd_id[lane]; };
	uint32_t GetByteCount(int lane)                       {return byte_cnt[lane]; };
	uint32_t GetPlaneCount(int lane)                      {return plane_cnt[lane]; };
	uint32_t GetStrideCount(int lane)                     {return stride_cnt[lane]; };
	uint32_t GetLinkAddr(int lane)                        {return link_addr[lane]; };
	uint32_t GetInterruptEnable()                         {return irq_en; };
	uint32_t GetInterruptFrequency(int irq)               {return irq_freq[irq]; };
	uint32_t GetInterruptStatus()                         {return irq_status; };
	uint32_t GetInterruptReset()                          {return irq_reset; };
	uint32_t GetEndPointer()                              {return end_ptr; };
	uint32_t GetLinkedCommandAgentStartAddress(int agent) {return lca_start_addr[agent]; };
	uint32_t GetStartLinkedCommandAgent(int agent)        {return start_linked_cmd_agent[agent]; };
	uint32_t GetLinkedCommandAgentFirstSlice(int agent)   {return lca_first_slice[agent]; };
	uint32_t GetLinkedCommandAgentLastSlice (int agent)   {return lca_last_slice [agent]; };
	uint32_t GetLinkedCommandAgentWrapEnable(int agent)   {return lca_wrap_en    [agent]; };
	uint32_t GetSkipDescriptor()                          {return skip_descriptor; };
	uint32_t GetSliceSize()                               {return slice_size; };

	// Set up pointers and run
	void SetUpAndRun(void);

	// Run based on chained commands
	void LinkedRun(int);

private:
	std::string name;
	uint32_t cmx_base_address;
	uint32_t bottom_of_first_slice;
	uint32_t top_of_last_slice;
	bool wrappable_src;
	bool wrappable_dst;
	uint32_t first_slice;
	uint32_t last_slice;
	uint32_t wrap_en;
	CmxDmaCmd *command;

	uint32_t link_address;
	uint32_t rca_first_slice;
	uint32_t rca_last_slice;
	uint32_t rca_wrap_en;
	uint32_t transaction_type;
	uint32_t transaction_priority;
	uint32_t burst_length;
	uint32_t transaction_id;
	uint32_t irq_sel;
	uint32_t partition;
	uint32_t irq_disable;
	uint32_t skip_nr;
	uint32_t src_addr;
	uint32_t dst_addr;
	uint32_t transaction_length;
	uint32_t nplanes;
	uint32_t src_width;
	 int32_t src_stride;
	uint32_t dst_width;
	 int32_t dst_stride;
	 int32_t src_plane_stride;
	 int32_t dst_plane_stride;
	uint32_t start_ctrl_reg_agent;
	uint32_t dma_en;
	uint32_t software_reset;
	uint32_t high_priority_lane;
	uint32_t arbitration_mode;
	uint32_t dma_busy;
	uint32_t rca_busy;
	uint32_t lca_busy[4];
	uint32_t lane_busy[4];
	uint32_t last_finished_cmd_id[4];
	uint32_t crt_busy_cmd_id[4];
	uint32_t byte_cnt[4];
	uint32_t plane_cnt[4];
	uint32_t stride_cnt[4];
	uint32_t link_addr[4];
	uint32_t irq_en;
	uint32_t irq_freq[16];
	uint32_t irq_status;
	uint32_t irq_reset;
	uint32_t end_ptr;
	uint32_t lca_start_addr[4];
	uint32_t start_linked_cmd_agent[4];
	uint32_t lca_first_slice[4];
	uint32_t lca_last_slice[4];
	uint32_t lca_wrap_en[4];
	uint32_t skip_descriptor;
	uint32_t slice_size;

	uint32_t crt_lane;
	uint32_t crt_agent;
	uint32_t src_idx;
	uint32_t dst_idx;
	bool end_of_linked_cmds;
	bool skip_crt_cmd;

	void SetInputPointer (uint8_t **);
	void SetOutputPointer(uint8_t **);
	void SetStatus();
	void SetWrappableFlags();
	void SetLastFinishedCommandID();
	void AdvanceSourceAddress     (uint8_t **);
	void AdvanceDestinationAddress(uint8_t **);

	// Parse command structure
	void GetFirstCommand();
	void GetCommand();

	uint32_t IsEnabled () {
		return dma_en;
	}

	// Specific Run function for the CMX DMA controller
	void Run(void *, void *);
};

extern CmxDmaCtrlr cmxDma;

#endif // __SIPP_CMX_DMA_H__
