 /**
============================================================================
 * @copyright 
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
 * @defgroup usbVideoClassCore USB video device class core
 * @{
 * @brief USB video device class core implementation
 *
 */



#ifndef _USB_H_
#define _USB_H_

// sneak user defines to be visible to system manager
#include "usbConfig.h"


typedef struct
{
    unsigned int Endpoint;
    unsigned int Type;
    unsigned int BufferAddr;
    unsigned int BufferLength;
    unsigned int PriAddr;
    unsigned int PriLength;
    unsigned int SecAddr;
    unsigned int SecLength;
    unsigned int PriAddrShadow;
    unsigned int PriLengthShadow;
    unsigned int SecAddrShadow;
    unsigned int SecLengthShadow;
    unsigned int MuxPacket;
    unsigned int Packet;
    unsigned int (*MuxCallback)(unsigned int);
    unsigned int (*Callback)(unsigned int);
} usb_stream_t;

// UVC API
typedef struct
{
    // read only for device status
    unsigned int   deviceStat;
    unsigned int   deviceClock;
    unsigned int   interfaceAlter03;
    unsigned int   interfaceAlter47;
    // read/write interrupt signals here
    unsigned int   interruptSignal[60];
    // expose streaming interfaces
    usb_stream_t   stream[4];
} usb_ctrl_t;

//    unsigned int   streamINAddr;
//    unsigned int   streamINLength;
//    unsigned int   streamINAddrShadow;
//    unsigned int   streamINLengthShadow;
//    unsigned int   streamINEndpoint;
//    unsigned int   streamINHeader;
//    unsigned int   streamINPacket;
//    unsigned int (*streamINCallback)(unsigned int, unsigned int);
//    // expose dual IN streaming interface, use it as single OR paired with DO
//    unsigned int   streamDIAddr;
//    unsigned int   streamDILength;
//    unsigned int   streamDIAddrShadow;
//    unsigned int   streamDILengthShadow;
//    unsigned int   streamDIEndpoint;
//    unsigned int   streamDIHeader;
//    unsigned int   streamDIPacket;
//    unsigned int (*streamDICallback)(unsigned int, unsigned int);
//    // expose OUT streaming interface
//    unsigned int   streamOUTAddr;
//    unsigned int   streamOUTLength;
//    unsigned int   streamOUTAddrShadow;
//    unsigned int   streamOUTLengthShadow;
//    unsigned int   streamOUTEndpoint;
//    unsigned int   streamOUTHeader;
//    unsigned int   streamOUTPacket;
//    unsigned int (*streamOUTCallback)(unsigned int, unsigned int);
//    // expose dual OUT streaming interface, use it as single OR paired with DI
//    unsigned int   streamDOAddr;
//    unsigned int   streamDOLength;
//    unsigned int   streamDOAddrShadow;
//    unsigned int   streamDOLengthShadow;
//    unsigned int   streamDOEndpoint;
//    unsigned int   streamDOHeader;
//    unsigned int   streamDOPacket;
//    unsigned int (*streamDOCallback)(unsigned int, unsigned int);

/// API visible to system manager, initialize structure and signal data base
extern volatile usb_ctrl_t __attribute__((aligned(64))) usb_ctrl;

///Global init function
extern void USB_Main(void);
/// @}
#endif
