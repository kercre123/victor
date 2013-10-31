#include "anki/cozmo/robot/hal.h"
#include "movidius.h"
#include "usbDefines.h"

#define REG_WORD(x) *(volatile u32*)(USB_BASE_ADDR + (x))
#define REG_HALF(x) *(volatile u16*)(USB_BASE_ADDR + (x))

#define DQH_CAPABILITIES            0
#define DQH_CURRENT_DTD_POINTER     1
#define DQH_NEXT_DTD_POINTER        2
#define DQH_TOTAL_BYTES             3
#define DQH_BUFFER_POINTER_0        4
#define DQH_BUFFER_POINTER_1        5
#define DQH_BUFFER_POINTER_2        6
#define DQH_BUFFER_POINTER_3        7
#define DQH_BUFFER_POINTER_4        8
#define DQH_RESERVED                9
#define DQH_SETUP_BUFFER_BYTES_3_0  10
#define DQH_SETUP_BUFFER_BYTES_7_4  11
// Reserved data used as temporary storage
#define DQH_CUSTOM_BUFFER           12
#define DQH_CUSTOM_LENGTH           13

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

#define MAX_PACKET_SIZE             0x40

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static volatile unsigned char __attribute__((aligned(4))) m_USBStatus[2] = { 0x01, 0x00 };  // Self-Powered, No Remote Wakeup
      static volatile unsigned char __attribute__((aligned(4))) m_interface = 0;
      static volatile unsigned int __attribute__((aligned(4))) m_deviceStatus = 0;
      static volatile unsigned int __attribute__((aligned(4))) m_deviceClock = 0;
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_IN[32];
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_OUT[32];
      //  static volatile unsigned int __attribute__((aligned(64)))    m_dTD_DIO[32];
      static volatile unsigned int __attribute__((aligned(1024)))  m_dTD[32][8];
      static volatile unsigned int __attribute__((aligned(2048)))  m_dQH[32][16];

      //volatile unsigned char __attribute__((aligned(2048))) m_endpointData[1024];

      static u8 m_lineCoding[] =
      {
          0xC0, 0xC6, 0x2D, 0x00, // DTERate == 3 Mega-Baud
          0x00,                   // CharFormat
          0x00,                   // ParityType
          0x08                    // DataBits
      };

      const u8 m_deviceDescriptor[] =
      {
          0x12,             // bLength
          DEVICE,           // bDescriptorType
          0x02, 0x00,       // bcdUSB == Version 2.00
          0x02,             // bDeviceClass == Communication Device Class
          0x00,             // bDeviceSubClass
          0x00,             // bDeviceProtocol
          MAX_PACKET_SIZE,  // bMaxPacketSize
          0xEF, 0xBE,       // Vendor ID
          0xAD, 0xDE,       // Product ID
          0x00, 0x00,       // bcdDevice == Version 1.00
          0x00,             // iManifacturer == Index to string manufacturer descriptor
          0x00,             // iProduct == Index to string product descriptor
          0x00,             // iSerialNumber == Index to string serial number
          0x01              // bNumConfigurations
      };

      const u8 m_stringDescriptorLanguageID[] =
      {
          0x04,             // bLength
          STRING,           // bDescriptorType
          0x09, 0x04        // wLANGID0 == English - United States
      };

      const u8 m_stringDescriptorManufacturer[] =
      {
          0x0A,             // bLength
          STRING,           // bDescriptorType
          'A', 0x00,
          'n', 0x00,
          'k', 0x00,
          'i', 0x00
      };

      const u8 m_stringDescriptorProduct[] =
      {
          0x0C,             // bLength
          STRING,           // bDescriptorType
          'C', 0x00,
          'o', 0x00,
          'z', 0x00,
          'm', 0x00,
          'o', 0x00
      };

      // TODO: Maybe get this from flash?
      const u8 m_stringDescriptorSerialNumber[] =
      {
          0x10,             // bLength
          STRING,           // bDescriptorType
          'A', 0x00,
          'B', 0x00,
          'C', 0x00,
          'D', 0x00,
          'E', 0x00,
          'F', 0x00,
          '0', 0x00
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
          0x09,             // bLength
          CONFIGURATION,    // bDescriptorType
          0x43, 0x00,       // wTotalLength
          0x02,             // bNumInterfaces
          0x01,             // bConfigurationValue
          0x00,             // iConfiguration == Index to string descriptor
          0xC0,             // bmAttributes == bit[7]: Compatability with USB 1.0
                            //                 bit[6]: 1 if Self-Powered, 0 if Bus-Powered
                            //                 bit[5]: Remote Wakeup
                            //                 bits[4:0]: Reserved
          0x00,             // bMaxPower == 0 mA

          // Interface Descriptor
          0x09,             // bLength
          INTERFACE,        // bDescriptorType
          0x00,             // bInterfaceNumber
          0x00,             // bAlternateSetting
          0x01,             // bNumEndpoints
          0x02,             // bInterfaceClass == Communication Interface Class
          0x02,             // bInterfaceSubClass == Abstract Control Model
          0x01,             // bInterfaceProtocol == Common AT Commands
          0x00,             // iInterface == Index to string descriptor

          // Header Functional Descriptor
          0x05,             // bLength
          0x24,             // bDescriptorType == CS_INTERFACE
          0x00,             // bDescriptorSubType == Header Functional Descriptor
          0x10, 0x01,       // bcdCDC

          // Call Management Functional Descriptor
          0x05,             // bLength
          0x24,             // bDescriptorType == CS_INTERFACE
          0x01,             // bDescriptorSubType == Call Management Functional Descriptor
          0x00,             // bmCapabilities
          0x01,             // bDataInterface

          // Abstract Control Management Functional Descriptor
          0x04,             // bLength
          0x24,             // bDescriptorType == CS_INTERFACE
          0x02,             // bDescriptorSubType == Abstract Control Management Functional Descriptor
          0x02,             // bmCapabilities ==  Device can send/receive call management information over a Data Class Interface

          // Union Functional Descriptor
          0x05,             // bLength
          0x24,             // bDescriptorType == CS_INTERFACE
          0x06,             // bDescriptorSubType == Union Functional Descriptor
          0x00,             // bMasterInterface
          0x01,             // bSlaveInterface0

          // Endpoint 2 Descriptor
          0x07,             // bLength
          ENDPOINT,         // bDescriptorType
          0x82,             // bEndpointAddress == IN[2]
          0x03,             // bmAttributes
          MAX_PACKET_SIZE,  // wMaxPacketSize
          0x00,
          0xFF,             // bInterval

          // Data Class Interface Descriptor
          0x09,             // bLength
          INTERFACE,        // bDescriptorType
          0x01,             // bInterfaceNumber
          0x00,             // bAlternateSetting
          0x02,             // bNumEndpoints
          0x0A,             // bInterfaceClass == CDC
          0x00,             // bInterfaceSubClass
          0x00,             // bInterfaceProtocol
          0x00,             // iInterface

          // Endpoint 3 Descriptor
          0x07,             // bLength
          ENDPOINT,         // bDescriptorType
          0x03,             // bEndpointAddress == OUT[3]
          0x02,             // bmAttributes == Bulk
          MAX_PACKET_SIZE,  // wMaxPacketSize
          0x00,
          0x00,             // bInterval == Ignore for Bulk transfer

          // Endpoint 1 Descriptor
          0x07,             // bLength
          ENDPOINT,         // bDescriptorType
          0x81,             // bEndpointAddress == IN[1]
          0x02,             // bmAttributes == Bulk
          0x00,             // bInterval == Ignore for Bulk transfer
      };

      const u8 m_deviceQualifier[] =
      {
          0x0A,             // bLength
          DEVICE_QUALIFIER, // bDescriptorType
          0x00, 0x02,       // bcdUSB
          0x00,             // bDeviceClass
          0x00,             // bDeviceSubClass
          0x00,             // bDeviceProtocol
          MAX_PACKET_SIZE,  // bMaxPacketSize0
          0x01,             // bNumConfigurations
          0x00              // bReserved
      };

      static void HandleSetupMessage();
      static bool Setup(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool StandardRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool ClassRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static bool VendorRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength);
      static void SetupTransfer(u32 index, u32 addressOUT, u32 lengthOUT, u32 addressIN, u32 lengthIN);

      const endpoint_desc_t __attribute__((aligned(64))) usb_control_out_endpoint =
      {
          sizeof(endpoint_desc_t),
          ENDPOINT,
          0x00,  // OUT
          (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
          MAX_PACKET_SIZE,
          0x01,
      };

      const endpoint_desc_t __attribute__((aligned(64))) usb_control_in_endpoint =
      {
          sizeof(endpoint_desc_t),
          ENDPOINT,
          0x80,  // IN
          (EP_TT_CONTROL | EP_ST_NO_SYNC | EP_UT_DATA),
          MAX_PACKET_SIZE,
          0x01,
      };

      static void EPStart(const endpoint_desc_t* ep)
      {
        unsigned int index, cap, type, ctrl;
        bool isIN = false;

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
        cap = 0x20000000;
        // Add Mult (number of packets executed per transaction)
        if (type == EP_TT_ISOCHRONOUS)
          cap |= ((ep->wMaxPacketSize >> 11) + 1) << 30;

        // Add maximum packet length
        cap |= (ep->wMaxPacketSize & 0x7FF) << 16;

        // Add IOS (Interrupt On Setup)
        if (type == EP_TT_CONTROL)
          cap |= 0x8000;

        volatile unsigned int* dQH = m_dQH[index];
        volatile unsigned int* dTD = m_dTD[index];

        // Write capabilities
        dQH[DQH_CAPABILITIES] = cap;
        // Write current dTD pointer
        dQH[DQH_CURRENT_DTD_POINTER] = 0;
        // Write next dTD pointer
        dQH[DQH_NEXT_DTD_POINTER] = 1;  // Next dTD pointer terminate
        dQH[DQH_TOTAL_BYTES] = 0;
        dQH[DQH_BUFFER_POINTER_0] = 0;
        dQH[DQH_BUFFER_POINTER_1] = 0;
        dQH[DQH_BUFFER_POINTER_2] = 0;
        dQH[DQH_BUFFER_POINTER_3] = 0;
        dQH[DQH_BUFFER_POINTER_4] = 0;

        // Clear transfer descriptor
        dTD[0] = 1;
        dTD[1] = 0;

        // Set the endpoint control
        cap |= type << 2;
        if (isIN)
        {
          // TX Endpoint Reset and Enable
          REG_WORD(ctrl) |= (type << 18) | (1 << 22) | (1 << 23);
          //REG_HALF(ctrl + 2) = (cap | 0x40);
          //REG_HALF(ctrl + 2) = (cap | 0x80);
        } else {
          // RX Endpoint Reset and Enable
          REG_WORD(ctrl) |= (type << 2) | (1 << 6) | (1 << 7);
          //REG_HALF(ctrl) = (cap | 0x40);
          //REG_HALF(ctrl) = (cap | 0x80);
 
        }

        cap |= type << 2;
     }

      static void InitDeviceController()
      {
        // Clear run bit
        REG_WORD(REG_USBCMD) = 0;
        // Wait for bit to clear
        while (REG_WORD(REG_USBCMD) & USB_USBCMD_RS)
          ;
        // Set reset bit and reinitialize hardware
        REG_WORD(REG_USBCMD) |= USB_USBCMD_RST;
        // Wait for bit to clear
        while (REG_WORD(REG_USBCMD) & USB_USBCMD_RST)
          ;
        // Set USB address to 0
        REG_WORD(REG_DEVICE_ADDR) = 0;
        // Set the maximum burst length (in 32-bit words)
        REG_WORD(REG_BURSTSIZE) = 0x1010;
        REG_WORD(REG_SBUSCFG) = 0;
        // Set the USB controller to DEVICE mode and disable "setup lockout mode"
        REG_WORD(REG_USBMODE) = (USBMODE_DEVICE_MODE | USB_USBMODE_SLOM);
        
        // Clear the setup status
        REG_WORD(REG_USBSTS) = 0;

        // Setup control endpoints
        EPStart(&usb_control_out_endpoint);
        EPStart(&usb_control_in_endpoint);

        // Configure the endpoint-list address
        REG_WORD(REG_ENDPOINTLISTADDR) = (u32)&m_dQH;

        // Set the run bit
        REG_WORD(REG_USBCMD) = USB_USBCMD_RS;
        // Dual poll for SRI to sync with host
        while (!(REG_WORD(REG_USBSTS) & USB_USBSTS_SRI))
          ;
        // Clear SOF
        REG_WORD(REG_USBSTS) = USB_USBSTS_SRI;
      }

      static void Reset()
      {
        // Clear all setup token semaphores
        REG_WORD(REG_ENDPTSETUPSTAT) = REG_WORD(REG_ENDPTSETUPSTAT);
        // Clear all the endpoint complete status bits
        REG_WORD(REG_ENDPTCOMPLETE) = REG_WORD(REG_ENDPTCOMPLETE);
        // Cancel all primed status
        while (REG_WORD(REG_ENDPTPRIME))
          ;
        REG_WORD(REG_ENDPTFLUSH) = 0xFFFFffff;

        // TODO:
        // Read the reset bit in the PORTSCx register and make sure it is still
        // active. A USB reset will occur for a minimum of 3 ms and the DCD
        // must reach this point in the reset cleanup before the end of a reset
        // occurs, otherwise a hardware reset of the device controller is
        // recommended (rare).

        //printf("port: %08X\n", REG_WORD(REG_PORTSC1));

        //InitDeviceController();

        REG_WORD(REG_DEVICE_ADDR) = 0;
      }

      void USBInit()
      {
       // USB calibration (from Movidius)
        REG_WORD(CPR_USB_CTRL_ADR) = 0x00003004;
        REG_WORD(CPR_USB_PHY_CTRL_ADR) = 0x01C18d07;

        // Initialize the hardware
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_USB);
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_USB);

        // Set OTG transceiver configuration
        REG_WORD(REG_OTGSC) |=  (USB_OTGSC_OT | USB_OTGSC_VC);
        // Transceiver enable
        REG_WORD(REG_PORTSC1) = (USB_PORTSCX_PFSC | USB_PORTSCX_PE);

        InitDeviceController();

        // XXX-MA: Not including the setup for interrupts...
        //REG_WORD(REG_USBINTR) |= USB_USBINTR_UE;

        // TODO: Add IN-addresses?
      //    m_dTD_IN[0] = (unsigned int)&m_dTD_IN[8];
      //    m_dTD_IN[8] = (unsigned int)&m_dTD_IN[16];
      //    m_dTD_IN[16] = (unsigned int)&m_dTD_IN[24];
      //    m_dTD_IN[24] = (unsigned int)&m_dTD_IN[0];

        m_deviceStatus = (m_deviceStatus & 0xFFFF) | (REG_WORD(REG_FRINDEX) << 16);
      }

      void USBSend(u8* buffer, u32 length)
      {
        // TODO: Implement
      }

      void USBUpdate()
      {
        static u32 oldStatus = 0;
        u32 temp = REG_WORD(REG_USBSTS) &~ (USB_USBSTS_SLI | USB_USBSTS_SRI);
        if (temp != oldStatus)
        {
          oldStatus = temp;
          printf("stat: %08X\n", oldStatus);
        }

        // Check for USB Reset
        if (REG_WORD(REG_USBSTS) & USB_USBSTS_URI)
        {
          printf("RESET\n");
          // Clear USB Reset Received
          REG_WORD(REG_USBSTS) = USB_USBSTS_URI;
          Reset();
        }
        // Check for Port Change
        if (REG_WORD(REG_USBSTS) & USB_USBSTS_PCI)
        {
          printf("PORT CHANGE\n");
          // TODO: Anything else required here?
          REG_WORD(REG_USBSTS) = USB_USBSTS_PCI;
        }
        // Check for suspended state
        if (REG_WORD(REG_USBSTS) & USB_USBSTS_SLI)
        {
          printf("SLI\n");
          REG_WORD(REG_USBSTS) = USB_USBSTS_SLI;
        }
        // Check for SOF (Start Of Frame)
        if (REG_WORD(REG_USBSTS) & USB_USBSTS_SRI)
        {
          // Clear SOF
          REG_WORD(REG_USBSTS) = USB_USBSTS_SRI;

          // Increment the clock counter
          m_deviceClock++;

          HandleSetupMessage();

          // Update the frame index
          m_deviceStatus = (m_deviceStatus & 0xFFFF) | (REG_WORD(REG_FRINDEX) << 16);
        }

        // Check for USB Interrupt
        if (REG_WORD(REG_USBSTS) & USB_USBSTS_UI)
        {
          printf("USB-I\n");
          REG_WORD(REG_USBSTS) = USB_USBSTS_UI;
          // Handle USB control messages
          HandleSetupMessage();
        }

        // TODO: Implement the rest
        // ...
      }

      bool isFull()
      {
        return true;
      }

      static void HandleSetupMessage()
      {
        u32 i;
        u32 setupStatus, index, ctrl;
        bool setupStall;

        static u32 s_setupStatus = 0, s_usbmode = 0, s_usbsts = 0, s_deviceStatus = 0;
        setupStatus = REG_WORD(REG_ENDPTSETUPSTAT);

        if (s_setupStatus != setupStatus || s_usbmode != REG_WORD(REG_USBMODE) || s_usbsts != REG_WORD(REG_USBSTS) || s_deviceStatus != m_deviceStatus)
        {
          s_setupStatus = setupStatus;
          s_usbmode = REG_WORD(REG_USBMODE);
          s_usbsts = REG_WORD(REG_USBSTS);
          s_deviceStatus = m_deviceStatus;
          printf("%X, %X, %X, %X, %08X, %08X\n", s_setupStatus, s_usbmode, s_usbsts, s_deviceStatus, 
              m_dQH[0][10], REG_WORD(REG_ENDPTCOMPLETE));
        }

        if (setupStatus)
        {
          printf("ENDPTSETUPSTAT: %08X\n", setupStatus);
        }

        for (i = 0; i < 16; i++)
        {
          u32 bit = 1 << i;
          if (!(setupStatus & bit))
          {
            continue;
          }

          printf("found %i\n", i);

          // Clear the setup status
          REG_WORD(REG_ENDPTSETUPSTAT) = bit;

          // Get the queue head index offset (two per endpoint)
          index = i << 1;

          // Setup tripwire
          REG_WORD(REG_USBCMD) |= USB_USBCMD_SUTW;
          while (!(REG_WORD(REG_USBCMD) & USB_USBCMD_SUTW))
          {
            REG_WORD(REG_USBCMD) |= USB_USBCMD_SUTW;
          }

          // Read out data from the setup buffer
          unsigned int data03 = (unsigned int)m_dQH[DQH_SETUP_BUFFER_BYTES_3_0];
          unsigned int data47 = (unsigned int)m_dQH[DQH_SETUP_BUFFER_BYTES_7_4];
          unsigned int bmRequestType = data03 & 0xFF;
          unsigned int bRequest = (data03 >> 8) & 0xFF;
          unsigned int wValue = (data03 >> 16) & 0xFFFF;
          unsigned int wIndex = data47 & 0xFFFF;
          unsigned int wLength = (data47 >> 16) & 0xFFFF;
          setupStall = Setup(index, bmRequestType, bRequest, wValue, wIndex, wLength);

          // Clear tripwire
          REG_WORD(REG_USBCMD) &= ~USB_USBCMD_SUTW;

          if (setupStall)
          {
             //EPStall(index);
          } else {
            //if (m_dQH[index + 0][12]) EPSetup(index + 0);
            //if (m_dQH[index + 1][12]) EPSetup(index + 1);
          }
        }
      }

      static bool Setup(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = true;

        switch ((bmRequestType >> RT_TYPE_SHIFT) & RT_TYPE_MASK)
        {
          case RT_TYPE_STANDARD:
            printf("STANDARD\n");
            setupStall = StandardRequestHandler(index, bmRequestType, bRequest, wValue, wIndex, wLength);
            break;

          case RT_TYPE_CLASS:
            printf("CLASS\n");
            setupStall = ClassRequestHandler(index, bmRequestType, bRequest, wValue, wIndex, wLength);
            break;

          case RT_TYPE_VENDOR:
            printf("VENDOR\n");
            setupStall = VendorRequestHandler(index, bmRequestType, bRequest, wValue, wIndex, wLength);
            break;

          default:
            printf("[ERROR] Unknown USB bmRequestType\n");
            break;
        }

        printf("bRequest = %02X\n", bRequest);

        return setupStall;
      }

      static bool StandardRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = false;

        switch (bRequest)
        {
          case GET_STATUS:
          {
            if (wLength > sizeof(m_USBStatus))
              wLength = sizeof(m_USBStatus);
            SetupTransfer(index, (u32)m_USBStatus, 0, (u32)m_USBStatus, wLength);
            break;
          }

          case SET_ADDRESS:
          {
            REG_WORD(REG_DEVICE_ADDR) = ((wValue << 1) + 1) << 24;
            m_deviceStatus = (m_deviceStatus & 0xFFFF00FF) | (wValue << 8);
            // Unknown ... Following Movidius. Pointer to byte[1]...
            SetupTransfer(index, 0, 0, (u32)&m_deviceStatus + 1, 0);
            break;
          }

          case GET_DESCRIPTOR:
          {
            switch (wValue >> 8)
            {
              case DEVICE:
              {
                if (wLength > sizeof(m_deviceDescriptor))
                  wLength = sizeof(m_deviceDescriptor);
                SetupTransfer(index, (u32)m_deviceDescriptor, 0, (u32)m_deviceDescriptor, wLength);
                break;
              }

              case CONFIGURATION:
              {
                if (wLength > sizeof(m_configurationDescriptor))
                  wLength = sizeof(m_configurationDescriptor);
                SetupTransfer(index, (u32)m_configurationDescriptor, 0, (u32)m_configurationDescriptor, wLength);
                break;
              }

              case DEVICE_QUALIFIER:
              {
                if (wLength > sizeof(m_deviceQualifier))
                  wLength = sizeof(m_deviceQualifier);
                SetupTransfer(index, (u32)m_deviceQualifier, 0, (u32)m_deviceQualifier, wLength);
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
            SetupTransfer(index, (u32)&m_deviceStatus, 0, (u32)&m_deviceStatus, wLength);
            break;
          }

          case SET_CONFIGURATION:
          {
            if (wValue >= 2)  // TODO: Magic Movidius number...
            {
              setupStall = true;
            } else {
              m_deviceStatus = (m_deviceStatus &~ 0xFF) | wValue;
              SetupTransfer(index, 0, 0, (u32)&m_deviceStatus, 0);
            }
            break;
          }

          case GET_INTERFACE:
          {
            if (wLength > sizeof(m_interface))
              wLength = m_interface;
            SetupTransfer(index, (u32)&m_interface, 0, (u32)&m_interface, wLength);
            break;
          }

          case SET_INTERFACE:
          {
            m_interface = wIndex;
            SetupTransfer(index, 0, 0, (u32)&m_interface, 0);
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

      static bool ClassRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = false;

        switch (bRequest)
        {
          case 0x20:  // SET_LINE_CODING
          {
            SetupTransfer(index, (u32)m_lineCoding, sizeof(m_lineCoding), (u32)m_lineCoding, 0);
            break;
          }

          case 0x21:  // GET_LINE_CODING
          {
            SetupTransfer(index, (u32)m_lineCoding, 0, (u32)m_lineCoding, sizeof(m_lineCoding));
            break;
          }

          case 0x22:  // SET_CONTROL_LINE_STATE
          {
            // TODO: Implement
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

      static bool VendorRequestHandler(u32 index, u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = true;
        return setupStall;
      }

      static void SetupTransfer(u32 index, u32 addressOUT, u32 lengthOUT, u32 addressIN, u32 lengthIN)
      {
        m_dQH[index + 0][DQH_CUSTOM_BUFFER] = addressOUT;
        m_dQH[index + 0][DQH_CUSTOM_LENGTH] = lengthOUT;
        m_dQH[index + 1][DQH_CUSTOM_BUFFER] = addressIN;
        m_dQH[index + 1][DQH_CUSTOM_LENGTH] = lengthIN;
      }
    }
  }
}

