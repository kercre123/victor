///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Common UVC USB configurations
///

#include "uvcDefines.h"
#include "SPI.h"
#include "usbConfig.h"
#include "usb.h"
#include "usbDefines.h"
#include "uvcConfig.h"

// some utility data length defines
#define  uvc_vc_interface_header_if00_sizeof  (sizeof(vc_interface_header_desc_t) + sizeof(vc_camera_terminal_desc_t) + sizeof(vc_output_terminal_desc_t))
#define  uvc_config_sizeof                    (sizeof(config_desc_t) + sizeof(iad_desc_t) + \
                                               sizeof(interface_desc_t) + uvc_vc_interface_header_if00_sizeof + \
                                               sizeof(interface_desc_t) + uvc_vs_interface_header_uncompressed_sizeof + sizeof(interface_desc_t) + sizeof(endpoint_desc_t) + \
                                               sizeof(interface_desc_t) + uvc_vs_interface_header_compressed_sizeof + sizeof(interface_desc_t) + sizeof(endpoint_desc_t) )
#define uvc_vs_uncompressed_sizeof  (sizeof(interface_desc_t) + uvc_vs_interface_header_uncompressed_sizeof + sizeof(interface_desc_t) + sizeof(endpoint_desc_t))
#define uvc_vs_compressed_sizeof    (sizeof(interface_desc_t) + uvc_vs_interface_header_compressed_sizeof + sizeof(interface_desc_t) + sizeof(endpoint_desc_t))

extern video_interface_t video_interface_array[VIDEO_INTERFACES_NR];

unsigned char uvc_config_frame[uvc_config_sizeof];

// freely configure USB descriptors according to standard here
device_desc_t uvc_device =
{
    .bLength = sizeof(device_desc_t),
    .bDescriptorType = DEVICE,
    .bcdUSB = USB_REV,
    .bDeviceClass = CC_MISCELLANEOUS,
    .bDeviceSubClass = 0x02,
    .bDeviceProtocol = 0x01,
    .bMaxPacketSize0 = 0x40,
    .idVendor = 0x03E7,
    .idProduct = 0x1811,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x00,
    .iProduct = 0x00,
    .iSerialNumber = 0x00,
    .bNumConfigurations = 0x01,
};

device_qualifier_desc_t uvc_device_qualifier =
{
    .bLength = sizeof(device_qualifier_desc_t),
    .bDescriptorType = DEVICE_QUALIFIER,
    .bcdUSB = USB_REV,
    .bDeviceClass = CC_MISCELLANEOUS,
    .bDeviceSubClass = 0x02,
    .bDeviceProtocol = 0x01,
    .bMaxPacketSize0 = 0x40,
    .bNumConfigurations = 0x00,
    .bReserved = 0x00,
};

// initialise config data structure
uvc_config_t uvc_config =
{
    .config =
    {
        .bLength = sizeof(config_desc_t),
        .bDescriptorType = CONFIGURATION,
        .wTotalLength = uvc_config_sizeof,
        .bNumInterfaces = UVC_DEVICE_NR_INTERFACE,
        .bConfigurationValue = 0x01,
        .iConfiguration = 0x00,
        .bmAttributes = 0x80,
        .bMaxPower = 0xFA,
    },
    .iad =
    {
        .bLength = sizeof(iad_desc_t),
        .bDescriptorType = INTERFACE_ASSOCIATION,
        .bFirstInterface = 0x00,
        .bInterfaceCount = UVC_DEVICE_NR_INTERFACE,
        .bFunctionClass = CC_VIDEO,
        .bFunctionSubClass = SC_VIDEO_INTERFACE_COLLECTION,
        .bFunctionProtocol = 0x00,
        .iFunction = 0x00,
    },
    .vc_interface_if00 =
    {
        .bLength = sizeof(interface_desc_t),
        .bDescriptorType = INTERFACE,
        .bInterfaceNumber = 0x00,
        .bAlternateSetting = 0x00,
        .bNumEndpoints = 0x00,
        .bInterfaceClass = CC_VIDEO,
        .bInterfaceSubClass = SC_VIDEOCONTROL,
        .bInterfaceProtocol = 0x00,
        .iInterface = 0x00,
    },
    .vc_interface_header_if00 =
    {
        .bLength = sizeof(vc_interface_header_desc_t),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = VC_HEADER,
        .bcdUVC = UVC_REV,
        .wTotalLength = uvc_vc_interface_header_if00_sizeof,
        .dwClockFrequency = 0x00001F40,
        .bInCollection = (UVC_DEVICE_NR_INTERFACE-1),
#if UVC_DEVICE_NR_INTERFACE>1
        .baInterfaceNr[0] = 1,
#endif
#if UVC_DEVICE_NR_INTERFACE>2
        .baInterfaceNr[1] = 2,
#endif
#if UVC_DEVICE_NR_INTERFACE>3
        .baInterfaceNr[2] = 3,
#endif
#if UVC_DEVICE_NR_INTERFACE>4
        .baInterfaceNr[3] = 4,
#endif
#if UVC_DEVICE_NR_INTERFACE>5
        .baInterfaceNr[4] = 5,
#endif
#if UVC_DEVICE_NR_INTERFACE>6
        .baInterfaceNr[5] = 6,
#endif
#if UVC_DEVICE_NR_INTERFACE>7
        .baInterfaceNr[6] = 7,
#endif
    },
    .vc_camera_terminal_cam =
    {
        .bLength = sizeof(vc_camera_terminal_desc_t),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = VC_INPUT_TERMINAL,
        .bTerminalID = UVC_MODEL_CAM_IN_ID,
        .wTerminalType = ITT_CAMERA,
        .bAssocTerminal = UVC_MODEL_NONE_ID,
        .iTerminal = 0x00,
        .wObjectiveFocalLengthMin = 0x0000,
        .wObjectiveFocalLengthMax = 0x0000,
        .wOcularFocalLength = 0x0000,
        .bControlSize = UVC_DEVICE_NR_BITFIELD,
#if UVC_DEVICE_NR_BITFIELD>0
        .bmControls[0] = 0x00,
#endif
#if UVC_DEVICE_NR_BITFIELD>1
        .bmControls[1] = 0x00,
#endif
#if UVC_DEVICE_NR_BITFIELD>2
        .bmControls[2] = 0x00,
#endif
#if UVC_DEVICE_NR_BITFIELD>3
        .bmControls[3] = 0x00,
#endif
    },
    .vc_output_terminal_usb =
    {
        .bLength = sizeof(vc_output_terminal_desc_t),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = VC_OUTPUT_TERMINAL,
        .bTerminalID = UVC_MODEL_USB_OUT_ID,
        .wTerminalType = TT_STREAMING,
        .bAssocTerminal = UVC_MODEL_NONE_ID,
        .bSourceID = UVC_MODEL_CAM_IN_ID,
        .iTerminal = 0x00,
    },
};

volatile unsigned char  __attribute__((aligned(4))) usb_status[2] = { 0x01, 0x00 };
volatile unsigned short __attribute__((aligned(4))) vs_probe_length = sizeof(video_probe_and_commit_control_t);
volatile unsigned char  __attribute__((aligned(4))) vs_probe_info = 0x03;
volatile unsigned char  __attribute__((aligned(64))) usb_spi_buffer[4096];



void SPI_Init(void)
{
    unsigned int cfg;
    volatile SPI_t *spi;

    spi = (volatile SPI_t *)0x80050000;
    // configure the SPI
    cfg = SPI_MODE_MASTER | SPI_MODE_FSIZE_8BIT | SPI_MODE_DIR_MSB | SPI_MODE_SPIMODE_3 | SPI_MODE_CLK_DIV16_EN;
    spi->MODE = cfg;
    spi->MASK = SPI_EVENT_NONE;
    spi->SLVSEL = 0xFFFFFFFF;

    // enable the SPI block
    spi->MODE = cfg | SPI_MODE_EN;
}

unsigned int SPI_Read(void)
{
    unsigned int dummy, i, length;
    volatile SPI_t *spi;

    spi = (volatile SPI_t *)0x80050000;
    spi->SLVSEL = 0;
    spi->EVENT = 0xFFFFFFFF;
    length = usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x10;

    // send cmd
    dummy = (usb_ctrl.interruptSignal[USB_SETUP03_IDX]>>0x18)&0xFF;
    spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
    while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
    dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
    // send address, spi address range capped at 2MB
    dummy = (usb_ctrl.interruptSignal[USB_SETUP03_IDX]>>0x10)&0xFF;
    if (dummy < 0x20)
    {
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
        dummy = (usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x08)&0xFF;
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
        dummy = (usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x00)&0xFF;
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
    }
    // transfer payload
    for (i=0; i<length; i++)
    {
        spi->TDATA = 0;
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        usb_spi_buffer[i] = SPI_RDATA_8BIT_MSB(spi->RDATA);
    }
    spi->SLVSEL = 0xFFFFFFFF;
    return 0;
}

unsigned int SPI_Write(void)
{
    unsigned int dummy, i, length;
    volatile SPI_t *spi;

    spi = (volatile SPI_t *)0x80050000;
    spi->SLVSEL = 0;
    spi->EVENT = 0xFFFFFFFF;
    length = usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x10;

    // send cmd
    dummy = (usb_ctrl.interruptSignal[USB_SETUP03_IDX]>>0x18)&0xFF;
    spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
    while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
    dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
    // send address, spi address range capped at 2MB
    dummy = (usb_ctrl.interruptSignal[USB_SETUP03_IDX]>>0x10)&0xFF;
    if (dummy < 0x20)
    {
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
        dummy = (usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x08)&0xFF;
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
        dummy = (usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x00)&0xFF;
        spi->TDATA = SPI_TDATA_8BIT_MSB(dummy);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
    }
    // transfer payload
    for (i=0; i<length; i++)
    {
        spi->TDATA = SPI_TDATA_8BIT_MSB(usb_spi_buffer[i]);
        while ( (spi->EVENT & SPI_EVENT_RFNE) == 0 );
        dummy = SPI_RDATA_8BIT_MSB(spi->RDATA);
    }
    spi->SLVSEL = 0xFFFFFFFF;
    return 0;
}




void USB_InitCallback(void)
{
    unsigned int  i_frame, i, j;
    //build uvc configuration frame
    i_frame = 0;
    for (i = 0; i < sizeof(config_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.config + i);
    }
    for (i = 0; i < sizeof(iad_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.iad + i);
    }
    //insert video control configurations
    for (i = 0; i < sizeof(interface_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.vc_interface_if00 + i);
    }
    for (i = 0; i < sizeof(vc_interface_header_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.vc_interface_header_if00 + i);
    }
    for (i = 0; i < sizeof(vc_camera_terminal_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.vc_camera_terminal_cam + i);
    }
    for (i = 0; i < sizeof(vc_output_terminal_desc_t); i_frame++, i++)
    {
        *(uvc_config_frame + i_frame) = *((unsigned char *)&uvc_config.vc_output_terminal_usb + i);
    }

    for(j = 0 ; j < VIDEO_INTERFACES_NR; ++j)
    {
        //insert video stream configurations
        if(video_interface_array[j].type == YUV)
            for (i = 0; i < uvc_vs_uncompressed_sizeof; i_frame++, i++)
            {
                *(uvc_config_frame + i_frame) = *((unsigned char *)video_interface_array[j].vs_yuv + i);
            }

        if(video_interface_array[j].type == MPEG2TS)
            for (i = 0; i < uvc_vs_compressed_sizeof; i_frame++, i++)
            {
                *(uvc_config_frame + i_frame) = *((unsigned char *)video_interface_array[j].vs_mpeg + i);
            }
    }

    // SPI driver init
    SPI_Init();
}

void USB_HandlerCallback(void)
{
    unsigned int opcode, length;

    opcode = usb_ctrl.interruptSignal[USB_SETUP03_IDX]&0xFFFF;
    // trigger on SPI READ operation
    if ( opcode == 0x02C0 )
    {
        if (SPI_Read() == 0)
        {
            usb_ctrl.interruptSignal[USB_SETUP03_IDX] = 0;
            dQH[0][12] = (unsigned int)&usb_spi_buffer;
            dQH[0][13] = 0;
            USB_EPSetup(0);
            length = usb_ctrl.interruptSignal[USB_SETUP47_IDX]>>0x10;
            if (length != 0)
            {
                dQH[1][12] = (unsigned int)&usb_spi_buffer;
                dQH[1][13] = length;
                USB_EPSetup(1);
            }
        }
    }
    // trigger on SPI WRITE operation
    if ( (opcode == 0x0240) && ((dTD[0][1]&0x80) == 0) )
    {
        if (SPI_Write() == 0)
        {
            usb_ctrl.interruptSignal[USB_SETUP03_IDX] = 0;
            dQH[1][12] = (unsigned int)&usb_spi_buffer;
            dQH[1][13] = 0;
            USB_EPSetup(1);
        }
    }
}

unsigned int USB_SetupCallback(unsigned int idx, unsigned int data03, unsigned int data47)
{
    unsigned int setup_stall;
    unsigned int bmRequestType, bRequest, wValue, wIndex, wLength;
    unsigned char *ip;
    int requestTypeRecongnized = 0;
    int i;

    bmRequestType = (data03>>0x00)&0xFF;
    bRequest = (data03>>0x08)&0xFF;
    wValue = (data03>>0x10)&0xFFFF;
    wIndex = (data47>>0x00)&0xFFFF;
    wLength = (data47>>0x10)&0xFFFF;
    setup_stall = 0;
    // identify standard request
    if ( ((bmRequestType>>0x05)&0x03) == 0 )
    {
        requestTypeRecongnized = 1;
        switch (bRequest)
        {
            case GET_STATUS:
            // prepare control phase
            if (wLength > sizeof(usb_status)) wLength = sizeof(usb_status);
            dQH[idx][12] = (unsigned int)&usb_status[0];
            dQH[idx][13] = 0;
            dQH[idx+1][12] = (unsigned int)&usb_status[0];
            dQH[idx+1][13] = wLength;
            break;
            case SET_ADDRESS:
            // write address into hw registers
            USB_SetAddress(wValue);
            // store address in data structure
            usb_ctrl.deviceStat = (usb_ctrl.deviceStat&0xFFFF00FF) | (wValue<<0x08);
            // prepare control phase
            dQH[idx][12] = 0;
            dQH[idx][13] = 0;
            dQH[idx+1][12] = (unsigned int)&usb_ctrl.deviceStat + 0x01;
            dQH[idx+1][13] = 0;
            break;
            case GET_DESCRIPTOR:
            if ((wValue>>0x08) == DEVICE)
            {
                if (wLength > sizeof(device_desc_t)) wLength = sizeof(device_desc_t);
                // prepare control phase
                dQH[idx][12] = (unsigned int)&uvc_device;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)&uvc_device;
                dQH[idx+1][13] = wLength;
            } else
            if ((wValue>>0x08) == CONFIGURATION)
            {
                if (wLength > uvc_config_sizeof) wLength = uvc_config_sizeof;
                // prepare control phase
                dQH[idx][12] = (unsigned int)uvc_config_frame;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)uvc_config_frame;
                dQH[idx+1][13] = wLength;
            } else
            if ((wValue>>0x08) == DEVICE_QUALIFIER)
            {
                if (wLength > sizeof(device_qualifier_desc_t)) wLength = sizeof(device_qualifier_desc_t);
                // prepare control phase
                dQH[idx][12] = (unsigned int)&uvc_device_qualifier;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)&uvc_device_qualifier;
                dQH[idx+1][13] = wLength;
            } else
            {
                setup_stall = 1;
            }
            break;
            case GET_CONFIGURATION:
            // prepare control phase
            dQH[idx][12] = (unsigned int)&usb_ctrl.deviceStat;
            dQH[idx][13] = 0;
            dQH[idx+1][12] = (unsigned int)&usb_ctrl.deviceStat;
            dQH[idx+1][13] = wLength;
            break;
            case SET_CONFIGURATION:
            if (wValue < 0x02)
            {
                // store address in data structure
                usb_ctrl.deviceStat = (usb_ctrl.deviceStat&0xFFFFFF00) | wValue;
                // prepare control phase
                dQH[idx][12] = 0;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)&usb_ctrl.deviceStat;
                dQH[idx+1][13] = 0;
            } else
            {
                setup_stall = 1;
            }
            break;
            case GET_INTERFACE:
            if (wIndex<UVC_DEVICE_NR_INTERFACE)
            {
                ip = (unsigned char *)((unsigned int)(&usb_ctrl.interfaceAlter03) + wIndex);
                // prepare control phase
                dQH[idx][12] = (unsigned int)ip;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)ip;
                dQH[idx+1][13] = wLength;
            } else
            {
                setup_stall = 1;
            }
            break;
            case SET_INTERFACE:
            setup_stall = 1;
            if (wIndex<UVC_DEVICE_NR_INTERFACE)
            {
                ip = (unsigned char *)((unsigned int)(&usb_ctrl.interfaceAlter03) + wIndex);
                // prepare control phase
                dQH[idx][12] = 0;
                dQH[idx][13] = 0;
                dQH[idx+1][12] = (unsigned int)ip;
                dQH[idx+1][13] = 0;
                // see if valid interface was requested
                if ( (wIndex==0) && (wValue<=0) ) setup_stall = 0;

                for(i = 0 ; i < VIDEO_INTERFACES_NR; ++i)
                {
                    if ( (wIndex==video_interface_array[i].interface_index) && (wValue<=1) )
                        setup_stall = 0;
                }
                if (setup_stall == 0)
                {
                    // disable old interface
                    for(i = 0 ; i < VIDEO_INTERFACES_NR; ++i)
                    {
                        if ( (wIndex==video_interface_array[i].interface_index) && (*ip!=0) )
                            video_interface_array[i].disable();
                    }
                    // update interface settings
                    *ip=wValue;
                    // enable new interface
                    for(i = 0 ; i < VIDEO_INTERFACES_NR; ++i)
                    {
                        if ( (wIndex==video_interface_array[i].interface_index) && (wValue!=0) )
                            video_interface_array[i].enable();
                    }
                }
            }
            break;
            default:
            setup_stall = 1;
            break;
        }
    }
    // identify vendor specific READ requests
    else if (bmRequestType == 0xC0)
    {
        requestTypeRecongnized = 1;
        switch (bRequest)
        {
        case USB_TRANSFER_SPI:
            usb_ctrl.interruptSignal[USB_SETUP03_IDX] = data03;
            usb_ctrl.interruptSignal[USB_SETUP47_IDX] = data47;
            dQH[idx][12] = 0;
            dQH[idx][13] = 0;
            dQH[idx+1][12] = 0;
            dQH[idx+1][13] = 0;
            break;
        default:
            setup_stall = 1;
            break;
        }
    }
    // identify vendor specific WRITE requests
    else if (bmRequestType == 0x40)
    {
        requestTypeRecongnized = 1;
        switch (bRequest)
        {
        case USB_TRANSFER_SPI:
            usb_ctrl.interruptSignal[USB_SETUP03_IDX] = data03;
            usb_ctrl.interruptSignal[USB_SETUP47_IDX] = data47;
            if ( wLength != 0 )
            {
                dQH[idx][12] = (unsigned int)&usb_spi_buffer;
                dQH[idx][13] = wLength;
            } else
            {
                dQH[idx][12] = 0;
                dQH[idx][13] = 0;
            }
            dQH[idx+1][12] = 0;
            dQH[idx+1][13] = 0;
            break;
        default:
            setup_stall = 1;
            break;
        }
    }
    // identify video control requests
    else if ( (((bmRequestType>>0x05)&0x03) == 0x01) && ((wIndex&0xFF) == 0x00) )
    {
        requestTypeRecongnized = 1;
        // reject unrecognized video control commands
        setup_stall = 1;
    }
    else
    {
        for (i = 0 ; i < VIDEO_INTERFACES_NR ; ++i)
        // identify video streaming requests
        if ( (((bmRequestType>>0x05)&0x03) == 0x01) && ((wIndex&0xFF) == video_interface_array[i].interface_index) )
        {
            requestTypeRecongnized = 1;
            // probe command
            if ((wValue>>0x08) == VS_PROBE_CONTROL)
            {
                switch (bRequest)
                {
                case SET_CUR:
                    if (wLength>sizeof(video_probe_and_commit_control_t)) wLength = sizeof(video_probe_and_commit_control_t);
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)video_interface_array[i].vs_probe_control_if;
                    dQH[idx][13] = wLength;
                    dQH[idx+1][12] = (unsigned int)video_interface_array[i].vs_probe_control_if;
                    dQH[idx+1][13] = 0;
                    break;
                case GET_CUR:
                case GET_MIN:
                case GET_MAX:
                case GET_DEF:
                    if (wLength>sizeof(video_probe_and_commit_control_t)) wLength = sizeof(video_probe_and_commit_control_t);
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)video_interface_array[i].vs_probe_control_if;
                    dQH[idx][13] = 0;
                    dQH[idx+1][12] = (unsigned int)video_interface_array[i].vs_probe_control_if;
                    dQH[idx+1][13] = wLength;

                    if(video_interface_array[i].type == YUV)
                    {
                        video_interface_array[i].probe(bRequest);
                    }
                    else if(video_interface_array[i].type == MPEG2TS)
                    {
                        video_interface_array[i].probe(bRequest);
                    }
                    break;
                case GET_LEN:
                    if (wLength>0x02) wLength = 0x02;
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)&vs_probe_length;
                    dQH[idx][13] = 0;
                    dQH[idx+1][12] = (unsigned int)&vs_probe_length;
                    dQH[idx+1][13] = wLength;
                    break;
                case GET_INFO:
                    if (wLength>0x01) wLength = 0x01;
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)&vs_probe_info;
                    dQH[idx][13] = 0;
                    dQH[idx+1][12] = (unsigned int)&vs_probe_info;
                    dQH[idx+1][13] = wLength;
                    break;
                default:
                    setup_stall = 1;
                    break;
                }
            }
            // commit command
            else if ((wValue>>0x08) == VS_COMMIT_CONTROL)
            {
                switch (bRequest)
                {

                case SET_CUR:
                    if (wLength>sizeof(video_probe_and_commit_control_t)) wLength = sizeof(video_probe_and_commit_control_t);
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)video_interface_array[i].vs_commit_control_if;
                    dQH[idx][13] = wLength;
                    dQH[idx+1][12] = (unsigned int)video_interface_array[i].vs_commit_control_if;
                    dQH[idx+1][13] = 0;
                    break;
                case GET_CUR:
                    if (wLength>sizeof(video_probe_and_commit_control_t)) wLength = sizeof(video_probe_and_commit_control_t);
                    // prepare control phase
                    dQH[idx][12] = (unsigned int)video_interface_array[i].vs_commit_control_if;
                    dQH[idx][13] = 0;
                    dQH[idx+1][12] = (unsigned int)video_interface_array[i].vs_commit_control_if;
                    dQH[idx+1][13] = wLength;
                    break;
                default:
                    setup_stall = 1;
                    break;
                }
            } else
            {
                // reject unrecognized video streaming commands
                setup_stall = 1;
            }
        }
    }
    // couldn't recognize request
    if (!requestTypeRecongnized)
    {
        setup_stall = 1;
    }
    return setup_stall;
}
