#include "anki/cozmo/robot/hal.h"
#include "movidius.h"
#include "usbDefines.h"

#ifdef USE_USB

#define USB_WORD(x) *(volatile u32*)(USB_BASE_ADDR + (x))

#define CACHE_FLUSH asm("flush")

#define ALIGNED(x) __attribute__((aligned((x))))
#define USB_INDEX(x) ((((x) & 0x0f) << 1) + (((x) >> 7) & 1))
#define ENDPTCTRL(x) (REG_ENDPTCTRL0 + ((x) << 2))

#define DTD_TERMINATE                 1
#define DTD_TOKEN_LENGTH(x)           ((x) << 16)
#define DTD_STATUS_ACTIVE             (1 << 7)
#define DTD_STATUS_HALTED             (1 << 6)
#define DTD_STATUS_DATA_BUFFER_ERROR  (1 << 5)
#define DTD_STATUS_TRANSACTION_ERROR  (1 << 3)

#define ENDPOINTCTRL_TXE              (1 << 23)
#define ENDPOINTCTRL_TXR              (1 << 22)
#define ENDPOINTCTRL_TXT_SHIFT        (18)
#define ENDPOINTCTRL_TXS              (1 << 16)
#define ENDPOINTCTRL_RXE              (1 << 7)
#define ENDPOINTCTRL_RXR              (1 << 6)
#define ENDPOINTCTRL_RXT_SHIFT        (2)
#define ENDPOINTCTRL_RXS              (1)

// Request Type Definitions
#define RT_DIRECTION_SHIFT            7
#define RT_DIRECTION_MASK             1
#define RT_DIRECTION_HOST_TO_DEVICE   0
#define RT_DIRECTION_DEVICE_TO_HOST   1

#define RT_TYPE_SHIFT                 5
#define RT_TYPE_MASK                  3
#define RT_TYPE_STANDARD              0
#define RT_TYPE_CLASS                 1
#define RT_TYPE_VENDOR                2
#define RT_TYPE_RESERVED              3

#define RT_RECIPIENT_SHIFT            0
#define RT_RECIPIENT_MASK             0x1f
#define RT_RECIPIENT_DEVICE           0
#define RT_RECIPIENT_INTERFACE        1
#define RT_RECIPIENT_ENDPOINT         2
#define RT_RECIPIENT_OTHER            3

// We can safely transmit 64 bytes per packet
#define MAX_PACKET_SIZE               64
#define MAX_PACKET_HI                 ((MAX_PACKET_SIZE >> 8) & 0xFF)
#define MAX_PACKET_LO                 (MAX_PACKET_SIZE & 0xFF)
#define MAX_PACKET_EP0                0x40

#define S4(a, b, c, d)                (d), (c), (b), (a)
#define S3(a, b, c)                   S4(a, b, c, 0)
#define S2(a, b)                      S3(a, b, 0)
#define S1(a)                         S2(a, 0)

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      struct USBQueueHead
      {
        u32 capabilities;
u32 currentDTDPointer;
        u32 nextDTDPointer;
        u32 token;
        u32 bufferPointer[5];
        u32 reserved0;
        u32 setupBuffer0;
        u32 setupBuffer1;
        u32 reserved1[4];
      };

      struct USBTransferDescriptor
      {
        u32 nextDTDPointer;
        u32 token;
        u32 bufferPointer[5];
        u32 reserved0;
      };

      struct USBEndpoint
      {
        u32 address;
        u32 type;
        u32 maxPacketSize;
        volatile USBTransferDescriptor* descriptor;
      };

      // Self-Powered, No Remote Wakeup
      static u8 ALIGNED(4) m_USBStatus[] = { S2(0x01, 0x00) };
      static u32 ALIGNED(4) m_configuration = 0;
      static u32 m_interface = 0;
      static volatile USBQueueHead ALIGNED(2048)  m_dQH[32];

      static u32 m_wasBulkPrimed = 0;
      static u32 m_bulkOutHead = 0;
      static u32 m_bulkOutTail = 0;
      static u32 m_bulkOutLength = 0;

      static u32 m_bulkInHead = 0;
      static u32 m_bulkInTail = 0;

      static u8 ALIGNED(4096) __attribute__((section(".ddr.bss"))) m_receiveBuffer[16 * 1024];
      static u8 ALIGNED(4096) __attribute__((section(".ddr.bss"))) m_transmitBuffer[16 * 1024];

      volatile USBTransferDescriptor m_descriptor_ep0_out;
      volatile USBTransferDescriptor m_descriptor_ep0_in;
      volatile USBTransferDescriptor m_descriptor_ep1_out;
      volatile USBTransferDescriptor m_descriptor_ep2_in;
      volatile USBTransferDescriptor m_descriptor_ep3_in;

      // Control endpoints
      static USBEndpoint m_ep0_out =
      {
        0x00,
        EP_TT_CONTROL,
        MAX_PACKET_EP0,
        &m_descriptor_ep0_out
      };

      static USBEndpoint m_ep0_in =
      {
        0x80,
        EP_TT_CONTROL,
        MAX_PACKET_EP0,
        &m_descriptor_ep0_in
      };

      // Serial endpoints
      static USBEndpoint m_ep1_out =
      {
        0x01,
        EP_TT_BULK,
        MAX_PACKET_SIZE,
        &m_descriptor_ep1_out
      };

      static USBEndpoint m_ep2_in =
      {
        0x82,
        EP_TT_BULK,
        MAX_PACKET_SIZE,
        &m_descriptor_ep2_in
      };

      static USBEndpoint m_ep3_in =
      {
        0x83,
        EP_TT_INTERRUPT,
        MAX_PACKET_EP0,
        &m_descriptor_ep3_in
      };

      // Descriptors and data
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

      // TODO: Make a constant serial for all robots
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
      static void HandleTransfers();
      static void HandleSetupMessage();
      static bool Setup(u32 bmRequestType, u32 bRequest, u32 wValue,
          u32 wIndex, u32 wLength);
      static bool StandardRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength);
      static bool ClassRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength);
      static bool VendorRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength);

      static void StartEndpoint(USBEndpoint* ep)
      {
        u32 index, cap, ctrl;

        index = USB_INDEX(ep->address);
        ctrl = ENDPTCTRL(ep->address & 0x0f);

        // Add Zero Length Termination select
        cap = 0;
        //cap |= 0x20000000;

        // Add maximum packet length
        cap |= (ep->maxPacketSize & 0x7ff) << 16;

        // Add IOS (Interrupt On Setup)
        //if (type == EP_TT_CONTROL)
        //  cap |= 0x8000;

        volatile USBQueueHead* dQH = &m_dQH[index];
        volatile USBTransferDescriptor* dTD = ep->descriptor;

        // Write capabilities
        dQH->capabilities = cap;
        // Write current dTD pointer
        dQH->currentDTDPointer = 0;
        // Write next dTD pointer
        dQH->nextDTDPointer = DTD_TERMINATE;
        dQH->token = 0;
        dQH->bufferPointer[0] = 0;
        dQH->bufferPointer[1] = 0;
        dQH->bufferPointer[2] = 0;
        dQH->bufferPointer[3] = 0;
        dQH->bufferPointer[4] = 0;

        // Clear the transfer descriptor
        dTD->nextDTDPointer = DTD_TERMINATE;
        dTD->token = 0;

        // Set the endpoint control
        if (ep->address & 0x80)  // IN-endpoint?
        {
          // TX Endpoint Reset and Enable
          USB_WORD(ctrl) =
            (ep->type << ENDPOINTCTRL_TXT_SHIFT) |
            ENDPOINTCTRL_TXR |
            ENDPOINTCTRL_TXE;
        } else {
          // RX Endpoint Reset and Enable
          USB_WORD(ctrl) =
            (ep->type << ENDPOINTCTRL_RXT_SHIFT) |
            ENDPOINTCTRL_RXR |
            ENDPOINTCTRL_RXE;
        }
      }

      static void PrimeEndpoint(USBEndpoint* ep, const void* address,
          u32 bufferLength, u32 offset, u32 transferLength)
      {
        u32 index = USB_INDEX(ep->address);

        volatile USBQueueHead* dQH = &m_dQH[index];
        volatile USBTransferDescriptor* dTD = ep->descriptor;

        // Write next dTD pointer as invalid
        dTD->nextDTDPointer = DTD_TERMINATE;
        // Write the active status bit and packet length
        dTD->token = DTD_STATUS_ACTIVE | DTD_TOKEN_LENGTH(transferLength);
        // Write buffer pointers in 4KB chunks
        u32 bufferEnd = (u32)address + bufferLength;
        dTD->bufferPointer[0] = (u32)address + offset;

        u32 addressAligned = ((u32)address + offset) & 0xFFFFf000;
        u32 nextAddress = addressAligned;
        for (int i = 1; i <= 4; i++)
        {
          // Increment 4KB and wrap around if it goes past the buffer
          nextAddress += 0x1000;
          if (nextAddress > bufferEnd)
          {
            nextAddress = (u32)address;
          }

          dTD->bufferPointer[i] = nextAddress;
        }

        // Write dQH next pointer
        dQH->nextDTDPointer = (u32)dTD;
        // Clear the active and halted status bits
        CACHE_FLUSH;

        dQH->token &= ~(DTD_STATUS_ACTIVE | DTD_STATUS_HALTED);

        // Prime the endpoint
        u32 prime = 1 << (index >> 1);
        if (ep->address & 0x80)  // IN endpoint?
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

         // Start the serial/cdcacm endpoints
        StartEndpoint(&m_ep1_out);
        StartEndpoint(&m_ep2_in);
        StartEndpoint(&m_ep3_in);

        // Prime the OUT endpoint
        PrimeEndpoint(&m_ep1_out,
            m_receiveBuffer, sizeof(m_receiveBuffer),
            0, MAX_PACKET_SIZE);
        m_bulkOutLength = MAX_PACKET_SIZE;

        // Prime the IN endpoints
        PrimeEndpoint(&m_ep2_in, NULL, 0, 0, 0);
        PrimeEndpoint(&m_ep3_in, NULL, 0, 0, 0);

        m_wasBulkPrimed = 1;  // TODO: Boolean when MYRIAD2 exists?
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
        // Transceiver enable (can force full-speed connect here)
        USB_WORD(REG_PORTSC1) = USB_PORTSCX_PE;
        // Set USB address to 0
        USB_WORD(REG_DEVICE_ADDR) = 0;
        // Set the maximum burst length (in 32-bit words)
        USB_WORD(REG_BURSTSIZE) = 0x1010;
        USB_WORD(REG_SBUSCFG) = 0;
        // Set the USB controller to DEVICE mode and disable "setup lockout mode"
        USB_WORD(REG_USBMODE) = (USBMODE_DEVICE_MODE | USB_USBMODE_SLOM);
        
        // Configure the endpoint-list address
        USB_WORD(REG_ENDPOINTLISTADDR) = (u32)&m_dQH;

        // Start control endpoints
        StartEndpoint(&m_ep0_out);
        StartEndpoint(&m_ep0_in);

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

        // AKA do a hardware reset for the peripheral

        //printf("port: %08X\n", USB_WORD(REG_PORTSC1));

        USB_WORD(REG_DEVICE_ADDR) = 0;
      }

      static void HandleTransfers()
      {
        // Check the bulk OUT transfer descriptor for data
        u32 length;
        if (!m_wasBulkPrimed)
        {
          return;
        }

        CACHE_FLUSH;

        volatile USBTransferDescriptor* dTD = m_ep1_out.descriptor;
        if (!(dTD->token & DTD_STATUS_ACTIVE))
        {
          u32 dataLeft = (dTD->token >> 16) & 0x7fff;
          length = m_bulkOutLength - dataLeft;
          m_bulkOutHead = (m_bulkOutHead + length) % sizeof(m_receiveBuffer);

          // Reprime the endpoint, but don't let it overflow the buffer. Allow
          // enough data to go up to the tail pointer.
          if (m_bulkOutTail > m_bulkOutHead)
          {
            length = m_bulkOutTail - m_bulkOutHead - 1;
          } else {
            // Receive until the end of the buffer
            length = sizeof(m_receiveBuffer) - m_bulkOutHead;
          }

          m_bulkOutLength = length;

          if (length)
          {
            PrimeEndpoint(&m_ep1_out,
                m_receiveBuffer, sizeof(m_receiveBuffer),
                m_bulkOutHead, length);
          }
        }

        dTD = m_ep2_in.descriptor;
        if (!(dTD->token & DTD_STATUS_ACTIVE))
        {
          if (m_bulkInTail > m_bulkInHead)
          {
            length = sizeof(m_transmitBuffer) - m_bulkInTail;
          } else {
            length = m_bulkInHead - m_bulkInTail;
          }

          if (length)
          {
            PrimeEndpoint(&m_ep2_in,
                m_transmitBuffer, sizeof(m_transmitBuffer),
                m_bulkInTail, length);
          }

          // TODO: Do this after transfer complete
          m_bulkInTail = (m_bulkInTail + length) % sizeof(m_transmitBuffer);
        }
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

        printf("reg = %08X\n", USB_WORD(REG_OTGSC));
      }

      void USBUpdate()
      {
        u32 status = USB_WORD(REG_USBSTS);
        // Check for USB Reset
        if (status & USB_USBSTS_URI)
        {
          // Clear USB Reset Received
          USB_WORD(REG_USBSTS) = USB_USBSTS_URI;
          Reset();
        }
        // Check for Port Change
        if (status & USB_USBSTS_PCI)
        {
          // TODO: Anything else required here?
          //USB_WORD(REG_USBSTS) = USB_USBSTS_PCI;
        }
        // Check for suspended state
        if (status & USB_USBSTS_SLI)
        {
          //USB_WORD(REG_USBSTS) = USB_USBSTS_SLI;
        }
        // Check for SOF (Start Of Frame)
        if (status & USB_USBSTS_SRI)
        {
          // Clear SOF
          USB_WORD(REG_USBSTS) = USB_USBSTS_SRI;
          //while (USB_WORD(REG_USBSTS) & USB_USBSTS_SRI)
          //  ;

          //SET_USB_WORD(ICB_INT_CLEAR, (1 << IRQ_USB));

        }

        // Check for USB Interrupt
        if (status & USB_USBSTS_UI)
        {
          //USB_WORD(REG_USBSTS) = USB_USBSTS_UI;
          // Handle USB control messages
          //HandleSetupMessage();
        }

        HandleSetupMessage();
        HandleTransfers();
      }

      static void HandleSetupMessage()
      {
        bool setupStall;

        // Only accept setup on endpoint 0
        if (USB_WORD(REG_ENDPTSETUPSTAT) & 1)
        {
          // Clear the setup status
          USB_WORD(REG_ENDPTSETUPSTAT) = 1;

          // Setup tripwire for reading the setup bytes
          do
          {
            USB_WORD(REG_USBCMD) |= USB_USBCMD_SUTW;
          } while (!(USB_WORD(REG_USBCMD) & USB_USBCMD_SUTW));

          // Read out data from the setup buffer
          CACHE_FLUSH;

          u32 data03 = m_dQH[0].setupBuffer0;
          u32 data47 = m_dQH[0].setupBuffer1;
          u32 bmRequestType = data03 & 0xFF;
          u32 bRequest = (data03 >> 8) & 0xFF;
          u32 wValue = (data03 >> 16) & 0xFFFF;
          u32 wIndex = data47 & 0xFFFF;
          u32 wLength = (data47 >> 16) & 0xFFFF;

          printf("%02X %02X %04X %04X %04X\n", 
              bmRequestType, bRequest, wValue, wIndex, wLength);

          setupStall = Setup(bmRequestType, bRequest, wValue, wIndex, wLength);

          // Clear tripwire
          USB_WORD(REG_USBCMD) &= ~USB_USBCMD_SUTW;

          if (setupStall)
          {
            // Stall the endpoint
            USB_WORD(REG_ENDPTCTRL0) =
              ENDPOINTCTRL_TXS |
              ENDPOINTCTRL_RXS;
          }
        }
      }

      static bool Setup(u32 bmRequestType, u32 bRequest, u32 wValue, u32 wIndex,
          u32 wLength)
      {
        bool setupStall = true;

        switch ((bmRequestType >> RT_TYPE_SHIFT) & RT_TYPE_MASK)
        {
          case RT_TYPE_STANDARD:
            setupStall = StandardRequestHandler(bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          case RT_TYPE_CLASS:
            setupStall = ClassRequestHandler(bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          case RT_TYPE_VENDOR:
            setupStall = VendorRequestHandler(bmRequestType, bRequest,
                wValue, wIndex, wLength);
            break;

          default:
            // TODO: Assert?
            break;
        }

        return setupStall;
      }

      static bool StandardRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength)
      {
        bool setupStall = false;

        switch (bRequest)
        {
          case GET_STATUS:
          {
            // XXX: FIX FOR MYRIAD1 BSWAP
            if (wLength > sizeof(m_USBStatus) - 2)
              wLength = sizeof(m_USBStatus) - 2;
            PrimeEndpoint(&m_ep0_in, 
                &m_USBStatus, sizeof(m_USBStatus),
                0, wLength);
            //PrimeEndpoint(&m_ep0_out, NULL, 0);
            break;
          }

          case SET_ADDRESS:
          {
            // Device address is the high 7 bits. The +1 is from Movidius...
            USB_WORD(REG_DEVICE_ADDR) = ((wValue << 1) + 1) << 24;
            // Acknowledge the packet
            PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
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
                PrimeEndpoint(&m_ep0_in,
                    m_deviceDescriptor, sizeof(m_deviceDescriptor),
                    0, wLength);
                PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
                break;
              }

              case CONFIGURATION:
              {
                // XXX: FIX FOR MYRIAD1 BSWAP
                if (wLength > (sizeof(m_configurationDescriptor) - 1))  
                  wLength = sizeof(m_configurationDescriptor) - 1;
                PrimeEndpoint(&m_ep0_in,
                    m_configurationDescriptor,
                    sizeof(m_configurationDescriptor),
                    0, wLength);
                PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
                break;
              }

              case DEVICE_QUALIFIER:
              {
                // XXX: FIX FOR MYRIAD1 BSWAP
                if (wLength > (sizeof(m_deviceQualifier) - 2))
                  wLength = sizeof(m_deviceQualifier) - 2;
                PrimeEndpoint(&m_ep0_in,
                    m_deviceQualifier, sizeof(m_deviceQualifier),
                    0, wLength);
                PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
                break;
              }

              case STRING:
              {
                u32 stringIndex = wValue & 0xFF;
                if (stringIndex < sizeof(m_stringDescriptors) / sizeof(u8*))
                {
                  const u8* descriptor = m_stringDescriptors[stringIndex];
                  // XXX: FIX FOR MYRIAD1 BSWAP
                  if (wLength > descriptor[3])  // First element is bLength
                    wLength = descriptor[3];
                  PrimeEndpoint(&m_ep0_in,
                      descriptor, wLength,
                      0, wLength);
                  PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
                } else {
                  setupStall = true;
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
            // XXX: Endian problem... Must send 1 byte representing config
            if (wLength > sizeof(m_configuration) - 3)
              wLength = sizeof(m_configuration) - 3;
            PrimeEndpoint(&m_ep0_in,
                &m_configuration, sizeof(m_configuration),
                0, wLength);
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            break;
          }

          case SET_CONFIGURATION:
          {
            // We only have 1 configuration for CDC ACM
            if (wValue > 1)
            {
              setupStall = true;
            } else {
              // Initialize the CDC ACM endpoints
              if (wValue == 1)
              {
                InitEndpoints();
              }

              m_configuration = wValue;
              PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
              PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            }
            break;
          }

          case GET_INTERFACE:
          {
            // XXX: Endian problem... Must send 1 byte
            if (wLength > sizeof(m_interface))
              wLength = m_interface;
            PrimeEndpoint(&m_ep0_in,
                &m_interface, sizeof(m_interface),
                0, wLength);
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            break;
          }

          case SET_INTERFACE:
          {
            m_interface = wIndex;
            // The interface comes from wIndex, so just ackknowledge it here
            PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
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

      static bool ClassRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength)
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
            // XXX: FIX FOR MYRIAD1 BSWAP
            PrimeEndpoint(&m_ep0_out, 
                m_lineCoding, sizeof(m_lineCoding),
                0, sizeof(m_lineCoding) - 1);
            PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
            break;
          }

          case 0x21:  // GET_LINE_CODING
          {
             // XXX: FIX FOR MYRIAD1 BSWAP
            PrimeEndpoint(&m_ep0_in,
                m_lineCoding, sizeof(m_lineCoding),
                0, sizeof(m_lineCoding) - 1);
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            break;
          }

          case 0x22:  // SET_CONTROL_LINE_STATE
          {
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
            break;
          }

          case 0x23:  // SEND_BREAK
          {
            PrimeEndpoint(&m_ep0_out, NULL, 0, 0, 0);
            PrimeEndpoint(&m_ep0_in, NULL, 0, 0, 0);
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

      static bool VendorRequestHandler(u32 bmRequestType, u32 bRequest,
          u32 wValue, u32 wIndex, u32 wLength)
      {
        // TODO: Implement

        bool setupStall = true;
        return setupStall;
      }

      void USBSendBuffer(const u8* buffer, const u32 size)
      {
        for (int i = 0; i < size; i++)
        {
          USBPutChar(buffer[i]);
        }
      }

      s32 USBGetChar()
      {
        if (m_bulkOutHead == m_bulkOutTail)
          return -1;

        // Endianness fix with xor 3
        s32 c = m_receiveBuffer[m_bulkOutTail ^ 3] & 0xFF;
        m_bulkOutTail = (m_bulkOutTail + 1) % sizeof(m_receiveBuffer);

        return c;
      }

      s32 USBPeekChar(u32 offset)
      {
        if (m_bulkOutHead == m_bulkOutTail)
          return -1;

        offset = (offset + m_bulkOutTail) % sizeof(m_receiveBuffer);
        
        // Endianness fix with xor 3
        return m_receiveBuffer[offset ^ 3];
      }

      int USBPutChar(int c)
      {
        m_transmitBuffer[m_bulkInHead] = c;
        m_bulkInHead = (m_bulkInHead + 1) % sizeof(m_transmitBuffer);
        return c;
      }

      u32 USBGetNumBytesToRead()
      {
        if (m_bulkOutHead >= m_bulkOutTail)
        {
          return m_bulkOutHead - m_bulkOutTail;
        }

        return sizeof(m_receiveBuffer) - m_bulkOutTail + m_bulkOutHead;
      }
    }
  }
}

#endif  // USE_USB

