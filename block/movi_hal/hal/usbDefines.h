
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


#ifndef _USB_DEFINES_H_
#define _USB_DEFINES_H_
//#include "usbConfig.h"
//#include "uvcConfig.h"

;// USB Specification Revision Number
#define USB_REV 0x0200
;// UVC Specification Revision Number
#define UVC_REV 0x0100

;// Base Class Code
#define CC_DEVICE                 0x00
#define CC_AUDIO                  0x01
#define CC_COMMUNICATION          0x02
#define CC_HID                    0x03
#define CC_PHYSICAL               0x05
#define CC_IMAGE                  0x06
#define CC_PRINTER                0x07
#define CC_MASS_STORAGE           0x08
#define CC_HUB                    0x09
#define CC_CDC_DATA               0x0A
#define CC_SMART_CARD             0x0B
#define CC_CONTENT_SECURITY       0x0D
#define CC_VIDEO                  0x0E
#define CC_PERSONAL_HEALTHCARE    0x0F
#define CC_DIAGNOSTIC_DEVICE      0xDC
#define CC_WIRELESS_CONTROLLER    0xE0
#define CC_MISCELLANEOUS          0xEF
#define CC_VENDOR_SPECIFIC        0xFF

;// Subclass Code
#define SC_UNDEFINED                              0x00
#define SC_VIDEOCONTROL                           0x01
#define SC_VIDEOSTREAMING                         0x02
#define SC_VIDEO_INTERFACE_COLLECTION             0x03

;// Descriptor Type
#define GET_STATUS                                0x00
#define CLEAR_FEATURE                             0x01
#define SET_FEATURE                               0x03
#define SET_ADDRESS                               0x05
#define GET_DESCRIPTOR                            0x06
#define SET_DESCRIPTOR                            0x07
#define GET_CONFIGURATION                         0x08
#define SET_CONFIGURATION                         0x09
#define GET_INTERFACE                             0x0A
#define SET_INTERFACE                             0x0B
#define SYNCH_FRAME                               0x0C

;// Descriptor Type
#define DEVICE                                    0x01
#define CONFIGURATION                             0x02
#define STRING                                    0x03
#define INTERFACE                                 0x04
#define ENDPOINT                                  0x05
#define DEVICE_QUALIFIER                          0x06
#define OTHER_SPEED_CONFIGURATION                 0x07
#define INTERFACE_POWER                           0x08
#define OTG                                       0x09
#define DEBUG                                     0x0A
#define INTERFACE_ASSOCIATION                     0x0B

;// Endpoint Descriptor Attributes
;// Transfer Type
#define EP_TT_CONTROL                             0x00
#define EP_TT_ISOCHRONOUS                         0x01
#define EP_TT_BULK                                0x02
#define EP_TT_INTERRUPT                           0x03
;// Synchronization Type
#define EP_ST_NO_SYNC                             0x00
#define EP_ST_ASYNC                               0x04
#define EP_ST_ADAPTIVE                            0x08
#define EP_ST_SYNC                                0x0C
;// Usage Type
#define EP_UT_DATA                                0x00
#define EP_UT_FEEDBACK                            0x10
#define EP_UT_IMPLICIT_FEEDBACK                   0x20
#define EP_UT_RSV                                 0x30

;// Video Class-Specific Descriptor Types
#define CS_UNDEFINED                              0x20
#define CS_DEVICE                                 0x21
#define CS_CONFIGURATION                          0x22
#define CS_STRING                                 0x23
#define CS_INTERFACE                              0x24
#define CS_ENDPOINT                               0x25

;// Video Class-Specific VC Interface Descriptor Subtypes
#define VC_DESCRIPTOR_UNDEFINED                   0x00
#define VC_HEADER                                 0x01
#define VC_INPUT_TERMINAL                         0x02
#define VC_OUTPUT_TERMINAL                        0x03
#define VC_SELECTOR_UNIT                          0x04
#define VC_PROCESSING_UNIT                        0x05
#define VC_EXTENSION_UNIT                         0x06

;// UVC Terminal Types
#define TT_VENDOR_SPECIFIC                        0x0100
#define TT_STREAMING                              0x0101

;// UVC Input Terminal Types
#define ITT_VENDOR_SPECFIC                        0x0200
#define ITT_CAMERA                                0x0201
#define ITT_MEDIA_TRANSPORT_INPUT                 0x0202

;// UVC Output Terminal Types
#define OTT_VENDOR_SPECIFIC                       0x0300
#define OTT_DISPLAY                               0x0301
#define OTT_MEDIA_TRANSPORT_OUTPUT                0x0302

;// Video Class Specific Endpoint Descriptor Subtypes
#define EP_UNDEFINED                              0x00
#define EP_GENERAL                                0x01
#define EP_ENDPOINT                               0x02
#define EP_INTERRUPT                              0x03

;// VideoControl Inteface Control Selectors
#define VC_CONTROL_UNDEFINED                      0x00
#define VC_VIDEO_POWER_MODE_CONTROL               0x01
#define VC_REQUEST_ERROR_CODE_CONTROL             0x02

;// Color Primaries Definition (bColorPrimaries)
#define CP_UNSPECIFIED                            0x00
#define CP_BT709                                  0x01
#define CP_BT470M                                 0x02
#define CP_BT470BG                                0x03
#define CP_SMPTE170M                              0x04
#define CP_SMPTE240M                              0x05

;// Transfer Characteristics Definition (bTransferCharacteristics)
#define TC_UNSPECIFIED                            0x00
#define TC_BT709                                  0x01
#define TC_BT470M                                 0x02
#define TC_BT470BG                                0x03
#define TC_SMPTE170M                              0x04
#define TC_SMPTE240M                              0x05
#define TC_LINEAR                                 0x06
#define TC_SRGB                                   0x07

;// Matrix Coefficients Definition (bMatrixCoefficients)
#define MC_UNSPECIFIED                            0x00
#define MC_BT709                                  0x01
#define MC_FCC                                    0x02
#define MC_BT470BG                                0x03
#define MC_SMPTE170M                              0x04
#define MC_SMPTE240M                              0x05

;// Video Class-Specific VS Interface Descriptor Subtypes
#define VS_UNDEFINED                              0x00
#define VS_INPUT_HEADER                           0x01
#define VS_OUTPUT_HEADER                          0x02
#define VS_STILL_IMAGE_FRAME                      0x03
#define VS_FORMAT_UNCOMPRESSED                    0x04
#define VS_FRAME_UNCOMPRESSED                     0x05
#define VS_FORMAT_MJPEG                           0x06
#define VS_FRAME_MJPEG                            0x07
#define VS_FORMAT_MPEG2TS                         0x0A
#define VS_FORMAT_DV                              0x0C
#define VS_COLORFORMAT                            0x0D
#define VS_FORMAT_FRAME_BASED                     0x10
#define VS_FRAME_FRAME_BASED                      0x11
#define VS_FORMAT_STREAM_BASED                    0x12

;// VideoStreaming Interface Control Selectors
#define VS_CONTROL_UNDEFINED                      0x00
#define VS_PROBE_CONTROL                          0x01
#define VS_COMMIT_CONTROL                         0x02
#define VS_STILL_PROBE_CONTROL                    0x03
#define VS_STILL_COMMIT_CONTROL                   0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL            0x05
#define VS_STREAM_ERROR_CODE_CONTROL              0x06
#define VS_GENERATE_KEY_FRAME_CONTROL             0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL           0x08
#define VS_SYNCH_DELAY_CONTROL                    0x09

;// Video Class-Specific Request Codes
#define SET_CUR                                   0x01
#define GET_CUR                                   0x81
#define GET_MIN                                   0x82
#define GET_MAX                                   0x83
#define GET_RES                                   0x84
#define GET_LEN                                   0x85
#define GET_INFO                                  0x86
#define GET_DEF                                   0x87

// Device Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned short bcdUSB;
    unsigned char  bDeviceClass;
    unsigned char  bDeviceSubClass;
    unsigned char  bDeviceProtocol;
    unsigned char  bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char  iManufacturer;
    unsigned char  iProduct;
    unsigned char  iSerialNumber;
    unsigned char  bNumConfigurations;
} device_desc_t;

// Device Qualifier
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned short bcdUSB;
    unsigned char  bDeviceClass;
    unsigned char  bDeviceSubClass;
    unsigned char  bDeviceProtocol;
    unsigned char  bMaxPacketSize0;
    unsigned char  bNumConfigurations;
    unsigned char  bReserved;
} device_qualifier_desc_t;

// Configuration Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned short wTotalLength;
    unsigned char  bNumInterfaces;
    unsigned char  bConfigurationValue;
    unsigned char  iConfiguration;
    unsigned char  bmAttributes;
    unsigned char  bMaxPower;
} config_desc_t;

// Interface Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bInterfaceNumber;
    unsigned char  bAlternateSetting;
    unsigned char  bNumEndpoints;
    unsigned char  bInterfaceClass;
    unsigned char  bInterfaceSubClass;
    unsigned char  bInterfaceProtocol;
    unsigned char  iInterface;
} interface_desc_t;

// Endpoint Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bEndpointAddress;
    unsigned char  bmAttributes;
    unsigned short wMaxPacketSize;
    unsigned char  bInterval;
} endpoint_desc_t;

// Standard Interface Association Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bFirstInterface;
    unsigned char  bInterfaceCount;
    unsigned char  bFunctionClass;
    unsigned char  bFunctionSubClass;
    unsigned char  bFunctionProtocol;
    unsigned char  iFunction;
} iad_desc_t;

// Class-Specific VC Interface Header Descriptor
/*typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned short bcdUVC;
    unsigned short wTotalLength;
    unsigned int dwClockFrequency;
    unsigned char  bInCollection;
    unsigned char  baInterfaceNr[UVC_DEVICE_NR_INTERFACE-1];
} vc_interface_header_desc_t;

// Camera Terminal Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bTerminalID;
    unsigned short wTerminalType;
    unsigned char  bAssocTerminal;
    unsigned char  iTerminal;
    unsigned short wObjectiveFocalLengthMin;
    unsigned short wObjectiveFocalLengthMax;
    unsigned short wOcularFocalLength;
    unsigned char  bControlSize;
    unsigned char  bmControls[UVC_DEVICE_NR_BITFIELD];
} vc_camera_terminal_desc_t;

// Input Terminal Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bTerminalID;
    unsigned short wTerminalType;
    unsigned char  bAssocTerminal;
    unsigned char  iTerminal;
} vc_input_terminal_desc_t;

// Output Terminal Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bTerminalID;
    unsigned short wTerminalType;
    unsigned char  bAssocTerminal;
    unsigned char  bSourceID;
    unsigned char  iTerminal;
} vc_output_terminal_desc_t;

// Extension Unit Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bUnitID;
    unsigned char  guidExtensionCode[16];
    unsigned char  bNumControls;
    unsigned char  bNrInPins;
    unsigned char  baSourceID[1];
    unsigned char  bControlSize;
    unsigned char  bmControls[UVC_DEVICE_NR_BITFIELD];
    unsigned char  iExtension;
} vc_extension_unit_desc_t;

// Class-specific VC Interrupt Endpoint Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned short wMaxTransferSize;
} vc_interrupt_endpoint_desc_t;

// Class-specific VS Interface Input Header Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bNumFormats;
    unsigned short wTotalLength;
    unsigned char  bEndpointAddress;
    unsigned char  bmInfo;
    unsigned char  bTerminalLink;
    unsigned char  bStillCaptureMethod;
    unsigned char  bTriggerSupport;
    unsigned char  bTriggerUsage;
    unsigned char  bControlSize;
    unsigned char  bmControls[UVC_DEVICE_NR_BITFIELD];
} vs_interface_input_header_desc_t;

// Class-specific VS Interface Output Header Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bNumFormats;
    unsigned short wTotalLength;
    unsigned char  bEndpointAddress;
    unsigned char  bTerminalLink;
#if UVC_REV == 0x0110
    unsigned char  bControlSize;
    unsigned char  bmControls[UVC_DEVICE_NR_BITFIELD];
#endif
} vs_interface_output_header_desc_t;

// Color Matching Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bColorPrimaries;
    unsigned char  bTransferCharacteristics;
    unsigned char  bMatrixCoefficients;
} vs_color_matching_desc_t;

// Uncompressed Video Format Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bFormatIndex;
    unsigned char  bNumFrameDescriptors;
    unsigned char  guidFormat[16];
    unsigned char  bBitsPerPixel;
    unsigned char  bDefaultFrameIndex;
    unsigned char  bAspectRatioX;
    unsigned char  bAspectRatioY;
    unsigned char  bmInterlaceFlags;
    unsigned char  bCopyProtect;
} vs_uncompressed_video_format_desc_t;

// Uncompressed Video Frame Descriptor (Discrete Frame Interval)
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bFrameIndex;
    unsigned char  bmCapabilities;
    unsigned short wWidth;
    unsigned short wHeight;
    unsigned int dwMinBitRate;
    unsigned int dwMaxBitRate;
    unsigned int dwMaxVideoFrameBufferSize;
    unsigned int dwDefaultFrameInterval;
    unsigned char  bFrameIntervalType;
    unsigned int dwFrameInterval[UVC_FRAMERATE_NR];
} vs_uncompressed_video_frame_desc_t;

// Stream Based Format Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bFormatIndex;
    unsigned char  guidFormat[16];
    unsigned int dwPacketLength;
} vs_stream_based_format_desc_t;

// MPEG2TS Video Format Descriptor
typedef struct __attribute__((packed))
{
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned char  bDescriptorSubtype;
    unsigned char  bFormatIndex;
    unsigned char  bDataOffset;
    unsigned char  bPacketLength;
    unsigned char  bStrideLength;
#if UVC_REV == 0x0110
    unsigned char  guidStrideFormat[16];
#endif
} vs_mpeg2ts_video_format_desc_t;

// Video Probe and Commit Controls Parameter Block
typedef struct __attribute__((packed))
{
    unsigned short bmHint;
    unsigned char  bFormatIndex;
    unsigned char  bFrameIndex;
    unsigned int dwFrameInterval;
    unsigned short wKeyFrameRate;
    unsigned short wPFrameRate;
    unsigned short wCompQuality;
    unsigned short wCompWindowSize;
    unsigned short wDelay;
    unsigned int dwMaxVideoFrameSize;
    unsigned int dwMaxPayloadTransferSize;
#if UVC_REV == 0x0110
    unsigned int dwClockFrequency;
    unsigned char  bmFramingInfo;
    unsigned char  bPreferedVersion;
    unsigned char  bMinVersion;
    unsigned char  bMaxVersion;
#endif
} video_probe_and_commit_control_t;

// VC Status interrupt messaqe format
typedef struct __attribute__((packed))
{
    unsigned char  bStatusType;
    unsigned char  bOriginator;
    unsigned char  bEvent;
    unsigned char  bSelector;
    unsigned char  bAttribute;
    unsigned char  bValue;
} vc_status_interrupt_packet_t; */

;// HW register address
#define USB_BASE_ADDR	          0xD0300000
#define USB_AXI_TRANSLATE         0xA0010000

;/********** USBCMD Register **********/
;/** Masks **/
#define MASK_USB_USBCMD_ITC      0x0000001F
#define POS_USB_USBCMD_ITC       16
#define USB_USBCMD_FS2           0x00008000
#define USB_USBCMD_ATDTW         0x00004000
#define USB_USBCMD_SUTW          0x00002000
#define USB_USBCMD_ASPE          0x00000800
#define USB_USBCMD_ASP1          0x00000200
#define USB_USBCMD_ASP0          0x00000100
#define USB_USBCMD_LR            0x00000080
#define USB_USBCMD_IAA           0x00000040
#define USB_USBCMD_ASE           0x00000020
#define USB_USBCMD_PSE           0x00000010
#define USB_USBCMD_FS1           0x00000008
#define USB_USBCMD_FS0           0x00000004
#define USB_USBCMD_RST           0x00000002
#define USB_USBCMD_RS            0x00000001

;/********** PORTSCX Register **********/
;/** Masks **/
#define MASK_USB_PORTSCX_PTS     0xC0000000
#define POS_USB_PORTSCX_PTS      30
#define USB_PORTSCX_STS          0x20000000
#define USB_PORTSCX_PTW          0x10000000
#define MASK_USB_PORTSCX_PSPD    0x0C000000
#define POS_USB_PORTSCX_PSPD     26
#define USB_PORTSCX_PTS2         0x02000000
#define USB_PORTSCX_PFSC         0x01000000
#define USB_PORTSCX_PHCD         0x00800000
#define USB_PORTSCX_WKOC         0x00400000
#define USB_PORTSCX_WKDS         0x00200000
#define USB_PORTSCX_WKCN         0x00100000
#define MASK_USB_PORTSCX_PTC     0x000F0000
#define POS_USB_PORTSCX_PTC      16
#define MASK_USB_PORTSCX_PIC     0x0000C000
#define POS_USB_PORTSCX_PIC      14
#define USB_PORTSCX_PO           0x00002000
#define USB_PORTSCX_PP           0x00001000
#define MASK_USB_PORTSCX_LS      0x00000C00
#define POS_USB_PORTSCX_LS       10
#define USB_PORTSCX_HSP          0x00000200
#define USB_PORTSCX_PR           0x00000100
#define USB_PORTSCX_SUSP         0x00000080
#define USB_PORTSCX_FRP          0x00000040
#define USB_PORTSCX_OCC          0x00000020
#define USB_PORTSCX_OCA          0x00000010
#define USB_PORTSCX_PEC          0x00000008
#define USB_PORTSCX_PE           0x00000004
#define USB_PORTSCX_CSC          0x00000002
#define USB_PORTSCX_CCS          0x00000001

;/********** USBMODE Register **********/
;/** Masks **/
#define USB_USBMODE_SRT          0x00008000
#define USB_USBMODE_VBPS         0x00000020
#define USB_USBMODE_SDIS         0x00000010
#define USB_USBMODE_SLOM         0x00000008
#define USB_USBMODE_ES           0x00000004
#define MASK_USB_USBMODE_CM      0x00000003
#define POS_USB_USBMODE_CM       0
#define USBMODE_IDLE          0x0
#define USBMODE_RESERVE       0x1
#define USBMODE_DEVICE_MODE   0x2
#define USBMODE_HOST_MODE     0x3

;/********** OTGSC Register **********/
;/** Masks **/
#define USB_OTGSC_DPIE          0x40000000
#define USB_OTGSC_1msE          0x20000000
#define USB_OTGSC_BSEIE         0x10000000
#define USB_OTGSC_BSVIE         0x08000000
#define USB_OTGSC_ASVIE         0x04000000
#define USB_OTGSC_AVVIE         0x02000000
#define USB_OTGSC_IDIE          0x01000000
#define USB_OTGSC_DPIS          0x00400000
#define USB_OTGSC_1msS          0x00200000
#define USB_OTGSC_BSEIS         0x00100000
#define USB_OTGSC_BSVIS         0x00080000
#define USB_OTGSC_ASVIS         0x00040000
#define USB_OTGSC_AVVIS         0x00020000
#define USB_OTGSC_IDIS          0x00010000
#define USB_OTGSC_DPS           0x00004000
#define USB_OTGSC_1msT          0x00002000
#define USB_OTGSC_BSE           0x00001000
#define USB_OTGSC_BSV           0x00000800
#define USB_OTGSC_ASV           0x00000400
#define USB_OTGSC_AVV           0x00000200
#define USB_OTGSC_ID            0x00000100
#define USB_OTGSC_HABA          0x00000080
#define USB_OTGSC_HADP          0x00000040
#define USB_OTGSC_IDPU          0x00000020
#define USB_OTGSC_DP            0x00000010
#define USB_OTGSC_OT            0x00000008
#define USB_OTGSC_HAAR          0x00000004
#define USB_OTGSC_VC            0x00000002
#define USB_OTGSC_VD            0x00000001

;/********** USBSTS Register **********/
;/** Masks **/
#define USB_USBSTS_TI1           0x02000000
#define USB_USBSTS_TI0           0x01000000
#define USB_USBSTS_UPI           0x00080000
#define USB_USBSTS_UAI           0x00040000
#define USB_USBSTS_NAKI          0x00010000
#define USB_USBSTS_AS            0x00008000
#define USB_USBSTS_PS            0x00004000
#define USB_USBSTS_RCL           0x00002000
#define USB_USBSTS_HCH           0x00001000
#define USB_USBSTS_ULPII         0x00000400
#define USB_USBSTS_SLI           0x00000100
#define USB_USBSTS_SRI           0x00000080
#define USB_USBSTS_URI           0x00000040
#define USB_USBSTS_AAI           0x00000020
#define USB_USBSTS_SEI           0x00000010
#define USB_USBSTS_FRI           0x00000008
#define USB_USBSTS_PCI           0x00000004
#define USB_USBSTS_UEI           0x00000002
#define USB_USBSTS_UI            0x00000001

;// HW register map
#define REG_ID                         0x0000
#define REG_HWGENERAL                  0x0004
#define REG_HWDEVICE                   0x000C
#define REG_HWTXBUF                    0x0010
#define REG_HWRXBUF                    0x0014
#define REG_GPTIMER0LD                 0x0080
#define REG_GPTIMER0CTRL               0x0084
#define REG_GPTIMER1LD                 0x0088
#define REG_GPTIMER1CTRL               0x008C
#define REG_SBUSCFG                    0x0090
#define REG_CAPLENGTH                  0x0100
#define REG_DCIVERSION                 0x0120
#define REG_DCCPARAMS                  0x0124
#define REG_USBCMD                     0x0140
#define REG_USBSTS                     0x0144
#define REG_USBINTR                    0x0148
#define REG_FRINDEX                    0x014C
#define REG_DEVICE_ADDR                0x0154
#define REG_ENDPOINTLISTADDR           0x0158
#define REG_BURSTSIZE                  0x0160
#define REG_TXFILLTUNING               0x0164
#define REG_IC_USB                     0x016C
#define REG_ULPI_VIEWPORT              0x0170
#define REG_ENDPTNAK                   0x0178
#define REG_ENDPTNAKEN                 0x017C
#define REG_PORTSC1                    0x0184
#define REG_PORTSC2                    0x0188
#define REG_PORTSC3                    0x018C
#define REG_PORTSC4                    0x0190
#define REG_PORTSC5                    0x0194
#define REG_PORTSC6                    0x0198
#define REG_PORTSC7                    0x019C
#define REG_PORTSC8                    0x01A0
#define REG_OTGSC                      0x01A4
#define REG_USBMODE                    0x01A8
#define REG_ENDPTSETUPSTAT             0x01AC
#define REG_ENDPTPRIME                 0x01B0
#define REG_ENDPTFLUSH                 0x01B4
#define REG_ENDPTSTAT                  0x01B8
#define REG_ENDPTCOMPLETE              0x01BC
#define REG_ENDPTCTRL0                 0x01C0
#define REG_ENDPTCTRL1                 0x01C4
#define REG_ENDPTCTRL2                 0x01C8
#define REG_ENDPTCTRL3                 0x01CC
#define REG_ENDPTCTRL4                 0x01D0
#define REG_ENDPTCTRL5                 0x01D4
#define REG_ENDPTCTRL6                 0x01D8
#define REG_ENDPTCTRL7                 0x01DC
#define REG_ENDPTCTRL8                 0x01E0
#define REG_ENDPTCTRL9                 0x01E4
#define REG_ENDPTCTRL10                0x01E8
#define REG_ENDPTCTRL11                0x01EC
#define REG_ENDPTCTRL12                0x01F0
#define REG_ENDPTCTRL13                0x01F4
#define REG_ENDPTCTRL14                0x01F8
#define REG_ENDPTCTRL15                0x01FC

// ICB interrupt clear address
#define IRQ_USB           27
#define IRQ_USB_PRIO      14
#define IRQ_POS_EDGE      0x00
#define IRQ_NEG_EDGE      0x01
#define IRQ_POS_LEVEL     0x02
#define IRQ_NEG_LEVEL     0x03
#define ICB_USB_CONFIG    (0x80010000+4*IRQ_USB)
#define ICB_INT_CLEAR     0x80010110
#define ICB_SVU0_HP       0x800101C0

// core data structures visible everywhere through driver layers
//extern volatile unsigned int __attribute__((aligned(2048))) dQH[32][16];
//extern volatile unsigned int __attribute__((aligned(1024))) dTD[32][8];
//extern volatile unsigned int __attribute__((aligned(64)))   dTD_IN[32];
//extern volatile unsigned int __attribute__((aligned(64)))   dTD_OUT[32];
//extern volatile unsigned int __attribute__((aligned(64)))   dTD_DIO[32];

//extern void USB_EPSetup(unsigned int);
//extern void USB_EPStream(unsigned int);
//extern void USB_EPStall(unsigned int);
//extern unsigned int  USB_EPStatus(unsigned int);
//extern void USB_EPStart(endpoint_desc_t*);
//extern void USB_EPStop(endpoint_desc_t*);
//extern void USB_EPFlush(unsigned int);
//extern void USB_SetAddress(unsigned int);

#endif
