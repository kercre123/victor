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


#include "usb.h"
#include "usbDefines.h"

// API visible to system manager, initialize structure and signal data base
volatile usb_ctrl_t __attribute__((aligned(64))) usb_ctrl =
{
    .deviceStat = 0,
    .deviceClock = 0,
    .interfaceAlter03 = 0,
    .interfaceAlter47 = 0,
};

endpoint_desc_t __attribute__((aligned(64))) usb_control_out_endpoint =
{
    .bLength = sizeof(endpoint_desc_t),
    .bDescriptorType = ENDPOINT,
    .bEndpointAddress = 0x00,
    .bmAttributes = (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
    .wMaxPacketSize = 0x40,
    .bInterval = 0x01,
};

endpoint_desc_t __attribute__((aligned(64))) usb_control_in_endpoint =
{
    .bLength = sizeof(endpoint_desc_t),
    .bDescriptorType = ENDPOINT,
    .bEndpointAddress = 0x80,
    .bmAttributes = (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
    .wMaxPacketSize = 0x40,
    .bInterval = 0x01,
};

unsigned int __attribute__((aligned(4))) dma_addr, svu_addr;
//unsigned int *usb_dbg;

volatile unsigned int __attribute__((aligned(64)))   dTD_IN[32];
volatile unsigned int __attribute__((aligned(64)))   dTD_OUT[32];
volatile unsigned int __attribute__((aligned(64)))   dTD_DIO[32];
volatile unsigned int __attribute__((aligned(1024))) dTD[32][8];
volatile unsigned int __attribute__((aligned(2048))) dQH[32][16];


// local functions
void USB_Init(void);
void USB_SetupMessage(void);
void USB_StreamIsoIN (volatile usb_stream_t*);
void USB_StreamIsoOUT(volatile usb_stream_t*);
void USB_StreamIN    (volatile usb_stream_t*);
void USB_StreamOUT   (volatile usb_stream_t*);
// callback functions from config
extern void USB_InitCallback(void);
extern void USB_HandlerCallback(void);
extern unsigned int USB_SetupCallback(unsigned int, unsigned int, unsigned int);

// global functions
void USB_Main(void)
//int main(void)
{
    unsigned int ep, fi, id, dma_stat;

    // init phase
    USB_Init();
    USB_InitCallback();
    //usb_dbg = (unsigned int *)0x100A0000;
    // use SOF to detect
    fi = *(unsigned int *)(USB_BASE_ADDR + REG_FRINDEX);
    usb_ctrl.deviceStat = (usb_ctrl.deviceStat&0xFFFF)|(fi<<0x10);
    // handler phase
    while (1)
    {
        // check for SOF
        __asm__ __volatile__("CMU.CPTI %0, 0x1E \n"
                             "IAU.FEXT.u32 %0, %0, 0x07, 0x01 \n"
                            :"=r"(id));
        if (id != 0)
        {
            // clear SOF
            *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS) = USB_USBSTS_SRI;
            //while ( (*(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS) & USB_USBSTS_SRI) != 0 );
            *(volatile unsigned int*)ICB_INT_CLEAR = (1<<IRQ_USB);
			// increment counter
            usb_ctrl.deviceClock++;
            // handle standard USB control messages
            USB_SetupMessage();
            // video class specific interrupt handling callback
            USB_HandlerCallback();
            // read frame index
            fi = *(unsigned int *)(USB_BASE_ADDR + REG_FRINDEX);
            usb_ctrl.deviceStat = (usb_ctrl.deviceStat&0xFFFF)|(fi<<0x10);
        } else
        {
            // delay 25usec
            __asm__ __volatile__("LSU1.LDIL i0, 0x400 \n"
                                 "BRU.RPI i0 \n"
                                :// no output
                                :// no input
                                :"i0" );
        }

        // see if new buffers available
        for (id=0; id<4; id++)
        {
            if ( (usb_ctrl.stream[id].PriAddr != 0) && (usb_ctrl.stream[id].PriAddrShadow == 0) )
            {
                fi = usb_ctrl.stream[id].PriLengthShadow;
                usb_ctrl.stream[id].PriLengthShadow = usb_ctrl.stream[id].PriLength;
                usb_ctrl.stream[id].PriLength = fi;
                fi = usb_ctrl.stream[id].PriAddrShadow;
                usb_ctrl.stream[id].PriAddrShadow = usb_ctrl.stream[id].PriAddr;
                usb_ctrl.stream[id].PriAddr = fi;
            }
            if ( (usb_ctrl.stream[id].SecAddr != 0) && (usb_ctrl.stream[id].SecAddrShadow == 0) )
            {
                fi = usb_ctrl.stream[id].SecLengthShadow;
                usb_ctrl.stream[id].SecLengthShadow = usb_ctrl.stream[id].SecLength;
                usb_ctrl.stream[id].SecLength = fi;
                fi = usb_ctrl.stream[id].SecAddrShadow;
                usb_ctrl.stream[id].SecAddrShadow = usb_ctrl.stream[id].SecAddr;
                usb_ctrl.stream[id].SecAddr = fi;
            }
        }

        // check dma status
        __asm__ __volatile__("CMU.CPTI %0, 0x1E \n"
                             "IAU.FEXT.u32 %0, %0, 0x03, 0x01 \n"
                            :"=r"(dma_stat));
        dma_addr = 0x0FF04000;
        if (dma_stat == 0)
        {
            for (id=0; id<4; id++)
            {
                ep = usb_ctrl.stream[id].Endpoint;
                fi = usb_ctrl.stream[id].Type;
                if (ep != 0)
                {
                    if ( ((ep&0x01) != 0) && ((fi&0xFF00) == 0) ) USB_StreamIsoIN(&usb_ctrl.stream[id]);
                    if ( ((ep&0x01) == 0) && ((fi&0xFF00) == 0) ) USB_StreamIsoOUT(&usb_ctrl.stream[id]);
                    if ( ((ep&0x01) != 0) && ((fi&0xFF00) != 0) ) USB_StreamIN(&usb_ctrl.stream[id]);
                    if ( ((ep&0x01) == 0) && ((fi&0xFF00) != 0) ) USB_StreamOUT(&usb_ctrl.stream[id]);
                }
            }
        }
        // dma program
        dma_stat = (dma_addr>>0x08)&0x07;
        if (dma_stat != 0)
        {
            dma_addr = 0x0FF04004;
            *(volatile unsigned int *)dma_addr = (dma_stat-1);
        }
    }
}

void USB_StreamIsoOUT(volatile usb_stream_t *stream)
{
    register unsigned int ep, *prepPtr, *waitPtr;
    register unsigned int dwAddr, dwLength, dwBuffer, dwPacket, dwHeader, dwInfo, dwPayload;

    ep = stream->Endpoint;
    prepPtr = (unsigned int *)dQH[ep][14];
    waitPtr = (unsigned int *)dQH[ep][15];
    // if next prepared transfer is not the usb transfer
    while (waitPtr != (unsigned int *)prepPtr[0])
    {
        prepPtr = (unsigned int *)prepPtr[0];
        dQH[ep][14] = (unsigned int)prepPtr;
        dQH[ep][12] = prepPtr[7];
        dQH[ep][13] = stream->Packet;
        USB_EPStream(ep);
    }
    // check completed transfers
    dwPayload = waitPtr[1];
    if ( ((dwPayload&0x80) == 0) && (stream->BufferLength == 0) )
    {
        // check for hw errors
        if ((dwPayload&0x0F) == 0)
        {
            // decode uvc header
            dwBuffer = waitPtr[7];
            dwPacket = stream->Packet;
            dwInfo   = stream->Callback(dwBuffer);
            dwHeader = dwInfo&0xFF;
            stream->BufferAddr = dwBuffer + dwHeader;
            dwHeader += (dwPayload>>0x10);
            if (dwHeader < dwPacket)
            {
                stream->BufferLength = dwPacket - dwHeader;
            } else
            {
                stream->BufferLength = 0;
            }
            // handle frame boundary
            if (dwInfo&0x200) stream->Type++;
        } else
        {
            // discard message
            stream->BufferLength = 0;
        }
        if (stream->BufferLength == 0)
        {
            // advance pointer
            waitPtr = (unsigned int *)waitPtr[0];
            dQH[ep][15] = (unsigned int)waitPtr;
        }
    }
    // decode piece by piece
    if (stream->BufferLength != 0)
    {
        dwBuffer  = stream->BufferAddr;
        dwPacket  = stream->MuxPacket;
        dwInfo    = stream->MuxCallback(dwBuffer);
        dwHeader  = dwInfo&0xFF;
        // determine targeted stream
        if (dwInfo > 0xFFFF)
        {
            dwAddr = stream->SecAddrShadow;
            dwLength = stream->SecLengthShadow;
        } else
        {
            dwAddr = stream->PriAddrShadow;
            dwLength = stream->PriLengthShadow;
        }
        dwPayload = dwPacket - dwHeader;
        if (stream->BufferLength<dwPayload) dwPayload = stream->BufferLength;
//        if (usb_dbg < (unsigned int *)0x100B0000)
//        {
//            *usb_dbg++ = stream->BufferAddr;
//            *usb_dbg++ = stream->BufferLength;
//            *usb_dbg++ = dwAddr;
//            *usb_dbg++ = waitPtr[1];
//        }
        // discard if no buffer available
        if (dwAddr != 0)
        {
            if (dwPayload != 0)
            {
                // DMA_Write();
                dma_addr += 0x100;
                *(volatile unsigned int *)(dma_addr+0x00) = 0x01;
                *(volatile unsigned int *)(dma_addr+0x04) = (dwBuffer + dwHeader);
                *(volatile unsigned int *)(dma_addr+0x08) = dwAddr;
                *(volatile unsigned int *)(dma_addr+0x0C) = dwPayload;
            }
            if (dwLength > dwPayload)
            {
                dwLength -= dwPayload;
            } else
            {
                dwLength = 0;
            }
            if (dwLength != 0)
            {
                dwAddr = dwAddr + dwPayload;
            } else
            {
                dwAddr = 0;
            }
        }
        stream->BufferAddr += (dwPayload + dwHeader);
        stream->BufferLength -= (dwPayload + dwHeader);
        // packet completion condition
        if (stream->BufferLength == 0)
        {
            if (stream->Type != 0)
            {
                stream->Type = 0;
                dwAddr = 0;
            }
            // advance pointer
            waitPtr = (unsigned int *)waitPtr[0];
            dQH[ep][15] = (unsigned int)waitPtr;
        }
        if (dwInfo > 0xFFFF)
        {
            stream->SecAddrShadow = dwAddr;
            stream->SecLengthShadow = dwLength;
        } else
        {
            stream->PriAddrShadow = dwAddr;
            stream->PriLengthShadow = dwLength;
        }
    }
}

void USB_StreamOUT(volatile usb_stream_t *stream)
{
    register unsigned int ep;
    register unsigned int dwAddr, dwLength, dwBuffer, dwPacket, dwHeader, dwInfo, dwPayload;

    ep = stream->Endpoint;

    // see if packet received and not decoding previous
    dwPayload = dTD[ep][1];
    if (((dwPayload&0x80) == 0) && (stream->BufferLength == 0))
    {
        dwBuffer = dQH[ep][14];
        dQH[ep][14] = dQH[ep][15];
        dQH[ep][15] = dwBuffer;
        // encode uvc header
        dwPacket = stream->Packet;
        dwInfo = stream->Callback(dwBuffer);
        if (dwInfo&0x200) stream->Type++;
        dwHeader = dwInfo&0xFF;
        stream->BufferAddr = dwBuffer + dwHeader;
        dwHeader += (dwPayload>>0x10);
        if (dwHeader < dwPacket)
        {
            stream->BufferLength = dwPacket - dwHeader;
        } else
        {
            stream->BufferLength = 0;
        }
        dQH[ep][12] = dQH[ep][14];
        dQH[ep][13] = dwPacket;
        // prime for next packet
        USB_EPSetup(ep);
    }
    // decode piece by piece
    if (stream->BufferLength != 0)
    {
        dwBuffer  = stream->BufferAddr;
        dwPacket  = stream->MuxPacket;
        dwInfo    = stream->MuxCallback(dwBuffer);
        dwHeader  = dwInfo&0xFF;
        // determine targeted stream
        if (dwInfo > 0xFFFF)
        {
            dwAddr = stream->SecAddrShadow;
            dwLength = stream->SecLengthShadow;
        } else
        {
            dwAddr = stream->PriAddrShadow;
            dwLength = stream->PriLengthShadow;
        }
        dwPayload = dwPacket - dwHeader;
        if (stream->BufferLength<dwPayload) dwPayload = stream->BufferLength;
        // stall pipeline
        if ( (dwAddr != 0) || (dwPayload == 0) )
        {
            if (dwPayload != 0)
            {
                // DMA_Write();
                dma_addr += 0x100;
                *(volatile unsigned int *)(dma_addr+0x00) = 0x01;
                *(volatile unsigned int *)(dma_addr+0x04) = (dwBuffer + dwHeader);
                *(volatile unsigned int *)(dma_addr+0x08) = dwAddr;
                *(volatile unsigned int *)(dma_addr+0x0C) = dwPayload;
            }
//            if (usb_dbg < (unsigned int *)0x100B0000)
//            {
//                *usb_dbg++ = stream->BufferAddr;
//                *usb_dbg++ = stream->BufferLength;
//                *usb_dbg++ = dwAddr;
//                *usb_dbg++ = dwInfo;
//            }
            if (dwLength > dwPayload)
            {
                dwLength -= dwPayload;
            } else
            {
                dwLength = 0;
            }
            if ( (dwLength != 0) && ((dwInfo&0x200) == 0) )
            {
                dwAddr = dwAddr + dwPayload;
            } else
            {
                dwAddr = 0;
            }
            stream->BufferAddr += (dwPayload + dwHeader);
            stream->BufferLength -= (dwPayload + dwHeader);
        }
        // packet completion condition
        if (stream->BufferLength == 0)
        {
            if (stream->Type != 0x100)
            {
                stream->Type = 0x100;
                dwAddr = 0;
            }
        }
        if (dwInfo > 0xFFFF)
        {
            stream->SecAddrShadow = dwAddr;
            stream->SecLengthShadow = dwLength;
        } else
        {
            stream->PriAddrShadow = dwAddr;
            stream->PriLengthShadow = dwLength;
        }
    }
}

void USB_StreamIsoIN(volatile usb_stream_t *stream)
{
    register unsigned int ep, *prepPtr, *waitPtr;
    register unsigned int dwAddr, dwLength, dwBuffer, dwPacket, dwHeader, dwInfo, dwPayload;

    ep = stream->Endpoint;
    prepPtr = (unsigned int *)dQH[ep][14];
    waitPtr = (unsigned int *)dQH[ep][15];
    // if next prepared transfer is not the waiting one
    if ( (waitPtr != (unsigned int *)prepPtr[0]) && (stream->BufferLength == 0) )
    {
        // setup current transfer
        USB_EPStream(ep);
        // advance pointer
        prepPtr = (unsigned int *)prepPtr[0];
        dQH[ep][14] = (unsigned int)prepPtr;
        // encode uvc header
        dwBuffer = prepPtr[7];
        dwPacket = stream->Packet;
        dwInfo = stream->Callback(dwBuffer);
        dwHeader = dwInfo&0xFF;
        stream->BufferAddr = dwBuffer + dwHeader;
        stream->BufferLength = dwPacket - dwHeader;
        dQH[ep][12] = dwBuffer;
        dQH[ep][13] = dwHeader;
    }
    // construct payload piece by piece
    if (stream->BufferLength != 0)
    {
        dwBuffer  = stream->BufferAddr;
        dwPacket  = stream->MuxPacket;
        if (stream->BufferLength < dwPacket ) dwPacket  = stream->BufferLength;
        dwInfo  = stream->MuxCallback(dwBuffer);
        dwHeader = dwInfo&0xFF;
        // determine targeted stream
        if (dwInfo > 0xFFFF)
        {
            dwAddr = stream->SecAddrShadow;
            dwLength = stream->SecLengthShadow;
        } else
        {
            dwAddr = stream->PriAddrShadow;
            dwLength = stream->PriLengthShadow;
        }
        dwPayload = dwPacket - dwHeader;
        if (dwLength < dwPayload) dwPayload = dwLength;
//        if (usb_dbg < (unsigned int *)0x100B0000)
//        {
//            *usb_dbg++ = stream->BufferAddr;
//            *usb_dbg++ = stream->BufferLength;
//            *usb_dbg++ = dwPayload;
//            *usb_dbg++ = dwLength;
//        }
        if (dwPayload != 0)
        {
            // DMA_Read()
            dma_addr += 0x100;
            *(volatile unsigned int *)(dma_addr+0x00) = 0x01;
            *(volatile unsigned int *)(dma_addr+0x04) = dwAddr;
            *(volatile unsigned int *)(dma_addr+0x08) = (dwBuffer+dwHeader);
            *(volatile unsigned int *)(dma_addr+0x0C) = dwPayload;
        }
        stream->BufferAddr += (dwPayload + dwHeader);
        stream->BufferLength -= (dwPayload + dwHeader);
        dQH[ep][13] += (dwPayload + dwHeader);
        // adjust buffer position
        dwLength = dwLength - dwPayload;
        if (dwLength != 0)
        {
            dwAddr = dwAddr + dwPayload;
        } else
        {
            dwAddr = 0;
        }
        // packet completion condition
        if (dwLength == 0) stream->BufferLength = 0;
        if (dwInfo > 0xFFFF)
        {
            stream->SecAddrShadow = dwAddr;
            stream->SecLengthShadow = dwLength;
        } else
        {
            stream->PriAddrShadow = dwAddr;
            stream->PriLengthShadow = dwLength;
        }
    }
    // check completed tranfers
    while ((waitPtr[1]&0x80) == 0)
    {
        waitPtr = (unsigned int *)waitPtr[0];
        // advance pointer
        dQH[ep][15] = (unsigned int)waitPtr;
    }
}

void USB_StreamIN(volatile usb_stream_t *stream)
{
    register unsigned int ep;
    register unsigned int dwAddr, dwLength, dwBuffer, dwPacket, dwHeader, dwInfo, dwPayload;

    // determine which stream to mux
    ep = stream->Endpoint;
    // if trigger and transfer is not active
    if ((stream->Type != 0x100) && ((dTD[ep][1]&0x80) == 0))
    {
        USB_EPSetup(ep);
        stream->Type = 0x100;
    }
    // init packet construction
    if ((stream->Type == 0x100) && (stream->BufferLength == 0))
    {
        // only if there is something to send
        if ( (stream->PriAddrShadow != 0) || (stream->SecAddrShadow != 0) )
        {
            // encode uvc header
            dwBuffer = dQH[ep][14];
            dQH[ep][14] = dQH[ep][15];
            dQH[ep][15] = dwBuffer;
            dwPacket = stream->Packet;
            dwInfo = stream->Callback(dwBuffer);
            dwHeader = dwInfo&0xFF;
            stream->BufferAddr = dwBuffer + dwHeader;
            stream->BufferLength = dwPacket - dwHeader;
            dQH[ep][12] = dwBuffer;
            dQH[ep][13] = dwHeader;
        }
    }
    // construct packet piece by piece
    if (stream->BufferLength != 0)
    {
        dwBuffer  = stream->BufferAddr;
        dwPacket  = stream->MuxPacket;
        if (stream->BufferLength < dwPacket ) dwPacket = stream->BufferLength;
        dwInfo  = stream->MuxCallback(dwBuffer);
        dwHeader = dwInfo&0xFF;
        // determine targeted stream
        if (dwInfo > 0xFFFF)
        {
            dwAddr = stream->SecAddrShadow;
            dwLength = stream->SecLengthShadow;
        } else
        {
            dwAddr = stream->PriAddrShadow;
            dwLength = stream->PriLengthShadow;
        }
        dwPayload = dwPacket - dwHeader;
        if (dwLength < dwPayload) dwPayload = dwLength;
        if (dwPayload != 0)
        {
            // DMA_Read()
            dma_addr += 0x100;
            *(volatile unsigned int *)(dma_addr+0x00) = 0x01;
            *(volatile unsigned int *)(dma_addr+0x04) = dwAddr;
            *(volatile unsigned int *)(dma_addr+0x08) = (dwBuffer+dwHeader);
            *(volatile unsigned int *)(dma_addr+0x0C) = dwPayload;
        }
//        if (usb_dbg < (unsigned int *)0x100B0000)
//        {
//            *usb_dbg++ = stream->BufferAddr;
//            *usb_dbg++ = stream->BufferLength;
//            *usb_dbg++ = dwPayload;
//            *usb_dbg++ = dwLength;
//        }
        stream->BufferAddr += (dwPayload + dwHeader);
        stream->BufferLength -= (dwPayload + dwHeader);
        dQH[ep][13] += (dwPayload + dwHeader);
        // adjust buffer position
        dwLength = dwLength - dwPayload;
        if (dwLength != 0)
        {
            dwAddr = dwAddr + dwPayload;
        } else
        {
            dwAddr = 0;
        }
        // packet completion condition
        if (dwLength == 0) stream->BufferLength = 0;
        if (stream->BufferLength == 0) stream->Type++;
        if (dwInfo > 0xFFFF)
        {
            stream->SecAddrShadow = dwAddr;
            stream->SecLengthShadow = dwLength;
        } else
        {
            stream->PriAddrShadow = dwAddr;
            stream->PriLengthShadow = dwLength;
        }
    }
}

void USB_EPSetup(unsigned int idx)
{
    register unsigned int cap, type, mult, addr, len;

    cap  = dQH[idx][0];
    addr = dQH[idx][12];
    len  = dQH[idx][13];
    mult = cap>>0x1E;
    cap  = (cap>>0x10)&0x07FF;

    // prepare transfer specifics
    type = 0x8480;
    if (len>cap) type = 0x8880;
    cap = cap<<0x01;
    if (len>cap) type = 0x8C80;
    if (mult==0) type = 0x8080;
    // add transfer length
    type |= (len<<0x10);
    // get svu id
//    __asm__ __volatile__( "CMU.CPTI %0, 0x1C \n"
//                        :"=r"(mult) );
    // address translation windowed to AHB
    addr = svu_addr + (addr&0x00FFFFFF);
    // write next dTD pointer as invalid
    dTD[idx][0] = 0x01;
    // write transfer control
    dTD[idx][1] = type;
    // write buffer pointers
    dTD[idx][2] = addr;
    dTD[idx][3] = (addr+0x1000);
    // address translation windowed to AHB
    addr = (unsigned int)&dTD[idx];
    addr = svu_addr + (addr&0x00FFFFFF);
    // write dQH next pointer
    dQH[idx][2] = addr;
    // prime endpoint
    cap = (1<<(idx>>0x01));
    if (idx&0x01) cap = cap<<0x10;
    *(volatile unsigned int *)(USB_BASE_ADDR + REG_ENDPTPRIME) = cap;
}


void USB_EPStream(unsigned int idx)
{
    register unsigned int cap, type, mult, addr, len;

    //*(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = USB_USBCMD_RS | USB_USBCMD_ATDTW;

    cap  = dQH[idx][0];
    addr = dQH[idx][12];
    len  = dQH[idx][13];
    mult = cap>>0x1E;
    cap  = (cap>>0x10)&0x07FF;

    // prepare transfer specifics
    type = 0x8480;
    if (len>cap) type = 0x8880;
    cap = cap<<0x01;
    if (len>cap) type = 0x8C80;
    if (mult==0) type = 0x8080;
    // add transfer length
    type |= (len<<0x10);
    // get svu id
//    __asm__ __volatile__( "CMU.CPTI %0, 0x1C \n"
//                        :"=r"(mult) );
    // address translation windowed to AHB
//    addr = USB_AXI_TRANSLATE + (mult<<0x11) + (addr&0x00FFFFFF);
    // bug in moviCompile forces hardcoding to svu 6
    addr = svu_addr + (addr&0x00FFFFFF);

    // setup tripwire
//    do
//    {
//        len = *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD);
//        *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = (len|USB_USBCMD_ATDTW);
//    } while ((len&USB_USBCMD_ATDTW) == 0);

    cap = dQH[idx][14];
    *(volatile unsigned int *)(cap+0x04) = type;
    *(volatile unsigned int *)(cap+0x08) = addr;
    *(volatile unsigned int *)(cap+0x0C) = (addr+0x1000);
    // prime endpoint
    cap = (1<<(idx>>0x01));
    if (idx&0x01) cap = cap<<0x10;
    *(volatile unsigned int *)(USB_BASE_ADDR + REG_ENDPTPRIME) = cap;

//   *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = USB_USBCMD_RS;
//    *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = (len&(~USB_USBCMD_ATDTW));
}

void USB_EPStall(unsigned int idx)
{
    register unsigned int cap, ctrl;

    idx = idx>>1;
    ctrl = USB_BASE_ADDR + REG_ENDPTCTRL0 + (idx<<2);
//    cap = *(volatile unsigned int*)ctrl;
    cap = 0x00010001;
    *(volatile unsigned int*)ctrl = cap;
}

unsigned int  USB_EPStatus(unsigned int idx)
{
    register unsigned int status, mask;

    status = *(volatile unsigned int*)(USB_BASE_ADDR+REG_ENDPTSTAT);
    mask = (1<<(idx>>0x01));
    if (idx&0x01) mask = mask<<0x10;
    status = status&mask;
    return status;
}

void USB_EPStart(endpoint_desc_t* ep)
{
    register unsigned int idx, cap, type, ctrl;

    idx = ep->bEndpointAddress&0x0F;
    ctrl = USB_BASE_ADDR + REG_ENDPTCTRL0 + (idx<<2);
    idx = idx<<1;
    if (ep->bEndpointAddress&0x80) idx++;
    type = ep->bmAttributes&0x03;
    // add ZLT
    cap = 0x20000000;
    // add Mult
    if (type == EP_TT_ISOCHRONOUS) cap |= ((ep->wMaxPacketSize>>0x0B)+1)<<0x1E;
    // add maximum packet length
    cap |= (ep->wMaxPacketSize&0x07FF)<<0x10;
    // add IOS
    if (type == EP_TT_CONTROL) cap |= 0x8000;
    // write capabilities
    dQH[idx][0] = cap;
    // write current dTD pointer
    dQH[idx][1] = 0x00;
    // write next dTD pointer
    dQH[idx][2] = 0x01;
    dQH[idx][3] = 0x00;
    dQH[idx][4] = 0x00;
    dQH[idx][5] = 0x00;
    // clear transfer descriptor too
    dTD[idx][0] = 0x01;
    dTD[idx][1] = 0x00;

    //cap = *(volatile unsigned int*)ctrl;
    if (ep->bEndpointAddress&0x80)
    {
        //cap &= 0xFFFF;
        //cap |= (type<<0x12);
        cap |= (type<<0x02);
        *(volatile unsigned short*)(ctrl+0x02) = (cap|0x40);
        *(volatile unsigned short*)(ctrl+0x02) = (cap|0x80);
    } else
    {
        //cap &= 0xFFFF0000;
        cap |= (type<<0x02);
        *(volatile unsigned short*)ctrl = (cap|0x40);
        *(volatile unsigned short*)ctrl = (cap|0x80);
    }
}

void USB_EPStop(endpoint_desc_t* ep)
{
    register unsigned int idx, cap, type, ctrl;

    idx = ep->bEndpointAddress&0x0F;
    ctrl = USB_BASE_ADDR + REG_ENDPTCTRL0 + (idx<<2);
    idx = idx<<1;
    if (ep->bEndpointAddress&0x80) idx++;
    dQH[idx][2] = 0x01;
    //cap = *(volatile unsigned int*)ctrl;
    if (ep->bEndpointAddress&0x80)
    {
        //cap &= 0xFFFF;
        *(volatile unsigned short*)(ctrl+0x02) = 0;
    } else
    {
        //cap &= 0xFFFF0000;
        *(volatile unsigned short*)ctrl = 0;
    }
}

void USB_EPFlush(unsigned int idx)
{
    register unsigned int cap;
    // prime endpoint
    cap = (1<<(idx>>0x01));
    if (idx&0x01) cap = cap<<0x10;
    *(volatile unsigned int *)(USB_BASE_ADDR + REG_ENDPTFLUSH) = cap;
}

void USB_SetAddress(unsigned int idx)
{
    *(volatile unsigned int *)(USB_BASE_ADDR+REG_DEVICE_ADDR) = ((idx<<0x01)+0x01)<<0x18;
}

void USB_Init(void)
{
    register unsigned int cmd, svu_id;

    // get svu id
    __asm__ __volatile__( "CMU.CPTI %0, 0x1C \n"
                          :"=r"(svu_id) );

    svu_addr = USB_AXI_TRANSLATE + (svu_id<<0x11);

    cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_OTGSC);
    // set OTG transceiver config
    //cmd |=  USB_OTGSC_OT;
    cmd |= (USB_OTGSC_OT | USB_OTGSC_VC);
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_OTGSC) = cmd;
    // transceiver enable
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_PORTSC1) = USB_PORTSCX_PE;
    // clear run bit, detach
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD) = 0;
    // wait to clear
    do
    {
        cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD);
    } while ((cmd&USB_USBCMD_RS) != 0);
    // set reset bit, reinit hw
    cmd |= USB_USBCMD_RST;
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD) = cmd;
    // wait to clear
    do
    {
        cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD);
    } while ((cmd&USB_USBCMD_RST) != 0);
    // set USB address to 0
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_DEVICE_ADDR) = 0;
    // burst settings, holy settings for best bus behaviour coz lately it falls to programmers to do that shit too
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_BURSTSIZE) = 0x1010;
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_SBUSCFG) = 0x00;
    // set the USB Controller to DEVICE mode, setup lockouts off
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBMODE) = (USBMODE_DEVICE_MODE | USB_USBMODE_SLOM);
    // configure ENDPOINTLISTADDR Pointer
    // address translation windowed to AHB
    cmd = (unsigned int)&dQH;
    cmd = svu_addr + (cmd&0x00FFFFFF);
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_ENDPOINTLISTADDR) = cmd;
    // setup control endpoints
    USB_EPStart(&usb_control_out_endpoint);
    USB_EPStart(&usb_control_in_endpoint);
    // set run bit
    //cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD);
    //cmd |= USB_USBCMD_RS;
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBCMD) = USB_USBCMD_RS;
    // dual poll for SRI to sync with host
    cmd = 0;
    while ((cmd&USB_USBSTS_SRI) == 0) cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS);
    // clear SOF
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS) = USB_USBSTS_SRI;
//    cmd = 0;
//    while ((cmd&USB_USBSTS_SRI) == 0) cmd = *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS);
//    // clear SOF
//    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBSTS) = USB_USBSTS_SRI;
    cmd = (1<<IRQ_USB);
    *(volatile unsigned int*)ICB_INT_CLEAR = cmd;
    // configure interrupt level and edge
    *(volatile unsigned int*)ICB_USB_CONFIG = (IRQ_POS_LEVEL|(IRQ_USB_PRIO<<2));
    // redirect interrupts to svu
    *(volatile unsigned int*)(ICB_SVU0_HP + (svu_id<<0x02)) = IRQ_USB;
    // enable USB interrupts
    *(volatile unsigned int*)(USB_BASE_ADDR+REG_USBINTR) = USB_USBSTS_SRI;

    // driver specific init
    usb_ctrl.stream[0].Endpoint = 0;
    usb_ctrl.stream[1].Endpoint = 0;
    usb_ctrl.stream[2].Endpoint = 0;
    usb_ctrl.stream[3].Endpoint = 0;

    dTD_IN[0]   = svu_addr + (((unsigned int)&dTD_IN[8])&0x00FFFFFF);
    dTD_IN[7]   = 0x1C008000;
    dTD_IN[8]   = svu_addr + (((unsigned int)&dTD_IN[16])&0x00FFFFFF);
    dTD_IN[15]  = 0x1C009000;
    dTD_IN[16]  = svu_addr + (((unsigned int)&dTD_IN[24])&0x00FFFFFF);
    dTD_IN[23]  = 0x1C00A000;
    dTD_IN[24]  = svu_addr + (((unsigned int)&dTD_IN[0])&0x00FFFFFF);
    dTD_IN[31]  = 0x1C00B000;
    dTD_OUT[0]  = svu_addr + (((unsigned int)&dTD_OUT[8])&0x00FFFFFF);
    dTD_OUT[7]  = 0x1C00C000;
    dTD_OUT[8]  = svu_addr + (((unsigned int)&dTD_OUT[16])&0x00FFFFFF);
    dTD_OUT[15] = 0x1C00D000;
    dTD_OUT[16] = svu_addr + (((unsigned int)&dTD_OUT[24])&0x00FFFFFF);
    dTD_OUT[23] = 0x1C00E000;
    dTD_OUT[24] = svu_addr + (((unsigned int)&dTD_OUT[0])&0x00FFFFFF);
    dTD_OUT[31] = 0x1C00F000;
    dTD_DIO[0]  = svu_addr + (((unsigned int)&dTD_DIO[8])&0x00FFFFFF);
    dTD_DIO[7]  = 0x1C004000;
    dTD_DIO[8]  = svu_addr + (((unsigned int)&dTD_DIO[16])&0x00FFFFFF);
    dTD_DIO[15] = 0x1C005000;
    dTD_DIO[16] = svu_addr + (((unsigned int)&dTD_DIO[24])&0x00FFFFFF);
    dTD_DIO[23] = 0x1C006000;
    dTD_DIO[24] = svu_addr + (((unsigned int)&dTD_DIO[0])&0x00FFFFFF);
    dTD_DIO[31] = 0x1C007000;
}

void USB_SetupMessage(void)
{
    unsigned int setup_stat, setup_stall, i, idx, ctrl;

    setup_stat = *(volatile unsigned int *)(USB_BASE_ADDR+REG_ENDPTSETUPSTAT);
    for (i=0; i<16; i++)
    {
        if ((setup_stat&(1<<i)) != 0)
        {
            // clear setup stat
            *(volatile unsigned int *)(USB_BASE_ADDR+REG_ENDPTSETUPSTAT) = (1<<i);
            idx = (i<<0x01);
            // setup tripwire
            do
            {
                ctrl = *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD);
                *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = (ctrl|USB_USBCMD_SUTW);
            } while ((ctrl&USB_USBCMD_SUTW) == 0);
            // read out data from setup buffer
            setup_stall = USB_SetupCallback(idx, dQH[idx][10], dQH[idx][11]);
            // clear tripwire
            *(volatile unsigned int *)(USB_BASE_ADDR+REG_USBCMD) = (ctrl&(~USB_USBCMD_SUTW));

            if (setup_stall != 0)
            {
                USB_EPStall(idx);
            } else
            {
                if (dQH[idx][12] != 0) USB_EPSetup(idx);
                if (dQH[idx+1][12] != 0) USB_EPSetup(idx+1);
            }
        }
    }

    // master haxxor to avoid USB IP write/SVU read clash -> infinite stalls
//    setup_stat = dQH[0][10];
//    if (setup_stat != 0)
//    {
//        ctrl = dQH[0][11];
//        setup_stall = USB_SetupCallback(0, setup_stat, ctrl);
//        dQH[0][10] = 0;
//        dQH[0][11] = 0;
//        *(volatile unsigned int *)(USB_BASE_ADDR+REG_ENDPTSETUPSTAT) = 0x01;
//    }
//    if (setup_stall != 0)
//    {
//        USB_EPStall(0);
//    } else
//    {
//        if ( (dQH[0][13] != 0) || (dQH[1][13] != 0) ) USB_EPSetup(0);
//        USB_EPSetup(1);
//    }
}

