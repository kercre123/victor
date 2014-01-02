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


#include "mpeg2ts.h"

unsigned char *TS_packet;
unsigned int TS_VS_state;
unsigned int TS_AS_state;
unsigned int TS_VS_counter;
unsigned int TS_AS_counter;
unsigned int TS_PAT_counter;
unsigned int TS_PMT_counter;
unsigned int TS_PCR_counter;
float TS_timer = 0;

volatile TSH_t TS_header;
volatile PAT_t TS_PAT;
volatile PMT_t TS_PMT;
volatile PES_t TS_PES;


unsigned int crc32(unsigned int _crc, unsigned char *p, unsigned int len)
{
    unsigned char i;
    while (len--) {
        _crc ^= *p++ << 24;
        for (i = 0; i < 8; i++) _crc = (_crc << 1) ^ ((_crc & 0x80000000) ? CRCPOLY : 0);
    }
    return _crc;
}

unsigned int mux_TS_header()
{
    unsigned int i, pos, section_start;

    // encode from local data structure
    TS_packet[0]  =  TS_header.sync_byte_u8;
    TS_packet[1]  = (TS_header.error_indicator_u1<<0x07) \
                  | (TS_header.payload_start_indicator_u1<<0x06) \
                  | (TS_header.transport_priority_u1<<0x05) \
                  | ((TS_header.PID_u13>>0x08)&0x1F);
    TS_packet[2]  =  TS_header.PID_u13;
    TS_packet[3]  = (TS_header.scrambling_control_u2<<0x06) \
                  | (TS_header.adaptation_field_control_u2<<0x04) \
                  | (TS_header.continuity_counter_u4);
    pos = 0x04;
    // see if adaptation field present
    if ( (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_NO_PYLD) || (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_PYLD) )
    {
        TS_packet[pos] = TS_header.adaptation_field_length_u8;
        pos++;
        section_start = pos;
        // read adaptation flags and optional data
        if (TS_header.adaptation_field_length_u8 != 0)
        {
            TS_packet[pos]  = (TS_header.discontinuity_indicator_u1<<0x07) \
                            | (TS_header.random_access_indicator_u1<<0x06) \
                            | (TS_header.priority_indicator_u1<<0x05) \
                            | (TS_header.PCR_flag_u1<<0x04) \
                            | (TS_header.OPCR_flag_u1<<0x03) \
                            | (TS_header.splice_point_flag_u1<<0x02) \
                            | (TS_header.private_data_flag_u1<<0x01);
            // extension not supported
            //TS_packet[pos] |=  TS_header.extension_flag_u1;
            pos++;
            // support for PCR
            if (TS_header.PCR_flag_u1 != 0)
            {
                TS_packet[pos]   = (TS_header.PCR_u32>>0x18);
                TS_packet[pos+1] = (TS_header.PCR_u32>>0x10);
                TS_packet[pos+2] = (TS_header.PCR_u32>>0x08);
                TS_packet[pos+3] = (TS_header.PCR_u32>>0x00);
                TS_packet[pos+4] = (TS_header.xPCR_u9>>0x08);
                TS_packet[pos+5] = (TS_header.xPCR_u9>>0x00);
                pos+=0x06;
            }
            // support for OPCR
            if (TS_header.OPCR_flag_u1 != 0)
            {
                TS_packet[pos]   = (TS_header.OPCR_u32>>0x18);
                TS_packet[pos+1] = (TS_header.OPCR_u32>>0x10);
                TS_packet[pos+2] = (TS_header.OPCR_u32>>0x08);
                TS_packet[pos+3] = (TS_header.OPCR_u32>>0x00);
                TS_packet[pos+4] = (TS_header.xOPCR_u9>>0x08);
                TS_packet[pos+5] = (TS_header.xOPCR_u9>>0x00);
                pos+=0x06;
            }
            // support for splicing point
            if (TS_header.splice_point_flag_u1 != 0)
            {
                TS_packet[pos] = TS_header.splice_countdown_u8;
                pos++;
            }
            // support for private data
            if (TS_header.private_data_flag_u1 != 0)
            {
                TS_packet[pos] = TS_header.private_length_u8;
                pos++;
                for (i=0; i<TS_header.private_length_u8; i++)
                {
                    if (i<TS_PCK_DATA_SIZE) TS_packet[pos+i] = TS_header.private_data_u8[i];
                }
                pos+=TS_header.private_length_u8;
            }
            // add padding bytes
            for (i=pos; i<(section_start+TS_header.adaptation_field_length_u8); i++) TS_packet[i] = 0xFF;
            pos = section_start+TS_header.adaptation_field_length_u8;
        }
    }
    // limit size
    if (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_NO_PYLD) for (;pos<TS_PCK_SIZE;pos++) TS_packet[pos] = 0xFF;
    if (pos > TS_PCK_SIZE) pos = TS_PCK_SIZE;
    return pos;
}

unsigned int mux_PAT_packet()
{
    unsigned int i, pos, section_start;
    // prepare header
    TS_header.sync_byte_u8 = 0x47;
    TS_header.error_indicator_u1 = 0x00;
    TS_header.payload_start_indicator_u1 = 0x01;
    TS_header.transport_priority_u1 = 0x00;
    TS_header.PID_u13 = TS_PAT_PID;
    TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
    TS_header.adaptation_field_control_u2 = TS_AFC_NO_ADP_PYLD;
    TS_header.continuity_counter_u4 = TS_PAT_counter;
    TS_PAT_counter = (TS_PAT_counter+0x01)&0x0F;
    // prepare PAT data
    TS_PAT.pointer_field_u8 = 0x00;
    TS_PAT.table_id_u8 = 0x00;
    TS_PAT.section_syntax_indicator_u1 = 0x01;
    TS_PAT.section_length_u10 = 0x0D;
    TS_PAT.ts_id_u16 = 0xDEAD;
    TS_PAT.version_number_u5 = 0x00;
    TS_PAT.current_next_indicator_u1 = 0x01;
    TS_PAT.section_number_u8 = 0x00;
    TS_PAT.last_section_number_u8 = 0x00;
    // support single program only at mux/demux
    TS_PAT.program_number_u16 = 0x01;
    TS_PAT.program_PID_u13 = TS_PMT_PID;

    // put signals into packet
    pos = mux_TS_header();
    TS_packet[pos]    =  TS_PAT.pointer_field_u8;
    TS_packet[pos+1]  =  TS_PAT.table_id_u8;
    TS_packet[pos+2]  = (TS_PAT.section_syntax_indicator_u1<<0x07) \
                      | (TS_PAT.section_length_u10>>0x08) \
                      | 0x30;
    TS_packet[pos+3]  =  TS_PAT.section_length_u10;
    pos+=0x04;
    section_start = pos;
    TS_packet[pos]    = (TS_PAT.ts_id_u16>>0x08);
    TS_packet[pos+1]  =  TS_PAT.ts_id_u16;
    TS_packet[pos+2]  = (TS_PAT.version_number_u5<<0x01) \
                      | TS_PAT.current_next_indicator_u1;
    TS_packet[pos+3]  =  TS_PAT.section_number_u8;
    TS_packet[pos+4]  =  TS_PAT.last_section_number_u8;
    TS_packet[pos+5]  = (TS_PAT.program_number_u16>>0x08);
    TS_packet[pos+6]  =  TS_PAT.program_number_u16;
    TS_packet[pos+7]  = (TS_PAT.program_PID_u13>>0x08) \
                      | 0xE0;
    TS_packet[pos+8]  =  TS_PAT.program_PID_u13;
    pos+=0x09;
    // add padding
    section_start = section_start + TS_PAT.section_length_u10 - 4;
    while (pos<section_start)
    {
        TS_packet[pos] = 0xFF;
        pos++;
    }
    // add crc
    TS_PAT.crc_u32   = crc32(0xFFFFFFFF, &TS_packet[5], (pos-0x05));
    TS_packet[pos]   = (TS_PAT.crc_u32>>0x18);
    TS_packet[pos+1] = (TS_PAT.crc_u32>>0x10);
    TS_packet[pos+2] = (TS_PAT.crc_u32>>0x08);
    TS_packet[pos+3] =  TS_PAT.crc_u32;
    pos+=0x04;
    // limit size
    while (pos<TS_PCK_SIZE)
    {
        TS_packet[pos] = 0xFF;
        pos++;
    }
    return pos;
}

unsigned int mux_PMT_packet()
{
    unsigned int i, pos, section_start;

    // prepare header
    TS_header.sync_byte_u8 = 0x47;
    TS_header.error_indicator_u1 = 0x00;
    TS_header.payload_start_indicator_u1 = 0x01;
    TS_header.transport_priority_u1 = 0x00;
    TS_header.PID_u13 = TS_PAT.program_PID_u13;
    TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
    TS_header.adaptation_field_control_u2 = TS_AFC_NO_ADP_PYLD;
    TS_header.continuity_counter_u4 = TS_PMT_counter;
    TS_PMT_counter = (TS_PMT_counter+0x01)&0x0F;
    // prepare PMT data
    TS_PMT.pointer_field_u8 = 0x00;
    TS_PMT.table_id_u8 = 0x02;
    TS_PMT.section_syntax_indicator_u1 = 0x01;
    TS_PMT.section_length_u10 = 0x17 + 0x0C + 0x06 + 0x06;
    TS_PMT.program_number_u16 = TS_PAT.program_number_u16;
    TS_PMT.version_number_u5 = 0x00;
    TS_PMT.current_next_indicator_u1 = 0x01;
    TS_PMT.section_number_u8 = 0x00;
    TS_PMT.last_section_number_u8 = 0x00;
    TS_PMT.PCR_PID_u13 = TS_PCR_PID;
    TS_PMT.program_info_length_u10  = 0x0C;
    TS_PMT.program_descriptor_u8[0] = 0x05;
    TS_PMT.program_descriptor_u8[1] = 0x04;
    TS_PMT.program_descriptor_u8[2] = 0x48;
    TS_PMT.program_descriptor_u8[3] = 0x44;
    TS_PMT.program_descriptor_u8[4] = 0x4D;
    TS_PMT.program_descriptor_u8[5] = 0x56;
    TS_PMT.program_descriptor_u8[6] = 0x88;
    TS_PMT.program_descriptor_u8[7] = 0x04;
    TS_PMT.program_descriptor_u8[8]  = 'W';
    TS_PMT.program_descriptor_u8[9]  = 'o';
    TS_PMT.program_descriptor_u8[10] = 'l';
    TS_PMT.program_descriptor_u8[11] = 'f';
    // support 1 video and 1 audio stream at mux/demux
    TS_PMT.VS_type_u8 = 0x1B; // stream type AVC
    TS_PMT.VS_PID_u13 = TS_VIDEO_PID;
    TS_PMT.VS_info_length_u10  = 0x06;
    TS_PMT.VS_descriptor_u8[0] = 0x28;
    TS_PMT.VS_descriptor_u8[1] = 0x04;
    TS_PMT.VS_descriptor_u8[2] = 0x42;
    TS_PMT.VS_descriptor_u8[3] = 0xC0;
    TS_PMT.VS_descriptor_u8[4] = 0x1F;
    TS_PMT.VS_descriptor_u8[5] = 0xBF;
    TS_PMT.AS_type_u8 = 0x80; // stream type user private
    TS_PMT.AS_PID_u13 = TS_AUDIO_PID;
    TS_PMT.AS_info_length_u10  = 0x06;
    // magical semi-standard PCM registration descriptor
    TS_PMT.AS_descriptor_u8[0] = 0x05;
    TS_PMT.AS_descriptor_u8[1] = 0x04;
    TS_PMT.AS_descriptor_u8[2] = 0x48;
    TS_PMT.AS_descriptor_u8[3] = 0x44;
    TS_PMT.AS_descriptor_u8[4] = 0x4D;
    TS_PMT.AS_descriptor_u8[5] = 0x56;
    // put signals into packet
    pos = mux_TS_header();
    TS_packet[pos]    =  TS_PMT.pointer_field_u8;
    TS_packet[pos+1]  =  TS_PMT.table_id_u8;
    TS_packet[pos+2]  = (TS_PMT.section_syntax_indicator_u1<<0x07) \
                      | (TS_PMT.section_length_u10>>0x08) \
                      | 0x30;
    TS_packet[pos+3]  =  TS_PMT.section_length_u10;
    pos+=0x04;
    section_start = pos;
    TS_packet[pos]    = (TS_PMT.program_number_u16>>0x08);
    TS_packet[pos+1]  =  TS_PMT.program_number_u16;
    TS_packet[pos+2]  = (TS_PMT.version_number_u5<<0x01) \
                      | TS_PMT.current_next_indicator_u1 \
                      | 0xC0;
    TS_packet[pos+3]  =  TS_PMT.section_number_u8;
    TS_packet[pos+4]  =  TS_PMT.last_section_number_u8;
    TS_packet[pos+5]  = (TS_PMT.PCR_PID_u13>>0x08) \
                      | 0xE0;
    TS_packet[pos+6]  =  TS_PMT.PCR_PID_u13;
    TS_packet[pos+7]  = (TS_PMT.program_info_length_u10>>0x08)
                      | 0xF0;
    TS_packet[pos+8]  =  TS_PMT.program_info_length_u10;
    pos+=0x09;
    for (i=0; i<TS_PMT.program_info_length_u10; i++) TS_packet[pos+i] = TS_PMT.program_descriptor_u8[i];
    pos+=TS_PMT.program_info_length_u10;
    TS_packet[pos]    =  TS_PMT.VS_type_u8;
    TS_packet[pos+1]  = (TS_PMT.VS_PID_u13>>0x08) \
                      | 0xE0;
    TS_packet[pos+2]  =  TS_PMT.VS_PID_u13;
    TS_packet[pos+3]  = (TS_PMT.VS_info_length_u10>>0x08);
    TS_packet[pos+4]  =  TS_PMT.VS_info_length_u10;
    pos+=0x05;
    for (i=0; i<TS_PMT.VS_info_length_u10; i++) TS_packet[pos+i] = TS_PMT.VS_descriptor_u8[i];
    pos+=TS_PMT.VS_info_length_u10;
    TS_packet[pos]    =  TS_PMT.AS_type_u8;
    TS_packet[pos+1]  = (TS_PMT.AS_PID_u13>>0x08) \
                      | 0xE0;
    TS_packet[pos+2]  =  TS_PMT.AS_PID_u13;
    TS_packet[pos+3]  = (TS_PMT.AS_info_length_u10>>0x08);
    TS_packet[pos+4]  =  TS_PMT.AS_info_length_u10;
    pos+=0x05;
    for (i=0; i<TS_PMT.AS_info_length_u10; i++) TS_packet[pos+i] = TS_PMT.AS_descriptor_u8[i];
    pos+=TS_PMT.AS_info_length_u10;
    // add padding
    section_start = section_start + TS_PMT.section_length_u10 - 4;
    while (pos<section_start)
    {
        TS_packet[pos] = 0xFF;
        pos++;
    }
    // add crc
    TS_PMT.crc_u32   = crc32(0xFFFFFFFF, &TS_packet[5], (pos-0x05));
    TS_packet[pos]   = (TS_PMT.crc_u32>>0x18);
    TS_packet[pos+1] = (TS_PMT.crc_u32>>0x10);
    TS_packet[pos+2] = (TS_PMT.crc_u32>>0x08);
    TS_packet[pos+3] =  TS_PMT.crc_u32;
    pos+=0x04;
    // limit size
    if (pos > TS_PCK_SIZE) pos = TS_PCK_SIZE;
    while (pos<TS_PCK_SIZE)
    {
        TS_packet[pos] = 0xFF;
        pos++;
    }
    return pos;
}

unsigned int mux_PCR_packet()
{
    unsigned int i, pos, section_start;

    // prepare header
    TS_header.sync_byte_u8 = 0x47;
    TS_header.error_indicator_u1 = 0x00;
    TS_header.payload_start_indicator_u1 = 0x00;
    TS_header.transport_priority_u1 = 0x00;
    TS_header.PID_u13 = TS_PMT.PCR_PID_u13;
    TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
    TS_header.adaptation_field_control_u2 = TS_AFC_ADP_NO_PYLD;
    TS_header.continuity_counter_u4 = TS_PCR_counter;
    TS_header.adaptation_field_length_u8 = 0xB7;
    TS_header.discontinuity_indicator_u1 = 0x00;
    TS_header.random_access_indicator_u1 = 0x00;
    TS_header.priority_indicator_u1 = 0x00;
    TS_header.PCR_flag_u1 = 0x01;
    TS_header.OPCR_flag_u1 = 0x00;
    TS_header.splice_point_flag_u1 = 0x00;
    TS_header.private_data_flag_u1 = 0x00;
    TS_header.extension_flag_u1 = 0x00;
    // TODO tie to usb timer
    TS_header.PCR_u32 = (unsigned int)TS_timer;
    TS_header.xPCR_u9 = 0x00;
    TS_PCR_counter = (TS_PCR_counter+0x01)&0x0F;

    // put signals into packet
    pos = mux_TS_header();
    // limit size
    if (pos > TS_PCK_SIZE) pos = TS_PCK_SIZE;
    while (pos<TS_PCK_SIZE)
    {
        TS_packet[pos] = 0xFF;
        pos++;
    }
    return pos;
}

void mux_Init(void)
{
    // reset state machine
    TS_VS_state = 0x04;
    TS_AS_state = 0x04;
    // reset counters
    TS_VS_counter = 0;
    TS_AS_counter = 0;
    TS_PAT_counter = 0;
    TS_PMT_counter = 0;
    TS_PCR_counter = 0;
    // clear data structures
    TS_PAT.program_PID_u13 = 0;
    TS_PMT.VS_PID_u13 = 0;
    TS_PMT.AS_PID_u13 = 0;
}

void mux_Handler(void)
{
    TS_timer += 5.625;
}

// muxer function
// input: buffer address, bytes targeted to encode
// output: header size
unsigned int mux_VS_packet(unsigned int dst, unsigned int length)
{
    unsigned int pos = TS_PCK_SIZE-TS_PCK_HEADER_SIZE-1;

    TS_packet = (unsigned char *)dst;
    switch (TS_VS_state)
    {
        // add a packet
        case 0:
        // prepare header
        TS_header.sync_byte_u8 = 0x47;
        TS_header.error_indicator_u1 = 0x00;
        TS_header.payload_start_indicator_u1 = 0x00;
        TS_header.transport_priority_u1 = 0x00;
        TS_header.PID_u13 = TS_PMT.VS_PID_u13;
        TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
        TS_header.adaptation_field_control_u2 = TS_AFC_NO_ADP_PYLD;
        TS_header.continuity_counter_u4 = TS_VS_counter;
        TS_header.adaptation_field_length_u8 = 0x00;
        TS_header.discontinuity_indicator_u1 = 0x00;
        TS_header.random_access_indicator_u1 = 0x00;
        TS_header.priority_indicator_u1 = 0x00;
        TS_header.PCR_flag_u1 = 0x00;
        TS_header.OPCR_flag_u1 = 0x00;
        TS_header.splice_point_flag_u1 = 0x00;
        TS_header.private_data_flag_u1 = 0x00;
        TS_header.extension_flag_u1 = 0x00;
        TS_VS_counter = (TS_VS_counter+0x01)&0x0F;
        // shorten packet from adaptation field for last packet
        if (length<=pos)
        {
            TS_header.adaptation_field_control_u2 = TS_AFC_ADP_PYLD;
            TS_header.adaptation_field_length_u8 = (pos-length);
            TS_VS_state = 0x04;
        }
        // add stream packet
        pos = mux_TS_header();
        break;
        // add PES
        case 1:
        // prepare header
        TS_header.sync_byte_u8 = 0x47;
        TS_header.error_indicator_u1 = 0x00;
        TS_header.payload_start_indicator_u1 = 0x01;
        TS_header.transport_priority_u1 = 0x00;
        TS_header.PID_u13 = TS_PMT.VS_PID_u13;
        TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
        TS_header.adaptation_field_control_u2 = TS_AFC_ADP_PYLD;
        TS_header.continuity_counter_u4 = TS_VS_counter;
        TS_header.adaptation_field_length_u8 = 0xA9; // set field length so only PES in packet
        TS_header.discontinuity_indicator_u1 = 0x00;
        TS_header.random_access_indicator_u1 = 0x00;
        TS_header.priority_indicator_u1 = 0x00;
        TS_header.PCR_flag_u1 = 0x00;
        TS_header.OPCR_flag_u1 = 0x00;
        TS_header.splice_point_flag_u1 = 0x00;
        TS_header.private_data_flag_u1 = 0x00;
        TS_header.extension_flag_u1 = 0x00;
        TS_VS_counter = (TS_VS_counter+0x01)&0x0F;
        // prepare PES packet
        TS_PES.start_code_prefix_u24 = 0x000001;
        TS_PES.stream_id_u8 = 0xE0;
        TS_PES.packet_length_u16 = 0x00;
        TS_PES.ext.scrambling_control_u2 = 0x00;
        TS_PES.ext.priority_u1 = 0x00;
        TS_PES.ext.data_alignment_indicator_u1 = 0x00;
        TS_PES.ext.copyright_u1 = 0x00;
        TS_PES.ext.original_u1 = 0x01;
        TS_PES.ext.PTS_DTS_flags_u2 = 0x02;
        TS_PES.ext.ESCR_flag_u1 = 0x00;
        TS_PES.ext.ES_rate_flag_u1 = 0x00;
        TS_PES.ext.DSM_trick_flag_u1 = 0x00;
        TS_PES.ext.extra_copy_info_flag_u1 = 0x00;
        TS_PES.ext.CRC_flag_u1 = 0x00;
        TS_PES.ext.extension_flag_u1 = 0x00;
        TS_PES.ext.header_data_length_u8 = 0x05;
        TS_PES.ext.PTS_u32 = (unsigned int)TS_timer;
        // put signals into packet
        pos = mux_TS_header();
        TS_packet[pos]   = (TS_PES.start_code_prefix_u24>>0x10);
        TS_packet[pos+1] = (TS_PES.start_code_prefix_u24>>0x08);
        TS_packet[pos+2] =  TS_PES.start_code_prefix_u24;
        TS_packet[pos+3] =  TS_PES.stream_id_u8;
        TS_packet[pos+4] = (TS_PES.packet_length_u16>>0x08);
        TS_packet[pos+5] =  TS_PES.packet_length_u16;
        TS_packet[pos+6] = (TS_PES.ext.scrambling_control_u2<<0x04) \
                         | (TS_PES.ext.priority_u1<<0x03) \
                         | (TS_PES.ext.data_alignment_indicator_u1<<0x02) \
                         | (TS_PES.ext.copyright_u1<<0x01) \
                         | TS_PES.ext.original_u1 \
                         | 0x80;
        TS_packet[pos+7] = (TS_PES.ext.PTS_DTS_flags_u2<<0x06) \
                         | (TS_PES.ext.ESCR_flag_u1<<0x05) \
                         | (TS_PES.ext.ES_rate_flag_u1<<0x04) \
                         | (TS_PES.ext.DSM_trick_flag_u1<<0x03) \
                         | (TS_PES.ext.extra_copy_info_flag_u1<<0x02) \
                         | (TS_PES.ext.CRC_flag_u1<<0x01) \
                         | TS_PES.ext.extension_flag_u1;
        TS_packet[pos+8] =  TS_PES.ext.header_data_length_u8;
        pos+= 0x09;
        if (TS_PES.ext.PTS_DTS_flags_u2 == 0x02)
        {
            TS_packet[pos]   = ((TS_PES.ext.PTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.PTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.PTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.PTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.PTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
        }
        if (TS_PES.ext.PTS_DTS_flags_u2 == 0x03)
        {
            TS_packet[pos]   = ((TS_PES.ext.PTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.PTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.PTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.PTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.PTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
            TS_packet[pos]   = ((TS_PES.ext.DTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.DTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.DTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.DTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.DTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
        }
        // TODO add support for ESCR, ES rate
        // mimic uvc EOF
        pos += 0x200;
        TS_VS_state = 0x00;
        break;
        // add PCR
        case 2:
        pos = mux_PCR_packet();
        TS_VS_state = 0x01;
        break;
        // add PMT
        case 3:
        pos = mux_PMT_packet();
        TS_VS_state = 0x02;
        break;
        // add a PAT
        default:
        pos = mux_PAT_packet();
        TS_VS_state = 0x03;
        break;
    }
    pos += 0x10000;
    // mimic stream ID
    return pos;
}

// muxer function
// input: buffer address, bytes targeted to encode
// output: header size
unsigned int mux_AS_packet(unsigned int dst, unsigned int length)
{
    unsigned int pos = TS_PCK_SIZE-TS_PCK_HEADER_SIZE-1;

    TS_packet = (unsigned char *)dst;
    switch (TS_AS_state)
    {
        // add a packet
        case 0:
        // prepare header
        TS_header.sync_byte_u8 = 0x47;
        TS_header.error_indicator_u1 = 0x00;
        TS_header.payload_start_indicator_u1 = 0x00;
        TS_header.transport_priority_u1 = 0x00;
        TS_header.PID_u13 = TS_PMT.AS_PID_u13;
        TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
        TS_header.adaptation_field_control_u2 = TS_AFC_NO_ADP_PYLD;
        TS_header.continuity_counter_u4 = TS_AS_counter;
        TS_header.adaptation_field_length_u8 = 0x00;
        TS_header.discontinuity_indicator_u1 = 0x00;
        TS_header.random_access_indicator_u1 = 0x00;
        TS_header.priority_indicator_u1 = 0x00;
        TS_header.PCR_flag_u1 = 0x00;
        TS_header.OPCR_flag_u1 = 0x00;
        TS_header.splice_point_flag_u1 = 0x00;
        TS_header.private_data_flag_u1 = 0x00;
        TS_header.extension_flag_u1 = 0x00;
        TS_AS_counter = (TS_AS_counter+0x01)&0x0F;
        // shorten packet from adaptation field for last packet
        if (length<pos)
        {
            TS_header.adaptation_field_control_u2 = TS_AFC_ADP_PYLD;
            TS_header.adaptation_field_length_u8 = (pos-length);
            TS_AS_state = 0x01;
        }
        // add stream packet
        pos = mux_TS_header();
        break;
        // add PES
        default:
        // prepare header
        TS_header.sync_byte_u8 = 0x47;
        TS_header.error_indicator_u1 = 0x00;
        TS_header.payload_start_indicator_u1 = 0x01;
        TS_header.transport_priority_u1 = 0x00;
        TS_header.PID_u13 = TS_PMT.AS_PID_u13;
        TS_header.scrambling_control_u2 = TS_SCRAMBLE_NONE;
        TS_header.adaptation_field_control_u2 = TS_AFC_NO_ADP_PYLD;
        TS_header.continuity_counter_u4 = TS_AS_counter;
        TS_header.adaptation_field_length_u8 = 0x00; // set field length to 0 audio frames are fixed size
        TS_header.discontinuity_indicator_u1 = 0x00;
        TS_header.random_access_indicator_u1 = 0x00;
        TS_header.priority_indicator_u1 = 0x00;
        TS_header.PCR_flag_u1 = 0x00;
        TS_header.OPCR_flag_u1 = 0x00;
        TS_header.splice_point_flag_u1 = 0x00;
        TS_header.private_data_flag_u1 = 0x00;
        TS_header.extension_flag_u1 = 0x00;
        TS_AS_counter = (TS_AS_counter+0x01)&0x0F;
        // prepare PES packet
        TS_PES.start_code_prefix_u24 = 0x000001;
        TS_PES.stream_id_u8 = 0xBD;
        TS_PES.packet_length_u16 = (length+0x0C); // 960 bytes per 'frame' 8 bytes extension 4 bytes PCM 'header'
        TS_PES.ext.scrambling_control_u2 = 0x00;
        TS_PES.ext.priority_u1 = 0x00;
        TS_PES.ext.data_alignment_indicator_u1 = 0x01;
        TS_PES.ext.copyright_u1 = 0x00;
        TS_PES.ext.original_u1 = 0x00;
        TS_PES.ext.PTS_DTS_flags_u2 = 0x02;
        TS_PES.ext.ESCR_flag_u1 = 0x00;
        TS_PES.ext.ES_rate_flag_u1 = 0x00;
        TS_PES.ext.DSM_trick_flag_u1 = 0x00;
        TS_PES.ext.extra_copy_info_flag_u1 = 0x00;
        TS_PES.ext.CRC_flag_u1 = 0x00;
        TS_PES.ext.extension_flag_u1 = 0x00;
        TS_PES.ext.header_data_length_u8 = 0x05;
        TS_PES.ext.PTS_u32 = (unsigned int)TS_timer;
        // put signals into packet
        pos = mux_TS_header();
        TS_packet[pos]   = (TS_PES.start_code_prefix_u24>>0x10);
        TS_packet[pos+1] = (TS_PES.start_code_prefix_u24>>0x08);
        TS_packet[pos+2] =  TS_PES.start_code_prefix_u24;
        TS_packet[pos+3] =  TS_PES.stream_id_u8;
        TS_packet[pos+4] = (TS_PES.packet_length_u16>>0x08);
        TS_packet[pos+5] =  TS_PES.packet_length_u16;
        TS_packet[pos+6] = (TS_PES.ext.scrambling_control_u2<<0x04) \
                         | (TS_PES.ext.priority_u1<<0x03) \
                         | (TS_PES.ext.data_alignment_indicator_u1<<0x02) \
                         | (TS_PES.ext.copyright_u1<<0x01) \
                         | TS_PES.ext.original_u1 \
                         | 0x80;
        TS_packet[pos+7] = (TS_PES.ext.PTS_DTS_flags_u2<<0x06) \
                         | (TS_PES.ext.ESCR_flag_u1<<0x05) \
                         | (TS_PES.ext.ES_rate_flag_u1<<0x04) \
                         | (TS_PES.ext.DSM_trick_flag_u1<<0x03) \
                         | (TS_PES.ext.extra_copy_info_flag_u1<<0x02) \
                         | (TS_PES.ext.CRC_flag_u1<<0x01) \
                         | TS_PES.ext.extension_flag_u1;
        TS_packet[pos+8] =  TS_PES.ext.header_data_length_u8;
        pos+= 0x09;
        if (TS_PES.ext.PTS_DTS_flags_u2 == 0x02)
        {
            TS_packet[pos]   = ((TS_PES.ext.PTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.PTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.PTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.PTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.PTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
        }
        if (TS_PES.ext.PTS_DTS_flags_u2 == 0x03)
        {
            TS_packet[pos]   = ((TS_PES.ext.PTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.PTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.PTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.PTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.PTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
            TS_packet[pos]   = ((TS_PES.ext.DTS_u32>>0x1C)&0x0E) | 0x21;
            TS_packet[pos+1] = (TS_PES.ext.DTS_u32>>0x15);
            TS_packet[pos+2] = ((TS_PES.ext.DTS_u32>>0x0D)&0xFE) | 0x01;
            TS_packet[pos+3] = (TS_PES.ext.DTS_u32>>0x06);
            TS_packet[pos+4] = ((TS_PES.ext.DTS_u32<<0x02)&0xFE) | 0x01;
            pos+=0x05;
        }
        // TODO add support for ESCR, ES rate
        // add PCM specific header for m2ts
        // size in bytes = 16 bits
        TS_packet[pos++] = (length>>0x08);
        TS_packet[pos++] = length;
        // channel assigment = 4 bits
        // sampling frequency = 4 bits
        // 3 stereo,  1 48 kHz
        TS_packet[pos++] = 0x31;
        // bits per sample = 2 bits
        // start flag = 1 bits
        // reserved = 5 bits
        // 1 16 bits, 1 start flag
        TS_packet[pos++] = 0x60;
        // mimic uvc EOF
        pos += 0x200;
        TS_AS_state = 0x00;
        break;
    }
    return pos;
}

unsigned int demux_TS_header()
{
    unsigned int i, pos, section_start;

    // decode into local data structure
    TS_header.sync_byte_u8 = TS_packet[0];
    TS_header.error_indicator_u1 = (TS_packet[1]>>0x07)&0x01;
    TS_header.payload_start_indicator_u1 = (TS_packet[1]>>0x06)&0x01;
    TS_header.transport_priority_u1 = (TS_packet[1]>>0x05)&0x01;
    TS_header.PID_u13 = ((TS_packet[1]<<0x08)&0x1F00) + TS_packet[2];
    TS_header.scrambling_control_u2 = (TS_packet[3]>>6)&0x03;
    TS_header.adaptation_field_control_u2 = (TS_packet[3]>>4)&0x03;
    TS_header.continuity_counter_u4 = (TS_packet[3]>>0)&0x0F;
    pos = 0x04;
    // see if adaptation field present
    if ( (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_NO_PYLD) || (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_PYLD) )
    {
        TS_header.adaptation_field_length_u8 = TS_packet[pos];
        pos++;
        section_start = pos;
        // read adaptation flags and optional data
        if (TS_header.adaptation_field_length_u8 != 0)
        {
            TS_header.discontinuity_indicator_u1 = (TS_packet[pos]>>0x07)&0x01;
            TS_header.random_access_indicator_u1 = (TS_packet[pos]>>0x06)&0x01;
            TS_header.priority_indicator_u1 = (TS_packet[pos]>>0x05)&0x01;
            TS_header.PCR_flag_u1  = (TS_packet[pos]>>0x04)&0x01;
            TS_header.OPCR_flag_u1 = (TS_packet[pos]>>0x03)&0x01;
            TS_header.splice_point_flag_u1 = (TS_packet[pos]>>0x02)&0x01;
            TS_header.private_data_flag_u1 = (TS_packet[pos]>>0x01)&0x01;
            TS_header.extension_flag_u1 = (TS_packet[pos]>>0x00)&0x01;
            pos++;
            // support for PCR
            if (TS_header.PCR_flag_u1 != 0)
            {
                TS_header.PCR_u32 = (TS_packet[pos]<<0x18) + (TS_packet[pos+1]<<0x10) + (TS_packet[pos+2]<<0x08) + TS_packet[pos+3];
                TS_header.xPCR_u9 = ((TS_packet[pos+4]<<0x08)&0x0100) + TS_packet[pos+5];
                pos+=0x06;
            }
            // support for OPCR
            if (TS_header.OPCR_flag_u1 != 0)
            {
                TS_header.OPCR_u32 = (TS_packet[pos]<<0x18) + (TS_packet[pos+1]<<0x10) + (TS_packet[pos+2]<<0x08) + TS_packet[pos+3];
                TS_header.xOPCR_u9 = ((TS_packet[pos+4]<<0x08)&0x0100) + TS_packet[pos+5];
                pos+=0x06;
            }
            // support for splicing point
            if (TS_header.splice_point_flag_u1 != 0)
            {
                TS_header.splice_countdown_u8 = TS_packet[pos];
                pos++;
            }
            // support for private data
            if (TS_header.private_data_flag_u1 != 0)
            {
                TS_header.private_length_u8 = TS_packet[pos];
                pos++;
                for (i=0; i<TS_header.private_length_u8; i++)
                {
                    if (i<TS_PCK_DATA_SIZE) TS_header.private_data_u8[i] = TS_packet[pos+i];
                }
                pos+=TS_header.private_length_u8;
            }
            // final position
            pos = section_start + TS_header.adaptation_field_length_u8;
        }
    }
    // limit size
    if (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_NO_PYLD) pos = TS_PCK_SIZE;
    if (pos > TS_PCK_SIZE) pos = TS_PCK_SIZE;
    return pos;
}

unsigned int demux_PAT_packet()
{
    unsigned int i, pos, section_start;

    // decode header
    pos = demux_TS_header();
    // update data structure
    TS_PAT.pointer_field_u8 = TS_packet[pos];
    TS_PAT.table_id_u8 = TS_packet[pos+1];
    TS_PAT.section_syntax_indicator_u1 = (TS_packet[pos+2]>>0x07)&0x01;
    TS_PAT.section_length_u10 = ((TS_packet[pos+2]<<0x08)&0x300) | TS_packet[pos+3];
    pos+=0x04;
    section_start = pos;
    TS_PAT.ts_id_u16 = (TS_packet[pos]<<0x08) | TS_packet[pos+1];
    TS_PAT.version_number_u5 = (TS_packet[pos+2]>>0x01)&0x1F;
    TS_PAT.current_next_indicator_u1 = TS_packet[pos+2]&0x01;
    TS_PAT.section_number_u8 = TS_packet[pos+3];
    TS_PAT.last_section_number_u8 = TS_packet[pos+4];
    // read first program
    TS_PAT.program_number_u16 = (TS_packet[pos+5]<<0x08) | TS_packet[pos+6];
    TS_PAT.program_PID_u13 = ((TS_packet[pos+7]<<0x08)&0x1F00) | TS_packet[pos+8];
    pos = section_start + TS_PAT.section_length_u10;
    // discard rest of packet
    pos = TS_PCK_SIZE;
    return pos;
}

unsigned int demux_PMT_packet()
{
    unsigned int i, pos, section_start;

    // decode header
    pos = demux_TS_header();
    // check if the correct program
    if (TS_header.PID_u13 == TS_PAT.program_PID_u13)
    {
        // update data structure
        TS_PMT.pointer_field_u8 = TS_packet[pos];
        TS_PMT.table_id_u8 = TS_packet[pos+1];
        TS_PMT.section_syntax_indicator_u1 = (TS_packet[pos+2]>>0x07)&0x01;
        TS_PMT.section_length_u10 = ((TS_packet[pos+2]<<0x08)&0x300) | TS_packet[pos+3];
        pos+=0x04;
        section_start = pos;
        TS_PMT.program_number_u16 = (TS_packet[pos]<<0x08) | TS_packet[pos+1];
        TS_PMT.version_number_u5 = (TS_packet[pos+2]>>0x01)&0x1F;
        TS_PMT.current_next_indicator_u1 = TS_packet[pos+2]&0x01;
        TS_PMT.section_number_u8 = TS_packet[pos+3];
        TS_PMT.last_section_number_u8 = TS_packet[pos+4];
        TS_PMT.PCR_PID_u13 = ((TS_packet[pos+5]<<0x08)&0x1F00) | TS_packet[pos+6];
        TS_PMT.program_info_length_u10 = ((TS_packet[pos+7]<<0x08)&0x300) | TS_packet[pos+8];
        pos+=0x09;
        for (i=0; i<TS_PMT.program_info_length_u10; i++) TS_PMT.program_descriptor_u8[i] = TS_packet[pos+i];
        pos+=TS_PMT.program_info_length_u10;
        // parse to identify an audio and a video stream
        TS_PMT.VS_PID_u13 = 0;
        TS_PMT.AS_PID_u13 = 0;
        while  (pos<(TS_PMT.section_length_u10+section_start-4))
        {
            // see if AVC video stream
            if (TS_packet[pos] == 0x1B)
            {
                TS_PMT.VS_type_u8 = TS_packet[pos];
                TS_PMT.VS_PID_u13 = ((TS_packet[pos+1]<<0x08)&0x1F00) + TS_packet[pos+2];
                TS_PMT.VS_info_length_u10 = ((TS_packet[pos+3]<<8)&0x300) | (TS_packet[pos+4]);
                pos+=0x05;
                for (i=0; i<TS_PMT.VS_info_length_u10; i++) TS_PMT.VS_descriptor_u8[i] = TS_packet[pos+i];
                pos+=TS_PMT.VS_info_length_u10;
            } else
            // see if PCM audio stream
            if (TS_packet[pos] == 0x80)
            {
                TS_PMT.AS_type_u8 = TS_packet[pos];
                TS_PMT.AS_PID_u13 = ((TS_packet[pos+1]<<0x08)&0x1F00) + TS_packet[pos+2];
                TS_PMT.AS_info_length_u10 = ((TS_packet[pos+3]<<8)&0x300) | (TS_packet[pos+4]);
                pos+=0x05;
                for (i=0; i<TS_PMT.AS_info_length_u10; i++) TS_PMT.AS_descriptor_u8[i] = TS_packet[pos+i];
                pos+=TS_PMT.AS_info_length_u10;
            } else
            // not supported stream type
            {
                i = ((TS_packet[pos+3]<<8)&0x300) | (TS_packet[pos+4]);
                pos += (i+5);
            }
        }
        pos = section_start + TS_PMT.section_length_u10;
    }
    // discard rest of packet
    pos = TS_PCK_SIZE;
    return pos;
}

// demuxer function
// input: buffer address, bytes targeted to decode
// output: header size
unsigned int mux_TS_packet(unsigned int src, unsigned int length)
{
    unsigned int pos, section_start;

    TS_packet = (unsigned char *)src;
    // decode header
    pos = demux_TS_header();
    // decode PAT
    if (TS_header.PID_u13 == 0)
    {
        pos = demux_PAT_packet();
    } else
    // decode PMT
    if (TS_header.PID_u13 == TS_PAT.program_PID_u13)
    {
        pos = demux_PMT_packet();
    } else
    // decode audio stream
    if (TS_header.PID_u13 == TS_PMT.AS_PID_u13)
    {
        // check if PES
        if (TS_header.payload_start_indicator_u1)
        {
            // update data structure
            TS_PES.start_code_prefix_u24 = (TS_packet[pos]<<0x10) + (TS_packet[pos+1]<<0x08) + TS_packet[pos+2];
            TS_PES.stream_id_u8 = TS_packet[pos+3];
            TS_PES.packet_length_u16 = (TS_packet[pos+4]<<0x08) + TS_packet[pos+5];
            TS_PES.ext.scrambling_control_u2 = (TS_packet[pos+6]>>0x04)&0x03;
            TS_PES.ext.priority_u1 = (TS_packet[pos+6]>>0x03)&0x01;
            TS_PES.ext.data_alignment_indicator_u1 = (TS_packet[pos+6]>>0x02)&0x01;
            TS_PES.ext.copyright_u1 = (TS_packet[pos+6]>>0x01)&0x01;
            TS_PES.ext.original_u1 = TS_packet[pos+6]&0x01;
            TS_PES.ext.PTS_DTS_flags_u2 = (TS_packet[pos+7]>>0x06)&0x03;
            TS_PES.ext.ESCR_flag_u1 = (TS_packet[pos+7]>>0x05)&0x01;
            TS_PES.ext.ES_rate_flag_u1 = (TS_packet[pos+7]>>0x04)&0x01;
            TS_PES.ext.DSM_trick_flag_u1 = (TS_packet[pos+7]>>0x03)&0x01;
            TS_PES.ext.extra_copy_info_flag_u1 = (TS_packet[pos+7]>>0x02)&0x01;
            TS_PES.ext.CRC_flag_u1 = (TS_packet[pos+7]>>0x01)&0x01;
            TS_PES.ext.extension_flag_u1 = TS_packet[pos+7]&0x01;
            TS_PES.ext.header_data_length_u8 = TS_packet[pos+8];
            pos+=0x09;
            section_start=pos;
            // discard optional data
            pos = section_start + TS_PES.ext.header_data_length_u8;
            // skip PCM header ;-)
            pos+=0x04;
        }
    } else
    // decode video stream
    if (TS_header.PID_u13 == TS_PMT.VS_PID_u13)
    {
        // check if PES
        if (TS_header.payload_start_indicator_u1 != 0)
        {
            // update data structure
            TS_PES.start_code_prefix_u24 = (TS_packet[pos]<<0x10) + (TS_packet[pos+1]<<0x08) + TS_packet[pos+2];
            TS_PES.stream_id_u8 = TS_packet[pos+3];
            TS_PES.packet_length_u16 = (TS_packet[pos+4]<<0x08) + TS_packet[pos+5];
            TS_PES.ext.scrambling_control_u2 = (TS_packet[pos+6]>>0x04)&0x03;
            TS_PES.ext.priority_u1 = (TS_packet[pos+6]>>0x03)&0x01;
            TS_PES.ext.data_alignment_indicator_u1 = (TS_packet[pos+6]>>0x02)&0x01;
            TS_PES.ext.copyright_u1 = (TS_packet[pos+6]>>0x01)&0x01;
            TS_PES.ext.original_u1 = TS_packet[pos+6]&0x01;
            TS_PES.ext.PTS_DTS_flags_u2 = (TS_packet[pos+7]>>0x06)&0x03;
            TS_PES.ext.ESCR_flag_u1 = (TS_packet[pos+7]>>0x05)&0x01;
            TS_PES.ext.ES_rate_flag_u1 = (TS_packet[pos+7]>>0x04)&0x01;
            TS_PES.ext.DSM_trick_flag_u1 = (TS_packet[pos+7]>>0x03)&0x01;
            TS_PES.ext.extra_copy_info_flag_u1 = (TS_packet[pos+7]>>0x02)&0x01;
            TS_PES.ext.CRC_flag_u1 = (TS_packet[pos+7]>>0x01)&0x01;
            TS_PES.ext.extension_flag_u1 = TS_packet[pos+7]&0x01;
            TS_PES.ext.header_data_length_u8 = TS_packet[pos+8];
            pos+=0x09;
            section_start=pos;
            // discard optional data
            pos = section_start + TS_PES.ext.header_data_length_u8;
        }
        // ugly solution for EOF
        if ( (TS_header.payload_start_indicator_u1 == 0) && (TS_header.adaptation_field_control_u2 == TS_AFC_ADP_PYLD) )
        {
            pos += 0x200;
        }
        // mimic secondary stream id selector
        pos += 0x10000;
    } else
    // discard packet
    {
        pos = TS_PCK_SIZE;
    }
    return pos;
}

