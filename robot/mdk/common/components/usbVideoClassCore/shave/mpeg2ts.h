 /*
============================================================================
 *  Copyright (C) 2012 Movidius Ltd.
 *
 *  All Rights Reserved
 *
============================================================================
 *
 * This library contains proprietary intellectual property of Movidius Ltd.
 * This source code is the property and confidential information of Movidius Ltd.
 * The library and its source code are protected by various copyrights
 * and portions may also be protected by patents or other legal protections.
 *
 * This software is licensed for use with the Myriad family of processors only.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * http://www.movidius.com/
 *
 */


#ifndef _MUX_H_
#define _MUX_H_

//This is the CRC calculated with the polynomial:
//x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
#define CRCPOLY 0x04c11db7

//Size of a TS packet (in bytes)
#define TS_PCK_SIZE         188
#define TS_PCK_HEADER_SIZE    4
#define TS_PCK_DATA_SIZE     16

// PIDs
#define TS_PAT_PID             0
#define TS_PMT_PID             0x0100
#define TS_VIDEO_PID           0x1000
#define TS_PCR_PID             0x1001
#define TS_AUDIO_PID           0x1100

//Transport Stream Header defines
#define TS_SCRAMBLE_NONE       0

//Define Adaptation field control
#define TS_AFC_RESERVED        0
#define TS_AFC_NO_ADP_PYLD     1
#define TS_AFC_ADP_NO_PYLD     2
#define TS_AFC_ADP_PYLD        3

typedef struct {
  // TS header contents		//DESCRIPTION                              BITS
	unsigned int sync_byte_u8;			//Sync byte: 				8
	unsigned int error_indicator_u1;					//Trasnport error indicator : 		1
	unsigned int payload_start_indicator_u1;		//Payload unit start indicator : 	1
	unsigned int transport_priority_u1; 		//Transport priority : 			1
	unsigned int PID_u13;				//Packet IDentifier : 			13
	unsigned int scrambling_control_u2;			//Transport scrambling control : 	2
	unsigned int adaptation_field_control_u2;	        //Adaptation field control :		2
	unsigned int continuity_counter_u4;				//Continuity counter : 			4
  // TS adaptation field contents
    unsigned int adaptation_field_length_u8;
    unsigned int discontinuity_indicator_u1;
    unsigned int random_access_indicator_u1;
    unsigned int priority_indicator_u1;
    unsigned int PCR_flag_u1;
    unsigned int OPCR_flag_u1;
    unsigned int splice_point_flag_u1;
    unsigned int private_data_flag_u1;
    unsigned int extension_flag_u1;
    // represent on 32 bit
    unsigned int  PCR_u32, xPCR_u9;
    unsigned int  OPCR_u32, xOPCR_u9;
    unsigned int  splice_countdown_u8;
    unsigned int  private_length_u8;
    unsigned char private_data_u8[TS_PCK_DATA_SIZE];
} TSH_t;

typedef struct {
  unsigned int pointer_field_u8;
  unsigned int table_id_u8;
  unsigned int section_syntax_indicator_u1;
  unsigned int section_length_u10;
  unsigned int ts_id_u16;
  unsigned int version_number_u5;
  unsigned int current_next_indicator_u1;
  unsigned int section_number_u8;
  unsigned int last_section_number_u8;
  // support single program only at mux/demux
  unsigned int program_number_u16;
  unsigned int program_PID_u13;
  unsigned int crc_u32;
} PAT_t;

typedef struct {
  unsigned int  pointer_field_u8;
  unsigned int  table_id_u8;
  unsigned int  section_syntax_indicator_u1;
  unsigned int  section_length_u10;
  unsigned int  program_number_u16;
  unsigned int  version_number_u5;
  unsigned int  current_next_indicator_u1;
  unsigned int  section_number_u8;
  unsigned int  last_section_number_u8;
  unsigned int  PCR_PID_u13;
  unsigned int  program_info_length_u10;
  unsigned char program_descriptor_u8[TS_PCK_DATA_SIZE];
  // support 1 video and 1 audio stream at mux/demux
  unsigned int  VS_type_u8;
  unsigned int  VS_PID_u13;
  unsigned int  VS_info_length_u10;
  unsigned char VS_descriptor_u8[TS_PCK_DATA_SIZE];
  unsigned int  AS_type_u8;
  unsigned int  AS_PID_u13;
  unsigned int  AS_info_length_u10;
  unsigned char AS_descriptor_u8[TS_PCK_DATA_SIZE];
  unsigned int crc_u32;
} PMT_t;

typedef struct {
  // PES header contents
  unsigned int start_code_prefix_u24;	//Packet start code prefix : 24
  unsigned int stream_id_u8;			//Stream id
  unsigned int packet_length_u16;			//PES packet length
  struct {
    unsigned int scrambling_control_u2;
    unsigned int priority_u1;
    unsigned int data_alignment_indicator_u1;
    unsigned int copyright_u1;
    unsigned int original_u1;			//Original or copy
    unsigned int PTS_DTS_flags_u2;
    unsigned int ESCR_flag_u1;
    unsigned int ES_rate_flag_u1;
    unsigned int DSM_trick_flag_u1;
    unsigned int extra_copy_info_flag_u1;
    unsigned int CRC_flag_u1;
    unsigned int extension_flag_u1;
    unsigned int header_data_length_u8;
    unsigned int PTS_u32;
    unsigned int DTS_u32;
    unsigned int ESCR_u32;
    unsigned int xESCR_u9;
    unsigned int ESrate_u8;
  } ext;
} PES_t;

extern void mux_Init(void);
extern void mux_Handler(void);
extern unsigned int mux_VS_packet(unsigned int, unsigned int);
extern unsigned int mux_AS_packet(unsigned int, unsigned int);
extern unsigned int mux_TS_packet(unsigned int, unsigned int);

#endif  // _MUX_H_
