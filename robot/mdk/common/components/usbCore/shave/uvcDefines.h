///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     usbCoreApiDefines
///              
/// USB  video class defines and data-structures
/// 

#ifndef _UVC_DEFINES_H_
#define _UVC_DEFINES_H_

;// UVC Specification Revision Number
#define UVC_REV 0x0100

;// Subclass Code
#define SC_UNDEFINED                              0x00
#define SC_VIDEOCONTROL                           0x01
#define SC_VIDEOSTREAMING                         0x02
#define SC_VIDEO_INTERFACE_COLLECTION             0x03

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

// Class-Specific VC Interface Header Descriptor
typedef struct __attribute__((packed))
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
} vc_status_interrupt_packet_t;

#endif
