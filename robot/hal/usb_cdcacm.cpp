#include "anki/cozmo/robot/hal.h"
#include "movidius.h"
#include "usbDefines.h"

#define USB_WORD(x) *(volatile u32*)(USB_BASE_ADDR + (x))

#define DQH_CAPABILITIES            0
#define DQH_CURRENT_DTD_POINTER     1
#define DQH_NEXT_DTD_POINTER        2
#define DQH_TOKEN                   3
#define DQH_BUFFER_POINTER_0        4
#define DQH_BUFFER_POINTER_1        5
#define DQH_BUFFER_POINTER_2        6
#define DQH_BUFFER_POINTER_3        7
#define DQH_BUFFER_POINTER_4        8
#define DQH_RESERVED                9
#define DQH_SETUP_BUFFER_0          10
#define DQH_SETUP_BUFFER_1          11
// Reserved data used as temporary storage
#define DQH_CUSTOM_BUFFER           12
#define DQH_CUSTOM_LENGTH           13

#define DTD_TERMINATE               1

#define DTD_NEXT_DTD_POINTER        0
#define DTD_TOKEN                   1
#define DTD_BUFFER_POINTER_0        2
#define DTD_BUFFER_POINTER_1        3
#define DTD_BUFFER_POINTER_2        4
#define DTD_BUFFER_POINTER_3        5
#define DTD_BUFFER_POINTER_4        6

// Request Type Definitions
#define RT_DIRECTION_SHIFT          7
#define RT_DIRECTION_MASK           1
#define RT_DIRECTION_HOST_TO_DEVICE 0
#define RT_DIRECTION_DEVICE_TO_HOST 1

#define RT_TYPE_SHIFT               5
#define RT_TYPE_MASK                3
#define RT_TYPE_STANDARD            0
#define RT_TYPE_CLASS               1
#define RT_TYPE_VENDOR              2
#define RT_TYPE_RESERVED            3

#define RT_RECIPIENT_SHIFT          0
#define RT_RECIPIENT_MASK           0x1f
#define RT_RECIPIENT_DEVICE         0
#define RT_RECIPIENT_INTERFACE      1
#define RT_RECIPIENT_ENDPOINT       2
#define RT_RECIPIENT_OTHER          3

// We can safely transmit 64 bytes per packet
#define MAX_PACKET_SIZE             64
#define MAX_PACKET_HI               ((MAX_PACKET_SIZE >> 8) & 0xFF)
#define MAX_PACKET_LO               (MAX_PACKET_SIZE & 0xFF)
#define MAX_PACKET_EP0              0x40

#define BSWAP32(x)                  ((((x) >> 24) & 0xFF) | \
                                      (((x) & 0xFF) << 24) | \
                                      (((x) >> 8) & 0xFF00) | \
                                      (((x) & 0xFF00) << 8))

#define S4(a, b, c, d)              (d), (c), (b), (a)
#define S3(a, b, c)                 S4(a, b, c, 0)
#define S2(a, b)                    S3(a, b, 0)
#define S1(a)                       S2(a, 0)

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static volatile u8 __attribute__((aligned(4))) m_USBStatus[2] = { 0x01, 0x00 };  // Self-Powered, No Remote Wakeup
      static volatile u8 __attribute__((aligned(4))) m_interface = 0;
      static volatile u32 __attribute__((aligned(4))) m_deviceStatus = 0;
      static volatile u32 __attribute__((aligned(4))) m_deviceClock = 0;
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_IN[32];
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_OUT[32];
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_DIO[32];
      static volatile u32 __attribute__((aligned(1024)))  m_dTD[32][8];
      static volatile u32 __attribute__((aligned(2048)))  m_dQH[64][16];

      volatile u8 __attribute__((aligned(4096))) m_endpointBuffer[1024];
      volatile u8 __attribute__((aligned(4096))) m_IN[1024];

      const endpoint_desc_t __attribute__((aligned(64))) usb_control_out_endpoint =
      {
          sizeof(endpoint_desc_t),
          ENDPOINT,
          0x00,  // OUT
          (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
          MAX_PACKET_EP0,
          0x01,
      };

      const endpoint_desc_t __attribute__((aligned(64))) usb_control_in_endpoint =
      {
          sizeof(endpoint_desc_t),
          ENDPOINT,
          0x80,  // IN
          (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
          MAX_PACKET_EP0,
          0x01,
      };

      // Serial endpoints
      const endpoint_desc_t __attribute__((aligned(64))) usb_ep3_in =
      {
        sizeof(endpoint_desc_t),
        ENDPOINT,
        0x83,  // IN
        (EP_TT_INTERRUPT | EP_ST_ASYNC | EP_UT_DATA),
        MAX_PACKET_EP0,
        0x01,
      };

      const endpoint_desc_t __attribute__((aligned(64))) usb_ep2_in =
      {
        sizeof(endpoint_desc_t),
        ENDPOINT,
        0x82,  // IN
        (EP_TT_BULK | EP_ST_ASYNC | EP_UT_DATA),
        MAX_PACKET_SIZE,
        0x01,
      };

      const endpoint_desc_t __attribute__((aligned(64))) usb_ep1_out =
      {
        sizeof(endpoint_desc_t),
        ENDPOINT,
        0x01,  // OUT
        (EP_TT_BULK | EP_ST_ASYNC | EP_UT_DATA),
        MAX_PACKET_SIZE,
        0x01,
      };

      static u8 m_lineCoding[] =
      {
     S4(0x80, 0x25, 0x00, 0x00), // DTERate == 9600
     S3(0x00,                   // CharFormat
        0x00,                   // ParityType
        0x08)                    // DataBits
      };

      const u8 m_deviceDescriptor[] =
      {
     S4(0x12,             // bLength
        DEVICE,           // bDescriptorType
        0x00, 0x02),       // bcdUSB == Version 2.00
     S4(0x02,             // bDeviceClass == Communication Device Class
        0x00,             // bDeviceSubClass
        0x00,             // bDeviceProtocol
        0x40),             // bMaxPacketSize
     S4(0x08, 0x21,       // Vendor ID
        0x0b, 0x78),       // Product ID
     S4(0x00, 0x01,       // bcdDevice == Version 1.00
        0x01,             // iManifacturer == Index to string manufacturer descriptor
        0x02),             // iProduct == Index to string product descriptor
     S2(0x03,             // iSerialNumber == Index to string serial number
        0x01)             // bNumConfigurations
      };

      const u8 m_stringDescriptorLanguageID[] =
      {
     S4(0x04,             // bLength
      STRING,           // bDescriptorType
        0x09, 0x04)       // wLANGID0 == English - United States
      };

      const u8 m_stringDescriptorManufacturer[] =
      {
     S4(0x0A,             // bLength
        STRING,           // bDescriptorType
        'A', 0x00),
     S4('n', 0x00,
        'k', 0x00),
     S2('i', 0x00)
      };

      const u8 m_stringDescriptorProduct[] =
      {
     S4(0x0C,             // bLength
        STRING,           // bDescriptorType
        'C', 0x00),
     S4('o', 0x00,
        'z', 0x00),
     S4('m', 0x00,
        'o', 0x00)
      };

      // TODO: Maybe get this from flash?
      const u8 m_stringDescriptorSerialNumber[] =
      {
     S4(0x10,             // bLength
        STRING,           // bDescriptorType
        'A', 0x00),
     S4('B', 0x00,
        'C', 0x00),
     S4('D', 0x00,
        'E', 0x00),
     S4('F', 0x00,
        '0', 0x00)
      };

      const u8* m_stringDescriptors[] =
      {
        m_stringDescriptorLanguageID,
        m_stringDescriptorManufacturer,
        m_stringDescriptorProduct,
        m_stringDescriptorSerialNumber
      };

      const u8 m_configurationDescriptor[] =
      {
          // Configuration Descriptor
     S4(0x09,             // bLength
        CONFIGURATION,    // bDescriptorType
        0x43, 0x00),       // wTotalLength
     S4(0x02,             // bNumInterfaces
        0x01,             // bConfigurationValue
        0x00,             // iConfiguration == Index to string descriptor
        0xC0),             // bmAttributes == bit[7]: Compatability with USB 1.0
                          //                 bit[6]: 1 if Self-Powered, 0 if Bus-Powered
                          //                 bit[5]: Remote Wakeup
                          //                 bits[4:0]: Reserved
     S4(0x00,             // bMaxPower == 0 mA

        // Interface Descriptor
        0x09,             // bLength
        INTERFACE,        // bDescriptorType
        0x00),             // bInterfaceNumber
     S4(0x00,             // bAlternateSetting
        0x01,             // bNumEndpoints
        0x02,             // bInterfaceClass == Communication Interface Class
        0x02),             // bInterfaceSubClass == Abstract Control Model
     S4(0x00,             // bInterfaceProtocol == Common AT Commands
        0x00,             // iInterface == Index to string descriptor

        // Header Functional Descriptor
        0x05,             // bLength
        0x24),             // bDescriptorType == CS_INTERFACE
     S4(0x00,             // bDescriptorSubType == Header Functional Descriptor
        0x10, 0x01,       // bcdCDC

        // Call Management Functional Descriptor
        0x05),             // bLength
     S4(0x24,             // bDescriptorType == CS_INTERFACE
        0x01,             // bDescriptorSubType == Call Management Functional Descriptor
        0x00,             // bmCapabilities
        0x01),             // bDataInterface

        // Abstract Control Management Functional Descriptor
     S4(0x04,             // bLength
        0x24,             // bDescriptorType == CS_INTERFACE
        0x02,             // bDescriptorSubType == Abstract Control Management Functional Descriptor
        0x00),             // bmCapabilities

        // Union Functional Descriptor
     S4(0x05,             // bLength
        0x24,             // bDescriptorType == CS_INTERFACE
        0x06,             // bDescriptorSubType == Union Functional Descriptor
        0x00),             // bMasterInterface
     S4(0x01,             // bSlaveInterface0

        // Endpoint 3 IN Descriptor
        0x07,             // bLength
        ENDPOINT,         // bDescriptorType
        0x83),  // bEndpointAddress == IN[3]
     S4(0x03,             // bmAttributes == Interrupt
        0x08,     // wMaxPacketSize
        0x00,
        0x01),             // bInterval

        // Data Class Interface Descriptor
     S4(0x09,             // bLength
        INTERFACE,        // bDescriptorType
        0x01,             // bInterfaceNumber
        0x00),             // bAlternateSetting
     S4(0x02,             // bNumEndpoints
        0x0a,             // bInterfaceClass == CDC
        0x00,             // bInterfaceSubClass
        0x00),             // bInterfaceProtocol
     S4(0x00,             // iInterface

        // Endpoint 1 OUT Descriptor
        0x07,             // bLength
        ENDPOINT,         // bDescriptorType
        0x01),  // bEndpointAddress == OUT[1]
     S4(0x02,             // bmAttributes == Bulk
        MAX_PACKET_LO,    // wMaxPacketSize
        MAX_PACKET_HI,
        0x01),             // bInterval == Ignore for Bulk transfer

        // Endpoint 2 IN Descriptor
     S4(0x07,             // bLength
        ENDPOINT,         // bDescriptorType
        0x82,  // bEndpointAddress == IN[2]
        0x02),             // bmAttributes == Bulk
     S3(MAX_PACKET_LO,    // wMaxPacketSize
        MAX_PACKET_HI,
        0x01)             // bInterval == Ignore for Bulk transfer
      };

      const u8 m_deviceQualifier[] =
      {
     S4(0x0A,             // bLength
        DEVICE_QUALIFIER, // bDescriptorType
        0x00, 0x02),       // bcdUSB
     S4(0x00,             // bDeviceClass
        0x00,             // bDeviceSubClass
        0x00,             // bDeviceProtocol
        MAX_PACKET_EP0),  // bMaxPacketSize0
     S2(0x01,             // bNumConfigurations
        0x00)              // bReserved
      };

      static void Reset();
      static void HandleSetupMessage();
      static void EPSetup(u32 index);
      static bool Setup(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool StandardRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool ClassRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool VendorRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static void SetupTransfer(u32 index, u32 addressOUT, u32 lengthOUT, u32 addressIN, u32 lengthIN);

      static void EPStart(const endpoint_desc_t* ep)
      {
        unsigned int index, cap, type, ctrl;
        bool isIN = false;

        printf("starting ep: %02X\n", ep->bEndpointAddress);

        // Get the address for ENDPTCTRL[index]
        index = ep->bEndpointAddress & 0x0F;
        ctrl = REG_ENDPTCTRL0 + (index << 2); 

        // Note: even indices in dQH are for OUT endpoints and odd indices are
        // for IN endpoints
        index <<= 1;
        if (ep->bEndpointAddress & 0x80)
        {
          isIN = true;
          index++;
        }

        type = ep->bmAttributes & 0x03;

        // Add Zero Length Termination select
        cap = 0;
        //cap |= 0x20000000;

        // Add Mult (number of packets executed per transaction)
        if (type == EP_TT_ISOCHRONOUS)
          cap |= ((ep->wMaxPacketSize >> 11) + 1) << 30;

        // Add maximum packet length
        cap |= (ep->wMaxPacketSize & 0x7FF) << 16;

        // Add IOS (Interrupt On Setup)
        //if (type == EP_TT_CONTROL)
        //  cap |= 0x8000;

        volatile u32* dQH = m_dQH[index];
        volatile u32* dTD = m_dTD[index];

        // Write capabilities
        dQH[DQH_CAPABILITIES] = cap;
        // Write current dTD pointer
        dQH[DQH_CURRENT_DTD_POINTER] = 0;
        // Write next dTD pointer
        dQH[DQH_NEXT_DTD_POINTER] = DTD_TERMINATE;
        dQH[DQH_TOKEN] = 0;
        dQH[DQH_BUFFER_POINTER_0] = 0;
        dQH[DQH_BUFFER_POINTER_1] = 0;
        dQH[DQH_BUFFER_POINTER_2] = 0;
        dQH[DQH_BUFFER_POINTER_3] = 0;
        dQH[DQH_BUFFER_POINTER_4] = 0;

        // Clear transfer descriptor
        dTD[DTD_NEXT_DTD_POINTER] = DTD_TERMINATE;
        dTD[DTD_TOKEN] = 0;

        // Set the endpoint control
        //cap |= type << 2;
        if (isIN)
        {
          // TX Endpoint Reset and Enable
          USB_WORD(ctrl) |= (type << 18) | (1 << 22) | (1 << 23);
        } else {
          // RX Endpoint Reset and Enable
          USB_WORD(ctrl) |= (type << 2) | (1 << 6) | (1 << 7);
        }

        printf("%08X: %08X\n", ctrl, USB_WORD(ctrl));
      }

      static void EPSetup(u32 index)
      {
        u32 cap, type, mult, address, length;

        volatile u32* dQH = m_dQH[index];
        volatile u32* dTD = m_dTD[index];

        cap = dQH[DQH_CAPABILITIES];
        address = dQH[DQH_CUSTOM_BUFFER];
        length = dQH[DQH_CUSTOM_LENGTH];
        mult = cap >> 30;
        cap = (cap >> 16) & 0x7ff;

        if (!mult)
        {
          type = 0x0080;
        } else {
          type = 0x0480;
          if (length > cap)
            type = 0x0880;
          cap <<= 1;

          if (length > cap)
            type = 0x0c80;
        }

        // Add transfer length
        type |= (length << 16);
        // Write next dTD pointer as invalid
        dTD[DTD_NEXT_DTD_POINTER] = DTD_TERMINATE;
        // Write transfer control
        dTD[DTD_TOKEN] = type;
        // Write buffer pointers in 4KB chunks
        dTD[DTD_BUFFER_POINTER_0] = address;
        dTD[DTD_BUFFER_POINTER_1] = (address + 0x1000) & 0xFFFFf000;
        dTD[DTD_BUFFER_POINTER_2] = (address + 0x2000) & 0xFFFFf000;
        dTD[DTD_BUFFER_POINTER_3] = (address + 0x3000) & 0xFFFFf000;
        dTD[DTD_BUFFER_POINTER_4] = (address + 0x4000) & 0xFFFFf000;

        // Write dQH next pointer
        dQH[DQH_NEXT_DTD_POINTER] = (u32)dTD;
        dQH[DQH_TOKEN] &= ~((1 << 7) | (1 << 6));  // Clear Active | Halted

        // Prime the endpoint
        u32 prime = 1 << (index >> 1);
        if (index & 1)  // IN endpoint?
          prime <<= 16;

        // TODO: Verify this is correct in case of other pending setups
        // Ensure setup status has been cleared
        while (USB_WORD(REG_ENDPTSETUPSTAT))
          ;

        USB_WORD(REG_ENDPTPRIME) = prime;

        while (USB_WORD(REG_ENDPTPRIME) & prime)
          ;

        while (!(USB_WORD(REG_ENDPTSTAT) & prime))
          ;
      }

      static void InitEndpoints()
      {
        u32 index;

         // Setup serial/cdcacm endpoints
        EPStart(&usb_ep1_out);
        EPStart(&usb_ep2_in);
        EPStart(&usb_ep3_in);

        // Setup the OUT endpoint
        index = (1 << 1);
        SetupTransfer(index,
            (u32)m_endpointBuffer, MAX_PACKET_SIZE, //sizeof(m_endpointBuffer) >> 1,
            (u32)m_endpointBuffer, 0);
        EPSetup(index);

        m_IN[0] = 'H';
        m_IN[1] = 'e';
        m_IN[2] = 'l';
        m_IN[3] = 'l';
        m_IN[4] = 'o';
        m_IN[5] = '\n';
 
        // Setup the IN endpoints
        index = (2 << 1);
        SetupTransfer(index,
            (u32)m_IN, 0,
            (u32)m_IN, 0);
        EPSetup(index + 1);

        index = (3 << 1);
        SetupTransfer(index,
            (u32)m_endpointBuffer, 0,
            (u32)m_endpointBuffer, 0);
        EPSetup(index + 1);
      }

      static void InitDeviceController()
      {
        // Set OTG transceiver configuration
        USB_WORD(REG_OTGSC) |=  (USB_OTGSC_OT | USB_OTGSC_VC);
        // Clear run bit
        USB_WORD(REG_USBCMD) = 0;
        // Wait for bit to clear
        while (USB_WORD(REG_USBCMD) & USB_USBCMD_RS)
          ;
        // Set reset bit and reinitialize hardware
        USB_WORD(REG_USBCMD) |= USB_USBCMD_RST;
        // Wait for bit to clear
        while (USB_WORD(REG_USBCMD) & USB_USBCMD_RST)
          ;
        // Transceiver enable (force full-speed connect)
        USB_WORD(REG_PORTSC1) = (/*USB_PORTSCX_PFSC | */ USB_PORTSCX_PE);
        // Set USB address to 0
        USB_WORD(REG_DEVICE_ADDR) = 0;
        // Set the maximum burst length (in 32-bit words)
        USB_WORD(REG_BURSTSIZE) = 0x1010;
        USB_WORD(REG_SBUSCFG) = 0;
        // Set the USB controller to DEVICE mode and disable "setup lockout mode"
        USB_WORD(REG_USBMODE) = (USBMODE_DEVICE_MODE | USB_USBMODE_SLOM);
        
        // Configure the endpoint-list address
        USB_WORD(REG_ENDPOINTLISTADDR) = (u32)&m_dQH;

        // Setup control endpoints
        EPStart(&usb_control_out_endpoint);
        EPStart(&usb_control_in_endpoint);

        //InitEndpoints();

        // Set the run bit
        USB_WORD(REG_USBCMD) = USB_USBCMD_RS;
        // Dual poll for SRI to sync with host
        while (!(USB_WORD(REG_USBSTS) & USB_USBSTS_SRI))
          ;
        // Clear SOF
        USB_WORD(REG_USBSTS) = USB_USBSTS_SRI;
        // Clear IRQ
        REG_WORD(ICB_INT_CLEAR) = (1 << IRQ_USB);
        // Configure interrupt level and edge
        //SET_REG_WORD(ICB_USB_CONFIG, (IRQ_POS_LEVEL | (IRQ_USB_PRIO << 2)));
        // Enable USB interrupts
        //USB_WORD(USBINTR) = USBSTS_SRI;

        Reset();
      }

      static void Reset()
      {
        // Clear all setup token semaphores
        USB_WORD(REG_ENDPTSETUPSTAT) = USB_WORD(REG_ENDPTSETUPSTAT);
        // Clear all the endpoint complete status bits
        USB_WORD(REG_ENDPTCOMPLETE) = USB_WORD(REG_ENDPTCOMPLETE);
        // Cancel all primed status
        while (USB_WORD(REG_ENDPTPRIME))
          ;
        // Flush all primed endpoints
        USB_WORD(REG_ENDPTFLUSH) = 0xFFFFffff;

        // TODO:
        // Read the reset bit in the PORTSCx register and make sure it is still
        // active. A USB reset will occur for a minimum of 3 ms and the DCD
        // must reach this point in the reset cleanup before the end of a reset
        // occurs, otherwise a hardware reset of the device controller is
        // recommended (rare).

        //printf("port: %08X\n", USB_WORD(REG_PORTSC1));

        USB_WORD(REG_DEVICE_ADDR) = 0;
      }

      void USBInit()
      {
        // USB calibration (from Movidius)
        REG_WORD(CPR_USB_CTRL_ADR) = 0x00003004;
        REG_WORD(CPR_USB_PHY_CTRL_ADR) = 0x01C18d07;

        // Initialize the hardware
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_USB);
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_USB);

        InitDeviceController();

        // XXX: Not including the setup for interrupts...
        //USB_WORD(REG_USBINTR) |= USB_USBINTR_UE;

        // TODO: Add IN-addresses?
      //    m_dTD_IN[0] = (unsigned int)&m_dTD_IN[8];
      //    m_dTD_IN[8] = (unsigned int)&m_dTD_IN[16];
      //    m_dTD_IN[16] = (unsigned int)&m_dTD_IN[24];
      //    m_dTD_IN[24] = (unsigned int)&m_dTD_IN[0];

        m_deviceStatus =
          (m_deviceStatus & 0xFFFF) | (USB_WORD(REG_FRINDEX) << 16);
      }

      void USBSend(u8* buffer, u32 length)
      {
        // TODO: Implement
      }

      void USBUpdate()
      {
        u32 status = USB_WORD(REG_USBSTS);
        // Check for USB Reset
        if (status & USB_USBSTS_URI)
        {
          //printf("RESET\n");
          // Clear USB Reset Received
          USB_WORD(REG_USBSTS) = USB_USBSTS_URI;
          Reset();
        }
        // Check for Port Change
        if (status & USB_USBSTS_PCI)
        {
          //printf("PORT CHANGE\n");
          // TODO: Anything else required here?
          //USB_WORD(REG_USBSTS) = USB_USBSTS_PCI;
        }
        // Check for suspended state
        if (status & USB_USBSTS_SLI)
        {
          //printf("SLI\n");
          //USB_WORD(REG_USBSTS) = USB_USBSTS_SLI;
        }
        // Check for SOF (Start Of Frame)
        if (status & USB_USBSTS_SRI)
        {
          //printf("USBSTS: %08X\n", USB_WORD(REG_USBSTS));

          // Clear SOF
          USB_WORD(REG_USBSTS) = USB_USBSTS_SRI;
          while (USB_WORD(REG_USBSTS) & USB_USBSTS_SRI)
            ;

          //SET_USB_WORD(ICB_INT_CLEAR, (1 << IRQ_USB));

          // Increment the clock counter
          m_deviceClock++;

          HandleSetupMessage();

          // Update the frame index
          m_deviceStatus =
            (m_deviceStatus & 0xFFFF) | (USB_WORD(REG_FRINDEX) << 16);

          static u32 old = 0;
          if ((HAL::GetMicroCounter() - old) >= 250000)
          {
            old = HAL::GetMicroCounter();
            printf("buff: %02X %02X %02X %02X\n", 
                m_endpointBuffer[0],
                m_endpointBuffer[1],
                m_endpointBuffer[2],
                m_endpointBuffer[3]);
            printf("data: %08X\n", m_dQH[2][3]);
          }

          static volatile u8 sendBUFFER = 0;
          if (!sendBUFFER && (m_endpointBuffer[3] == 'a'))
          {
            printf("sending... %02X\n", sendBUFFER);
            sendBUFFER = 1;
            printf("true == %02X\n", sendBUFFER);

            u32 index = (2 << 1);
            SetupTransfer(index,
                (u32)m_IN, 0,
                (u32)m_IN, 6);
            printf("SetupTransfer == %02X\n", sendBUFFER);
            EPSetup(index + 1);
            printf("EPSetup == %02X\n", sendBUFFER);
          }

/*          static u8 a = 0, b = 0, c = 0;

          u8 a1 = m_dQH[2][2] & 0x7F;
          u8 b1 = m_dQH[5][2] & 0x7F;
          u8 c1 = m_dQH[7][2] & 0x7F;

          if (a1)
          {
            USB_WORD(REG_ENDPTFLUSH) |= 1 << 1;
          }
          if (b1)
          {
            USB_WORD(REG_ENDPTFLUSH) |= 1 << (16 + 2);
          }
          if (c1)
          {
            USB_WORD(REG_ENDPTFLUSH) |= 1 << (16 + 3);
          }

          if (a1 != a || b1 != b || c1 != c)
          {
            a = a1;
            b = b1;
            c = c1;
            printf("%02X %02X %02X\n", a1, b1, c1);
          }

          while (USB_WORD(REG_ENDPTFLUSH))
            ;

          // .........
          if (a1)
          {
            SetupTransfer(3, (u32)m_endpointBuffer, 0x10,
                (u32)m_endpointBuffer, 0);
            EPSetup(3);
          }
          if (b1)
          {
            SetupTransfer(5, (u32)m_endpointBuffer, 0,
                (u32)m_endpointBuffer, 0);
            EPSetup(5);
          }
          if (c1)
          {
            SetupTransfer(7, (u32)m_endpointBuffer, 0,
                (u32)m_endpointBuffer, 0);
            EPSetup(7);
          } */

        }

        // Check for USB Interrupt
        if (status & USB_USBSTS_UI)
        {
          //printf("USB-I\n");
          //USB_WORD(REG_USBSTS) = USB_USBSTS_UI;
          // Handle USB control messages
          //HandleSetupMessage();
        }

        // TODO: Implement the rest
        // ...


        static u32 err = 0;
        if ((m_dQH[1][1] & 0x68) && err != m_dQH[1][1])
        {
          err = m_dQH[1][1];
          printf("dQH == %08X\n", m_dQH[1][1]);
        }

        //wasActive = (m_dQH[1][1] & 0x80) != 0;




      }

      bool isFull()
      {
        return true;
      }

      static void HandleSetupMessage()
      {
        u32 setupStatus, index, ctrl;
        bool setupStall;

        setupStatus = USB_WORD(REG_ENDPTSETUPSTAT);

        u32 bit = 1;
        for (u32 i = 0; i < 16; i++, bit <<= 1)
        {
          if (!(setupStatus & bit))
          {
            continue;
          }

          printf("found: %d\n", i);

          // Clear the setup status
          USB_WORD(REG_ENDPTSETUPSTAT) = bit;

          // Get the queue head index offset (two per endpoint)
          index = i << 1;

          // Setup tripwire for reading the setup bytes
          do
          {
            USB_WORD(REG_USBCMD) |= USB_USBCMD_SUTW;
          } while (!(USB_WORD(REG_USBCMD) & USB_USBCMD_SUTW));

          // Read out data from the setup buffer
          unsigned int data03 = (unsigned int)m_dQH[index][DQH_SETUP_BUFFER_0];
          unsigned int data47 = (unsigned int)m_dQH[index][DQH_SETUP_BUFFER_1];
          unsigned int bmRequestType = data03 & 0xFF;
          unsigned int bRequest = (data03 >> 8) & 0xFF;
          unsigned int wValue = (data03 >> 16) & 0xFFFF;
          unsigned int wIndex = data47 & 0xFFFF;
          unsigned int wLength = (data47 >> 16) & 0xFFFF;

          printf("bmRequestType: %02X, bRequest: %02X, wValue: %04X, wIndex: %04X, wLength: %04X\n",
              bmRequestType, bRequest, wValue, wIndex, wLength);

          setupStall = Setup(index, bmRequestType, bRequest, wValue, wIndex,
              wLength);

          // Clear tripwire
          USB_WORD(REG_USBCMD) &= ~USB_USBCMD_SUTW;

          if (setupStall)
          {
            // Stall the endpoint
            USB_WORD(REG_ENDPTCTRL0 + (i << 2)) = 0x00010001;
          } else {
            // Configure a data transfer
            if (m_dQH[index + 0][DQH_CUSTOM_BUFFER])
            {
              EPSetup(index + 0);
            }
            if (m_dQH[index + 1][DQH_CUSTOM_BUFFER])
            {
              EPSetup(index + 1);
            }
          }
        }
      }

      static bool Setup(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue,
          u32 wIndex, u32 wLength)
      {
        bool setupStall = true;

        switch ((bmRequestType >> RT_TYPE_SHIFT) & RT_TYPE_MASK)
        {
          case RT_TYPE_STANDARD:
            printf("STANDARD\n");
            setupStall = StandardRequestHandler(index, bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          case RT_TYPE_CLASS:
            printf("CLASS\n");
            setupStall = ClassRequestHandler(index, bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          case RT_TYPE_VENDOR:
            printf("VENDOR\n");
            setupStall = VendorRequestHandler(index, bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          default:
            printf("[ERROR] Unknown USB bmRequestType\n");
            break;
        }

        return setupStall;
      }

      static bool StandardRequestHandler(u32 index, u32 bmRequestType,
          u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = false;

        switch (bRequest)
        {
          case GET_STATUS:
          {
            if (wLength > sizeof(m_USBStatus))
              wLength = sizeof(m_USBStatus);
            SetupTransfer(index,
                (u32)m_USBStatus, 0,
                (u32)m_USBStatus, wLength);
            break;
          }

          case SET_ADDRESS:
          {
            USB_WORD(REG_DEVICE_ADDR) = ((wValue << 1) + 1) << 24;
            m_deviceStatus = (m_deviceStatus & 0xFFFF00FF) | (wValue << 8);
            // Following Movidius. Pointer to new address and 0 length.
            SetupTransfer(index,
                (u32)&m_deviceStatus, 0,
                (u32)&m_deviceStatus, 0);
            break;
          }

          case GET_DESCRIPTOR:
          {
            switch (wValue >> 8)
            {
              case DEVICE:
              {
                if (wLength > (sizeof(m_deviceDescriptor) - 2))
                  wLength = sizeof(m_deviceDescriptor) - 2;
                SetupTransfer(index,
                    (u32)m_deviceDescriptor, 0,
                    (u32)m_deviceDescriptor, wLength);
                break;
              }

              case CONFIGURATION:
              {
                if (wLength > (sizeof(m_configurationDescriptor) - 1))  // XXX: FIX FOR MYRIAD1 BSWAP
                  wLength = sizeof(m_configurationDescriptor) - 1;
                SetupTransfer(index,
                    (u32)m_configurationDescriptor, 0,
                    (u32)m_configurationDescriptor, wLength);
                break;
              }

              case DEVICE_QUALIFIER:
              {
                if (wLength > (sizeof(m_deviceQualifier) - 2))
                  wLength = sizeof(m_deviceQualifier) - 2;
                SetupTransfer(index,
                    (u32)m_deviceQualifier, 0,
                    (u32)m_deviceQualifier, wLength);
                break;
              }

              case STRING:
              {
                u32 stringIndex = wValue & 0xFF;
                if (stringIndex < sizeof(m_stringDescriptors) / sizeof(u8*))
                {
                  const u8* descriptor = m_stringDescriptors[stringIndex];
                  if (wLength > descriptor[3])  // First element is bLength  XXX: FIX FOR MYRIAD1 BSWAP
                    wLength = descriptor[3];
                  SetupTransfer(index,
                      (u32)descriptor, 0,
                      (u32)descriptor, wLength);
                }
                break;
              }

              default:
              {
                setupStall = true;
                break;
              }
            }
            break;
          }

          case GET_CONFIGURATION:
          {
            if (wLength > sizeof(m_deviceStatus))
              wLength = sizeof(m_deviceStatus);
            SetupTransfer(index,
                (u32)&m_deviceStatus, 0,
                (u32)&m_deviceStatus, wLength);
            break;
          }

          case SET_CONFIGURATION:
          {
            printf("SET_CONFIGURATION: %02X\n", wValue);

            if (wValue >= 2)  // TODO: Get rid of modem/AT config/interface
            {
              setupStall = true;
            } else {
              // Initialize the cdc acm endpoints
              if (wValue == 1)
              {
                InitEndpoints();
              }

              m_deviceStatus = (m_deviceStatus &~ 0xFF) | wValue;
              SetupTransfer(index,
                  0, 0,
                  (u32)&m_deviceStatus, 0);
            }
            break;
          }

          case GET_INTERFACE:
          {
            if (wLength > sizeof(m_interface))
              wLength = m_interface;
            SetupTransfer(index,
                (u32)&m_interface, 0,
                (u32)&m_interface, wLength);
            break;
          }

          case SET_INTERFACE:
          {
            m_interface = wIndex;
            SetupTransfer(index,
                0, 0,
                (u32)&m_interface, 0);
            break;
          }

          default:
          {
            setupStall = true;
            break;
          }
        }

        return setupStall;
      }

      static bool ClassRequestHandler(u32 index, u32 bmRequestType,
          u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = false;

        switch (bRequest)
        {
          case 0x00:  // SET_ENCAPSULATED_COMMAND
          {
            // TODO: Implement
            break;
          }

          case 0x01:  // GET_ENCAPSULATED_COMMAND
          {
            // TODO: Implement
            break;
          }

          case 0x20:  // SET_LINE_CODING
          {
            SetupTransfer(index,
                (u32)m_lineCoding, sizeof(m_lineCoding) - 1,  // XXX: FIX FOR MYRIAD1 BSWAP
                (u32)m_lineCoding, 0);
            break;
          }

          case 0x21:  // GET_LINE_CODING
          {
            SetupTransfer(index,
                (u32)m_lineCoding, 0,
                (u32)m_lineCoding, sizeof(m_lineCoding) - 1);  // XXX: FIX FOR MYRIAD1 BSWAP
            break;
          }

          case 0x22:  // SET_CONTROL_LINE_STATE
          {
            SetupTransfer(index,
                (u32)&m_lineCoding, 0,
                (u32)&m_lineCoding, 0);
            break;
          }

          case 0x23:  // SEND_BREAK
          {
            SetupTransfer(index,
                (u32)&m_lineCoding, 0,
                (u32)&m_lineCoding, 0);
            break;
          }

          default:
          {
            setupStall = true;
            break;
          }
        }

        return setupStall;
      }

      static bool VendorRequestHandler(u32 index, u32 bmRequestType,
          u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        // TODO: Implement

        bool setupStall = true;
        return setupStall;
      }

      static void SetupTransfer(u32 index, u32 addressOUT, u32 lengthOUT,
          u32 addressIN, u32 lengthIN)
      {
        printf("index == %i\n", index);
        m_dQH[index + 0][DQH_CUSTOM_BUFFER] = addressOUT;
        m_dQH[index + 0][DQH_CUSTOM_LENGTH] = lengthOUT;
        m_dQH[index + 1][DQH_CUSTOM_BUFFER] = addressIN;
        m_dQH[index + 1][DQH_CUSTOM_LENGTH] = lengthIN;
      }
    }
  }
}

