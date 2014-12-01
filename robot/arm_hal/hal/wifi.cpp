#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "portable.h"

#define ENABLE_WIFI

// Enable the following for low level wifi debug on UART7/PF7
// #define ENABLE_WIFI_UART

// Set to 1 to enable LED indicator of wifi activity
//#define ENABLE_WIFI_LED 1

#ifndef ENABLE_WIFI_LED
#define SetLED(x,y)
#endif

static const char* ssid = "AnkiRobits";
static const char* psk = "KlaatuBaradaNikto!";

struct MACIP
{
  u8 MAC[6];
  u8 IP[4];
};

// This maps the modules at Anki to their IP address (as written on the front)
static const MACIP g_MACToIP[] =
{
  {
    0x00, 0x23, 0xA7, 0x0C, 0x02, 0x8C,
    192, 168, 3, 30
  },
  {
    0x00, 0x23, 0xA7, 0x0C, 0x01, 0xFF,
    192, 168, 3, 31
  },
  {
    0x00, 0x23, 0xA7, 0x0C, 0x03, 0x9B,
    192, 168, 3, 32
  },  
  {
    0x00, 0x23, 0xA7, 0x0C, 0x03, 0xC7,
    192, 168, 3, 33
  },  
  {
    0x00, 0x23, 0xA7, 0x0C, 0x03, 0xF7,
    192, 168, 3, 34
  },  
  {
    0x00, 0x23, 0xA7, 0x0C, 0x03, 0x81,
    192, 168, 3, 35
  },  
  {
    0x00, 0x23, 0xA7, 0x0C, 0x03, 0xF8,
    192, 168, 3, 36
  },  
};

static int g_moduleIndex = 0;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Function pointers depending on whether wifi or UART is used
      extern void (*StartTransfer)();
      extern s32 (*GetChar)(u32 timeout);
      
      extern int UARTGetFreeSpace();
      void UARTPutHex(u8 c);
      
      enum WaitState
      {
        WAIT_IDLE,
        
        WAIT_TX_SPI_READY_0,
        WAIT_TX_SPI_READY_1,
        WAIT_TX_DESCRIPTOR_DMA,
        WAIT_TX_DMA,
        
        WAIT_RX_INTERRUPT,
        WAIT_RX_SPI_READY_0,
        WAIT_RX_SPI_READY_1
      };

      enum WifiTransmitCommand
      {
        WIFI_TX_SEND_DATA                       = 0x00,
        WIFI_TX_SET_OPERATING_MODE              = 0x10,
        WIFI_TX_BAND                            = 0x11,
        WIFI_TX_INIT                            = 0x12,
        WIFI_TX_SCAN                            = 0x13,
        WIFI_TX_JOIN                            = 0x14,
        WIFI_TX_SET_POWER_MODE                  = 0x15,
        WIFI_TX_SET_SLEEP_TIMER                 = 0x16,
        WIFI_TX_QUERY_NETWORK_PARAMETERS        = 0x18,
        WIFI_TX_DISCONNECT                      = 0x19,
        WIFI_TX_RSSI_QUERY                      = 0x1A,
        WIFI_TX_SELECT_ANTENNA                  = 0x1B,
        WIFI_TX_SOFT_RESET                      = 0x1C,
        WIFI_TX_SET_REGION                      = 0x1D,
        WIFI_TX_SET_IP_PARAMETERS               = 0x41,
        WIFI_TX_SOCKET_CREATE                   = 0x42,
        WIFI_TX_SOCKET_CLOSE                    = 0x43,
        WIFI_TX_DNS_RESOLUTION                  = 0x44,
        WIFI_TX_QUERY_WLAN_CONNECTION_STATUS    = 0x48,
        WIFI_TX_QUERY_FIRMWARE_VERSION          = 0x49,
        WIFI_TX_GET_MAC_ADDRESS                 = 0x4A,
        WIFI_TX_CONFIGURE_P2P                   = 0x4B,
        WIFI_TX_CONFIGURE_EAP                   = 0x4C,
        WIFI_TX_SET_CERTIFICATE                 = 0x4D,
        WIFI_TX_QUERY_GO_PARAMETERS             = 0x4E,
        WIFI_TX_LOAD_WEBPAGE                    = 0x50,
        WIFI_TX_HTTP_GET                        = 0x51,
        WIFI_TX_HTTP_POST                       = 0x52,
        WIFI_TX_DNS_SERVER                      = 0x55
      };

      enum WifiReceiveCommand
      {
        //WIFI_RX_RECEIVE_DATA  // Ignore ID field when receiving data from a remote terminal
        WIFI_RX_SET_OPERATING_MODE              = 0x10,
        WIFI_RX_BAND                            = 0x11,
        WIFI_RX_INIT                            = 0x12,
        WIFI_RX_SCAN                            = 0x13,
        WIFI_RX_JOIN                            = 0x14,
        WIFI_RX_SET_POWER_MODE                  = 0x15,
        WIFI_RX_SET_SLEEP_TIMER                 = 0x16,
        WIFI_RX_QUERY_NETWORK_PARAMETERS        = 0x18,
        WIFI_RX_DISCONNECT                      = 0x19,
        WIFI_RX_RSSI_QUERY                      = 0x1A,
        WIFI_RX_SELECT_ANTENNA                  = 0x1B,
        WIFI_RX_SET_REGION                      = 0x1D,
        WIFI_RX_IP_PARAMETERS_CONFIGURE         = 0x41,
        WIFI_RX_SOCKET_CREATE                   = 0x42,
        WIFI_RX_SOCKET_CLOSE                    = 0x43,
        WIFI_RX_DNS_RESOLUTION                  = 0x44,
        WIFI_RX_QUERY_WLAN_CONNECTION_STATUS    = 0x48,
        WIFI_RX_QUERY_FIRMWARE_VERSION          = 0x49,
        WIFI_RX_GET_MAC_ADDRESS                 = 0x4A,
        WIFI_RX_CONFIGURE_P2P                   = 0x4B,
        WIFI_RX_CONFIGURE_EAP                   = 0x4C,
        WIFI_RX_SET_CERTIFICATE                 = 0x4D,
        WIFI_RX_QUERY_GO_PARAMETERS             = 0x4E,
        WIFI_RX_LOAD_WEBPAGE                    = 0x50,
        WIFI_RX_HTTP_GET                        = 0x51,
        WIFI_RX_HTTP_POST                       = 0x52,
        WIFI_RX_ASYNC_WFD                       = 0x54,
        WIFI_RX_DNS_SERVER                      = 0x55,
        WIFI_RX_ASYNC_TCP_SOCKET_CONNECTION_ESTABLISHED = 0x61,
        WIFI_RX_ASYNC_SOCKET_REMOTE_TERMINATE   = 0x62
      };

      enum WifiModuleStatus
      {
        WIFI_MODULE_BUFFERS_FULL          = (1 << 0),
        WIFI_MODULE_BUFFERS_EMTPY         = (1 << 1),
        WIFI_MODULE_PENDING_DATA          = (1 << 2),
        WIFI_MODULE_READY_TO_POWER_SAVE   = (1 << 3)
      };

      enum WifiInterrupt
      {
        WIFI_INTERRUPT_READ_RESPONSE  = 0,
        WIFI_INTERRUPT_READ_DATA      = 1
      };

      // This enum is useless, because the error is one byte and these values are
      // outside the signed range... Probably a documentation issue from old modules.
      // Some of the values are still correct...
      enum WifiSPIError
      {
        WIFI_ERROR_OK                             = 0,
        WIFI_ERROR_NO_AP_FOUND                    = 3,
        WIFI_ERROR_INVALID_BAND                   = 5,
        WIFI_ERROR_INVALID_CHANNEL                = 10,
        WIFI_ERROR_CONFIG_INCOMPLETE              = 20,  // Wi-Fi Direct or EAP configuration is not done
        WIFI_ERROR_MEMORY_ALLOCATION_FAILED       = 21,
        WIFI_ERROR_WRONG_INFO_IN_JOIN             = 22,
        WIFI_ERROR_PUSH_BUTTON_TOO_SOON           = 24,  // Push button command given before the expiry of previous push button command
        WIFI_ERROR_AP_NOT_FOUND                   = 25,  // (Specific) Access Point not found
        WIFI_ERROR_EAP_CONFIG_FAILED              = 28,
        WIFI_ERROR_P2P_CONFIG_FAILED              = 29,
        WIFI_ERROR_CANT_START_GROUP_OWNER_NEG     = 30,  // Unable to start Group Owner negotiation
        WIFI_ERROR_UNABLE_TO_JOIN                 = 32,
        WIFI_ERROR_COMMAND_GIVEN_IN_WRONG_STATE   = 33,  // Command given in incorrect
        WIFI_ERROR_QUERY_GO_IN_WRONG_MODE         = 34,  // Query GO parameters issued in incorrect operating mode
        WIFI_ERROR_UNABLE_TO_FORM_AP              = 35,  // Unable to form Access Point
        WIFI_ERROR_WRONG_INPUT_FOR_SCAN           = 36,  // Wrong Scan input parameters supplied to "Scan" command
        WIFI_ERROR_SOCKETS_UNAVAILABLE            = -2,  //Sockets not available. The error comes if the Host tries to open more than 8 sockets
        WIFI_ERROR_IP_CONFIG_FAILED               = -4,
        WIFI_ERROR_INVALID_DNS_CONTENT            = -69,  // Invalid content in the DNS response to the DNS Resolution query
        WIFI_ERROR_DNS_CLASS_ERROR                = -70,  // DNS Class error in the response to the DNS Resolution query
        WIFI_ERROR_DNS_COUNT_ERROR                = -72,  // DNS count error in the response to the DNS Resolution query
        WIFI_ERROR_DNS_RETURN_CODE_ERROR          = -73,  // DNS Return Code error in the response to the DNS Resolution query
        WIFI_ERROR_DNS_OPCODE_ERROR               = -74,  // DNS Opcode error in the response to the DNS Resolution query
        WIFI_ERROR_DNS_ID_MISMATCH                = -75,  // DNS ID mismatch between DNS Resolution request and response
        WIFI_ERROR_INVALID_DNS_INPUT              = -85,  // Invalid input to the DNS Resolution query
        WIFI_ERROR_DNS_RESPONSE_WAS_TIMED_OUT     = -92,  // DNS response was timed out
        WIFI_ERROR_ARP_REQUEST_FAILURE            = -95,
        WIFI_ERROR_DHCP_LEASE_TIME_EXPIRED        = -99,
        WIFI_ERROR_DHCP_HANDSHAKE_FAILURE         = -100,
        WIFI_ERROR_NON_EXISTENT_TCP_SOCKET        = -121,  // This error is issued when the module tried to connect to a non-existent TCP server socket on the remote side
        WIFI_ERROR_INVALID_SOCKET_PARAMETERS      = -121,
        WIFI_ERROR_SOCKET_ALREADY_OPEN            = -127,
        WIFI_ERROR_INVALID_COMMAND_IN_SEQUENCE    = -151,
        WIFI_ERROR_HTTP_SOCKET_CREATION_FAILED    = -191,
        WIFI_ERROR_TCP_SOCKET_CLOSE_TOO_SOON      = -192,  // TCP socket close command is issued before getting the response of the previous close command
        WIFI_ERROR_DNS_RESPONSE_TIMED_OUT         = -190,  // DNS response timed out
      };

      // This doesn't match the docs, but it seems to work
      struct ScanInfo
      {
        u8 reserved[4];
        u8 rfChannel;  // Probably not right...
        u8 securityMode;
        u8 rssiValue;
        u8 networkType;
        char ssid[32];
        char bssid[6];
      };

      struct ScanResponse
      {
        u32 scanCount;
        ScanInfo scanInfo[11];
      };

      struct JoinFrameSend
      {
        u8 reserved1;
        u8 reserved2;
        u8 dataRate;
        u8 powerLevel;
        char psk[64];
        char ssid[32];
        u8 reserved3;
        u8 reserved4;
        u8 reserved5;
        u8 ssidLength;
      };

      struct IPParametersResponse
      {
        u8 macAddress[6];
        u8 ipAddress[4];
        u8 netmask[4];
        u8 gateway[4];
      };

      struct SocketCreateResponse
      {
        u8 socketType[2];         // 16-bits
        u8 socketDescriptor[2];   // 16-bits
        u8 moduleSocket[2];       // 16-bits
        u8 moduleIPAddress[4];    // 32-bits
      };

      struct TCPPayload
      {
        u8 recvSocket[2];         // 16-bits
        u8 recvBufferLength[4];   // 32-bits
        u8 recvDataOffsetSize[2]; // 16-bits
        u8 fromPortNum[2];        // 16-bits
        u8 fromIPAddress[4];
        u8 reserved[40];
        u8 recvDataBuffer[1400];
      };

      const s32 TCP_SIZE_WITHOUT_PAYLOAD  = (sizeof(TCPPayload) - 1400);

      #define WAIT_FOR_INTERRUPT() while (!(GPIO_READ(GPIO_INTERRUPT) & PIN_INTERRUPT)) {}
      #define WAIT_FOR_SPI_READY_0() while (GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY) {}
      #define WAIT_FOR_SPI_READY_1() while (!(GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY)) {}

      // Ported to 2.1
      GPIO_PIN_SOURCE(SPI_CS, GPIOA, 8);      
      GPIO_PIN_SOURCE(SPI_SCK, GPIOC, 10);
      GPIO_PIN_SOURCE(SPI_MISO, GPIOC, 11);
      GPIO_PIN_SOURCE(SPI_MOSI, GPIOC, 12);
      GPIO_PIN_SOURCE(SPI_READY, GPIOC, 12);  // Multiplexed with SPI_MOSI
      GPIO_PIN_SOURCE(INTERRUPT, GPIOC, 10);  // Multiplexed with SPI_SCK

      extern int BUFFER_WRITE_SIZE;
      extern int BUFFER_READ_SIZE;
      extern u8 m_bufferWrite[];
      extern u8 m_bufferRead[];
      
      // The first two bytes are for the socket descriptor and the rest are reserved
      ONCHIP u8 m_bufferSocket[54] = {0};
      
      // This will not overflow, as long as Red Pine isn't full of liars in their
      // documentation... So, be aware that it might happen. Let's take the Pete-
      // approach and add some extra room, in case we forget things. After all,
      // this is only going to be used for debug robots.
      OFFCHIP u8 m_payloadRead[sizeof(TCPPayload) + 64];

      // Temporary buffer for writing data
      OFFCHIP u8 m_payloadWrite[sizeof(TCPPayload)];

      // The module specifies a maximum payload length of 1400, but with reserved
      // data, it's nicer to use 1344 (0x540) bytes for actual data.
      const s32 MAX_PAYLOAD_LENGTH = 1398;
      const s32 MAX_DATA_LENGTH = 1344;
      
      extern volatile s32 m_writeTail;
      extern volatile s32 m_writeHead;
      extern volatile s32 m_writeLength;
      
      extern volatile s32 m_readTail;
      extern volatile s32 m_readHead;
      
      extern volatile bool m_isTransferring;
      
      // Wifi module status is taken from one of the status read steps (SPI_READY == 0)
      u8 m_moduleStatus = 0;
      // Whether or not a remote client has connected to the wifi module
      volatile bool m_isClientConnected = false;
      // Has the requested DMA transfer completed
      volatile bool m_isDMAComplete = false;
      // Current state that the state machine is waiting to execute
      volatile WaitState m_waitState = WAIT_IDLE;
      
      static void WifiStateMachine();
    
      // For wifi driver debugging purposes
#ifdef ENABLE_WIFI_UART
      static int WifiPutChar(int c);      
      static void WifiPutString(const char* s)
      {
        while (*s)
          WifiPutChar(*s++);
      }      
      static void WifiPutHex(u8 c)
      {
        static u8 hex[] = "0123456789ABCDEF";
        WifiPutChar(hex[c >> 4]);
        WifiPutChar(hex[c & 0xF]);
        WifiPutChar(' ');
      }
#else
      static int WifiPutChar(int c) {}
      static void WifiPutString(const char* s) {}
      static void WifiPutHex(u8 c) {}      
#endif      

      static void CSLow()
      {
        PIN_AF(GPIO_SPI_MOSI, SOURCE_SPI_MOSI);
        PIN_AF(GPIO_SPI_SCK, SOURCE_SPI_SCK);
        GPIO_RESET(GPIO_SPI_CS, PIN_SPI_CS);
        
        // Disable GPIO interrupts - no matter what!
        EXTI->IMR &= ~(PIN_INTERRUPT | PIN_SPI_READY);
        
        MicroWait(1);  // Wait for pin to settle
      }
      
      static void CSHigh()
      {
        GPIO_SET(GPIO_SPI_CS, PIN_SPI_CS);
        PIN_IN(GPIO_SPI_MOSI, SOURCE_SPI_MOSI);
        PIN_IN(GPIO_SPI_SCK, SOURCE_SPI_SCK);
        
        MicroWait(1);  // Wait for pin to settle
        
        // Enable GPIO interrupts
        EXTI->IMR |= (PIN_INTERRUPT | PIN_SPI_READY);
      }

      static void SPI_SendReceive(u32 length, const u8* dataTX, u8* dataRX)
      {
        CSLow();
        
        for (int i = 0; i < length; i++)
        {
          while (!(SPI3->SR & SPI_FLAG_TXE))
            ;
          SPI3->DR = dataTX[i];
          
          while (!(SPI3->SR & SPI_FLAG_RXNE))
            ;
          dataRX[i] = SPI3->DR;
        }
        // Wait for transfers to complete
        while (SPI3->SR & SPI_FLAG_BSY)
          ;
        
        CSHigh();
        
        WifiPutString("\r\nCPU->WIFI: ");
        for (int i = 0; i < length; i++)
          WifiPutHex(dataTX[i]);
        WifiPutString("\r\nWIFI->CPU: ");
        for (int i = 0; i < length; i++)
          WifiPutHex(dataRX[i]);
        WifiPutString("\r\n");
      }

      static void WifiTxSpiReady0(WifiTransmitCommand commandID, u16 length)
      {
        u8 data;
        
        // Send TX_Descriptor_Frame1
        data = length & 0xFF;  // Low 8 bits of length (12-bits)
        SPI_SendReceive(1, &data, &m_payloadRead[0]);
        
        // Send TX_Descriptor_Frame2
        // High 4 bits of length (12-bits) | data or command
        data = ((length >> 8) & 0x0F) | ((commandID == WIFI_TX_SEND_DATA) << 5);
        SPI_SendReceive(1, &data, &m_payloadRead[1]);
        
        // Send TX_Descriptor_Frame3 (reserved/zero)
        data = 0;
        SPI_SendReceive(1, &data, &m_payloadRead[2]);
        
        // Send TX_Descriptor_Frame4 (reserved/zero)
        SPI_SendReceive(1, &data, &m_payloadRead[3]);
      }

      static void WifiTxSpiReady1(WifiTransmitCommand commandID)
      {
        int i;
        u8 data = 0;

        // Send Tx_Data_Descriptor_Frame1-10 (zeroes)
        for (i = 0; i < 10; i++)
          SPI_SendReceive(1, &data, &m_payloadRead[i]);
        
        // Send Tx_Data_Descriptor_Frame11-12
        SPI_SendReceive(1, (const u8*)&commandID, &m_payloadRead[10]);
        SPI_SendReceive(1, &data, &m_payloadRead[11]);
        
        m_moduleStatus = m_payloadRead[3];
      }

      static void WifiRxSpiReady0()
      {
        // Send Rx_Descriptor_Frame1-4 = {0x00, 0x00, 0x00, 0x01}
        // These do need to be separate transactions - Check figure 15: Rx Operation
        // Actually, every single "frame" must be a separate transaction -
        // The flow chart lies.
        u8 data0 = 0;
        u8 data1 = 1;
        SPI_SendReceive(1, &data0, &m_payloadRead[0]);
        SPI_SendReceive(1, &data0, &m_payloadRead[1]);
        SPI_SendReceive(1, &data0, &m_payloadRead[2]);
        SPI_SendReceive(1, &data1, &m_payloadRead[3]);
      }

      static s32 WifiRxSpiReady1()
      {
        int i;
        u8 data, error, lengthLow, lengthHigh;
        
        data = 0;
        // Send 4 null bytes to receive Rx_Data_Descriptor_Frame1-4
        SPI_SendReceive(1, &data, &lengthLow);
        SPI_SendReceive(1, &data, &lengthHigh);
        SPI_SendReceive(1, &data, &error);
        SPI_SendReceive(1, &data, &m_moduleStatus);
        
        if (!(m_moduleStatus & WIFI_MODULE_PENDING_DATA))
          return 0;
        
        // Check Rx_Data_Descriptor_Frame3 for SPI error and handle it
        if (error != 0 && error != 0xFF)
        {
          //DEBUG_PutString("SPI Error: ");
          //DEBUG_PutHex(error);
          //DEBUG_PutString("\r\n");
          
          return -1;
        }
        
        u16 payloadLength = lengthLow | ((lengthHigh & 0x0F) << 8);
        
        // Set Rx_Data_Read_Frame1-12 and receive Rx_Data_Read_Response_Frame1-12
        // The error argument is junk receive data (for now?)
        for (i = 0; i < 12; i++)
          SPI_SendReceive(1, &data, &error);
        
        // Send Rx_Payload_Read_Frame and receive Rx_Payload_Frame
        // The docs say we can stop after RDRRF12 if there's no response
        
        // The status variable holds the payload length
        if (payloadLength)
        {
          memset(m_payloadWrite, 0, payloadLength);
          SPI_SendReceive(payloadLength, m_payloadWrite, m_payloadRead);
        }
        
        //WifiInterrupt interruptType = 
        //  (WifiInterrupt)((m_bufferRead[2] >> 4) & 0x0F);
        
        return payloadLength;
      }

      // Returns the number of bytes received
      static int WifiReceive()
      {
        WAIT_FOR_INTERRUPT();
        
        WAIT_FOR_SPI_READY_0();
        
        WifiRxSpiReady0();
        
        WAIT_FOR_SPI_READY_1();
        
        return WifiRxSpiReady1();
      }

      static int WifiSendHeader(WifiTransmitCommand commandID, u16 length, 
        const u8* buffer)
      {
        // Check if the module buffer is full
        if (m_moduleStatus & WIFI_MODULE_BUFFERS_FULL)
          return -1;
        
        WAIT_FOR_SPI_READY_0();
        
        WifiTxSpiReady0(commandID, length);
        
        WAIT_FOR_SPI_READY_1();
        
        WifiTxSpiReady1(commandID);
        
        return 0;
      }

      // Blocking version for initialization
      static int WifiCommand(WifiTransmitCommand commandID, u16 length, 
        const u8* buffer)
      {
        int status = WifiSendHeader(commandID, length, buffer);
        if (status)
          return status;
        
        if (length)
          SPI_SendReceive(length, buffer, m_payloadRead);
        
        // Read response packet
        return WifiReceive();
      }

      // Replaced by state machine for DMA
      /*static int WifiSendData(u16 length, u8* buffer)
      {
        int status = WifiSendHeader(WIFI_TX_SEND_DATA, length, buffer);
        if (status)
          return status;
        
        CSLow(false);
        
        DMA1_Stream7->M0AR = (u32)buffer;
        DMA1_Stream7->NDTR = length;
        DMA_Cmd(DMA1_Stream7, ENABLE);
        
        // Wait for DMA to complete
        while (DMA1_Stream7->NDTR)
          ;
        
        while (SPI3->SR & SPI_FLAG_RXNE)
          SPI3->DR;
        
        while (SPI3->SR & SPI_FLAG_BSY)
          ;
        
        // Clear receive overrun bit by reading DR then SR
        SPI3->DR;
        SPI3->SR;
        
        CSHigh(false);
        
        return 0;
      }*/

      static int WifiCreateSocket()
      {
        static const u8 socketData[] =
        {
          2, 0,               // socketType = TCP Listener
          0xAF, 0x15,         // moduleSocket = 5551 (0x15AF)
          0xAF, 0x15,         // destSocket = 5551
          0,0,0,0             // destIpaddr - Doesn't matter with TCP Listener
        };
        if (WifiCommand(WIFI_TX_SOCKET_CREATE, sizeof(socketData), socketData) < 0)
          return -1;
        
        m_isClientConnected = false;
        
        return 0;
      }

      // Required handshake for module initialization from "5.3 Card Ready Operation"
      static int WifiEnterCardReadyState()
      {
        do
        {
          // Clear the buffer
          memset(m_bufferWrite, 0, BUFFER_WRITE_SIZE);
          
          // Perform card ready operation
          WAIT_FOR_INTERRUPT();
          
          WAIT_FOR_SPI_READY_0();
          
          // Send 3 NULL bytes
          SPI_SendReceive(3, m_bufferWrite, m_payloadRead);
          
          // Now send 0x01
          m_bufferWrite[4] = 1;
          SPI_SendReceive(1, &m_bufferWrite[4], m_payloadRead);
          
          WAIT_FOR_SPI_READY_1();
          
          // Send 4 null bytes
          SPI_SendReceive(4, m_bufferWrite, m_payloadRead);
        } while (!(m_payloadRead[3] & (1 << 2)));  // Check the response from "5.3 Card Ready Operation"
        
        // Send 12 null bytes
        m_bufferWrite[0] = 0;
        for (int i = 0; i < 12; i++)
          SPI_SendReceive(1, m_bufferWrite, &m_payloadRead[i]);
        
        return m_payloadRead[10] == 0x89 ? 0 : -1;
      }
      
      static void ConfigureDMA()
      {
        DMA_DeInit(DMA1_Stream7);
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_Channel_0;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI3->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_bufferWrite;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_BufferSize = 0xffff;  // Gets filled in later (SPI3->M0AR register)
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA1_Stream7, &DMA_InitStructure);
        
        SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);
      }
      
      static void ConfigureIRQ()
      {
        // Enable interrupt on DMA transfer complete
        DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);
        DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream7_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        // Setup interrupts on INTERRUPT and SPI_READY
        EXTI_InitTypeDef EXTI_InitStructure;
        EXTI_InitStructure.EXTI_Line = PIN_INTERRUPT;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);
        
        EXTI_InitStructure.EXTI_Line = PIN_SPI_READY;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
        EXTI_Init(&EXTI_InitStructure);
        
        // Disable the GPIO interrupts during initialization
        EXTI->IMR &= ~(PIN_INTERRUPT | PIN_SPI_READY);
        
        NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;  // SPI_READY / INTERRUPT
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
        NVIC_Init(&NVIC_InitStructure);
        
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, SOURCE_SPI_READY);
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, SOURCE_INTERRUPT);
      }

      static void WifiConfigurePins()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
                
        // Configure the pins for SPI in AF mode
        GPIO_PinAFConfig(GPIO_SPI_SCK, SOURCE_SPI_SCK, GPIO_AF_SPI3);
        GPIO_PinAFConfig(GPIO_SPI_MISO, SOURCE_SPI_MISO, GPIO_AF_SPI3);
        GPIO_PinAFConfig(GPIO_SPI_MOSI, SOURCE_SPI_MOSI, GPIO_AF_SPI3);
        
        // Configure the SPI pins
        GPIO_InitStructure.GPIO_Pin = PIN_SPI_SCK | PIN_SPI_MISO | PIN_SPI_MOSI;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_Init(GPIO_SPI_SCK, &GPIO_InitStructure);
        
        GPIO_SET(GPIO_SPI_CS, PIN_SPI_CS);  // Force CS high
        GPIO_InitStructure.GPIO_Pin = PIN_SPI_CS;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_Init(GPIO_SPI_CS, &GPIO_InitStructure);

        // Configure the input pins
        GPIO_InitStructure.GPIO_Pin = PIN_INTERRUPT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Init(GPIO_INTERRUPT, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = PIN_SPI_READY;
        GPIO_Init(GPIO_SPI_READY, &GPIO_InitStructure);
        
        // Initialize SPI in master mode
        SPI_I2S_DeInit(SPI3);
        SPI_InitTypeDef SPI_InitStructure;
        SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
        SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // This seems to work at div 2 at 22.5 MHz, but out of spec... (currently at 11.25 MHz)
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
        SPI_InitStructure.SPI_CRCPolynomial = 7;
        SPI_Init(SPI3, &SPI_InitStructure);
        SPI_Cmd(SPI3, ENABLE);
        
        // Ensure default state
        CSHigh();
      
// Turn on wifi UART if required
#ifdef ENABLE_WIFI_UART
        #define UART UART7        
        #define BAUDRATE 1000000
        
        static u8 s_wifiEnabled = 0;
        if (s_wifiEnabled) {
          WifiPutString("Wifi trying again!\r\n");
          return;
        }
        s_wifiEnabled = true;
        
        GPIO_PIN_SOURCE(TX, GPIOF, 7);
        
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, ENABLE);
        
        // Configure the pins for UART in AF mode
        GPIO_InitStructure.GPIO_Pin = PIN_TX;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  // XXX - why not OD to interface with 3V3 PropPlug?
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_TX, &GPIO_InitStructure);
        
        GPIO_PinAFConfig(GPIO_TX, SOURCE_TX, GPIO_AF_UART7);
        
        // Configure the UART for the appropriate baudrate
        USART_InitTypeDef USART_InitStructure;
        USART_Cmd(UART, DISABLE);
        USART_InitStructure.USART_BaudRate = BAUDRATE;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(UART, &USART_InitStructure);
        USART_Cmd(UART, ENABLE);
        
        WifiPutString("\r\nWIFI debug enabled\r\n");
      }
      
      static int WifiPutChar(int c)
      {
        UART->DR = c;
        while (!(UART->SR & USART_FLAG_TXE))
          ;
        return c;
#endif
      }       

      static int WifiJoinAP()
      {
        u8 data[36];
        memset(data, 0, sizeof(data));
        // Copy SSID into the structure for scanning, in order to find the hidden AP
        strlcpy((char*)&data[4], ssid, sizeof(data) - 4);        
        
        // Scan for access points (needs to be done, even though we know which to connect to)
        WifiPutString("Scan for access points...\r\n");
        SetLED(LED_LEFT_EYE_RIGHT, LED_GREEN);
        if (WifiCommand(WIFI_TX_SCAN, sizeof(data), data) < 0)
          return -1;        

        // Join the AP
        JoinFrameSend join;
        memset(&join, 0, sizeof(join));
        join.dataRate = 16;    // MCS3 - Most robust for our network
        join.powerLevel = 2;  // High power
        strlcpy(join.psk, psk, sizeof(join.psk));
        strlcpy(join.ssid, ssid, sizeof(join.ssid));
        join.ssidLength = strlen(ssid);
        
        WifiPutString("Join hidden AP...\r\n");
        SetLED(LED_LEFT_EYE_RIGHT, LED_PURPLE);
        if (WifiCommand(WIFI_TX_JOIN, sizeof(join), (u8*)&join) < 0)
          return -1;        

        // Configure the IP parameters
        const MACIP* macToIP = &g_MACToIP[g_moduleIndex];
        u8 parameters[] = 
        {
          0,                // Manual mode
          macToIP->IP[0], macToIP->IP[1], macToIP->IP[2], macToIP->IP[3],     // IP Address (0 for DHCP)
          255,255,248,0,    // Netmask
          192,168,3,1       // Gateway
        };
        WifiPutString("Set up my IP address...\r\n");
        SetLED(LED_LEFT_EYE_RIGHT, LED_YELLOW);
        if (WifiCommand(WIFI_TX_SET_IP_PARAMETERS, sizeof(parameters), parameters) < 0)
          return -1;
        
        WifiPutString("Open listener...\r\n");
        SetLED(LED_LEFT_EYE_RIGHT, LED_CYAN);
        if (WifiCreateSocket() < 0)
          return -1;

        SetLED(LED_LEFT_EYE_RIGHT, LED_WHITE);        
        return 0;
      }
      
      int UARTGetFreeReceiveSpace()
      {
        int tail = m_readTail;
        int head = m_readHead;
        if (head < tail)
          return tail - head;
        else
          return BUFFER_READ_SIZE - (head - tail);
      }
      
      static void WifiStateMachine()
      {
        // If state machine is already running, remember that we need to run it again later
        __disable_irq();
        static volatile u8 reentry = 0;
        reentry++;
        if (reentry > 1)
          return;
        __enable_irq();

        while (1)
        {
          switch (m_waitState)
          {
            case WAIT_IDLE:
            {
              m_isTransferring = false;
              
              // Check if there's data to be received
              if (GPIO_READ(GPIO_INTERRUPT) & PIN_INTERRUPT)
              {
                m_waitState = WAIT_RX_SPI_READY_0;
                m_isTransferring = true;
                
                SetLED(LED_LEFT_EYE_LEFT, LED_RED);
                WifiPutString("v");
                
              } else if ((UARTGetFreeSpace() < BUFFER_WRITE_SIZE) 
                          && m_isClientConnected
                          && !(m_moduleStatus & WIFI_MODULE_BUFFERS_FULL))  {
                // Send some data
                m_waitState = WAIT_TX_SPI_READY_0;
                m_isTransferring = true;
                
                SetLED(LED_LEFT_EYE_LEFT, LED_RED);
                WifiPutString("^");
              } else {
                // Nothing to do
                
                if (m_isClientConnected)
                  SetLED(LED_LEFT_EYE_LEFT, LED_BLUE);
                else
                  SetLED(LED_LEFT_EYE_LEFT, LED_GREEN);

                if (UARTGetFreeSpace() < BUFFER_WRITE_SIZE)
                  WifiPutString("o");
                else
                  WifiPutString(".");
                
                goto ret;
              }
              
              break;
            }
            
            case WAIT_TX_SPI_READY_0:
            {
              // Some other interrupt may have fired and triggered this, so verify
              if ((GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY))
                goto ret;
              
              // Set the correct length to send
              int tail = m_writeTail;
              int length = m_writeHead - tail;
              if (length < 0)
                length = BUFFER_WRITE_SIZE - tail;
              if (length > MAX_DATA_LENGTH)
                length = MAX_DATA_LENGTH;
              
              m_writeLength = length;
              
              // We need to set the length of the entire payload, not just the data
              // portion. However, we split DMA into two chunks and only want to set
              // m_writeLength to the actual data length.
              WifiTxSpiReady0(WIFI_TX_SEND_DATA, m_writeLength + sizeof(m_bufferSocket));
              
              // Set the next wait state
              m_waitState = WAIT_TX_SPI_READY_1;
              break;
            }
            
            case WAIT_TX_SPI_READY_1:
            {
              if (!(GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY))
                goto ret;

              WifiTxSpiReady1(WIFI_TX_SEND_DATA);
              
              // Set the next wait state
              m_waitState = WAIT_TX_DESCRIPTOR_DMA;
              
              // Configure DMA for transferring the socket descriptor and reserved bytes
              m_isDMAComplete = false;
              CSLow();
              
              DMA1_Stream7->M0AR = (u32)m_bufferSocket;
              DMA1_Stream7->NDTR = sizeof(m_bufferSocket);
              DMA_Cmd(DMA1_Stream7, ENABLE);
              break;
            }
            
            case WAIT_TX_DESCRIPTOR_DMA:
            {
              if (!m_isDMAComplete)
                goto ret;
              
              // Set the next wait state
              m_waitState = WAIT_TX_DMA;
              
              // Configure DMA for the actual data transfer
              m_isDMAComplete = false;
              
              DMA1_Stream7->M0AR = (u32)&m_bufferWrite[m_writeTail];
              DMA1_Stream7->NDTR = m_writeLength;
              DMA_Cmd(DMA1_Stream7, ENABLE);
              break;
            }
            
            case WAIT_TX_DMA:
            {
              volatile u32 v;
              
              if (!m_isDMAComplete)
                goto ret;
              
              // Clear out the receive buffer
              while (SPI3->SR & SPI_FLAG_RXNE)
                v = SPI3->DR;
              // Wait until not busy
              while (SPI3->SR & SPI_FLAG_BSY)
                ;
              // Clear receive overrun bit by reading DR then SR.
              // We do this, because receive DMA was not setup, nor does it matter.
              // The receive data during this stage should always be zeros.
              v = SPI3->DR;
              v = SPI3->SR;
              // We're done with the transfer, so de-assert chip select
              CSHigh();
              
              m_writeTail += m_writeLength;
              if (m_writeTail >= BUFFER_WRITE_SIZE)
                m_writeTail = 0;
              
              // Set the next wait state for figuring out whether to send more data or read the received data
              m_waitState = WAIT_IDLE;
              break;
            }
            
            case WAIT_RX_INTERRUPT:
            {
              if (!(GPIO_READ(GPIO_INTERRUPT) & PIN_INTERRUPT))
                goto ret;
              
              // Nothing else to do here, just wait for SPI_READY == 0
              m_waitState = WAIT_RX_SPI_READY_0;
              break;
            }
            
            case WAIT_RX_SPI_READY_0:
            {
              if ((GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY) != 0)
                goto ret;

              WifiRxSpiReady0();
              
              m_waitState = WAIT_RX_SPI_READY_1;
              break;
            }
            
            case WAIT_RX_SPI_READY_1:
            {
              if ((GPIO_READ(GPIO_SPI_READY) & PIN_SPI_READY) == 0)
                goto ret;
              
              // Don't modify interrupts, just wait for all data to be received
              int result = WifiRxSpiReady1();
              
              // Check for a connection and disconnection
              if (!m_isClientConnected && result == 8 )
              {
                m_bufferSocket[0] = m_payloadRead[0];
                m_bufferSocket[1] = m_payloadRead[1];
                m_isClientConnected = true;
                
                // Dump the buffer on connection
                //
                // TODO: Make this optional, depending on a command. 
                // Perhaps connecting to one socket should flush the buffer, and connecting to another doesn't?
                m_writeTail = m_writeHead;   
              } else if (result == 2 && m_isClientConnected) {
                if (m_bufferSocket[0] == m_payloadRead[0] &&
                    m_bufferSocket[1] == m_payloadRead[1])
                {
                  // Never call this command... It breaks being able to reconnect
                  //WifiCommand(WIFI_TX_SOCKET_CLOSE, sizeof(m_socket), m_socket);
                  WifiCreateSocket();
                }
              } else if (result >= TCP_SIZE_WITHOUT_PAYLOAD) {
                // Handle an actual TCP payload
                int length, bytesLeft;
                TCPPayload* payload = (TCPPayload*)m_payloadRead;
                
                length = payload->recvBufferLength[0] | (payload->recvBufferLength[1] << 8);
                
                // TODO: Should probably notify there is a dropped packet
                if (UARTGetFreeReceiveSpace() >= (length + 1))
                {
                  bytesLeft = BUFFER_READ_SIZE - m_readHead;
                  if (length <= bytesLeft)
                  {
                    memcpy(&m_bufferRead[m_readHead], payload->recvDataBuffer, length);
                    m_readHead += length;
                    
                    if (m_readHead >= BUFFER_READ_SIZE)
                    {
                      m_readHead = 0;
                    }
                  } else {
                    // Copy to the end of the buffer, then wrap around for the rest
                    int lengthFirst = bytesLeft;
                    memcpy(&m_bufferRead[m_readHead], payload->recvDataBuffer, lengthFirst);
                    
                    bytesLeft = length - lengthFirst;
                    memcpy(m_bufferRead, &payload->recvDataBuffer[lengthFirst], bytesLeft);
                    
                    m_readHead = bytesLeft;
                  }
                }
              }
              
              m_waitState = WAIT_IDLE;
              break;
            }
          }
        }
      ret:
        __disable_irq();
        u8 oldreentry = reentry;
        reentry = 0;
        __enable_irq();
        if (oldreentry > 1)
          WifiStateMachine();
      }
      
      s32 WifiGetCharacter(u32 timeout)
      {
        u32 startTime = GetMicroCounter();

        do
        {
          // Make sure there's data in the FIFO
          if (m_readTail != m_readHead)
          {
            u8 value = m_bufferRead[m_readTail];
            m_readTail = (m_readTail + 1) % BUFFER_READ_SIZE;
            return value;
          }
        }
        while ((GetMicroCounter() - startTime) < timeout);

        // No data, so return with an error
        return -1;
      }
      
      static int WifiConfigure()
      {
        m_writeHead = m_writeTail = m_readTail = m_readHead = 0;
        
        StartTransfer = WifiStateMachine;
        GetChar = WifiGetCharacter;
        
        WifiConfigurePins();
        
        SetLED(LED_LEFT_EYE_LEFT, LED_RED);
        SetLED(LED_LEFT_EYE_RIGHT, LED_RED);
        
        WifiPutString("Trying to enter card ready...\r\n");
        if (WifiEnterCardReadyState() < 0)
          return -1;
        
        // Set operating mode to 0 - must be the first command after reset
        WifiPutString("Set op mode 0...\r\n");
        u8 mode = 0;
        if (WifiCommand(WIFI_TX_SET_OPERATING_MODE, 1, &mode) < 0)
          return -1;
        
        // Band == 0: 2.4 GHz
        // Band == 1: 5 GHz
        WifiPutString("Set 5GHz...\r\n");
        mode = 1;
        if (WifiCommand(WIFI_TX_BAND, 1, &mode) < 0)
          return -1;
        
        // Initialize internal stuff
        // This command programs the module's baseband and RF components and
        // returns the MAC address of the module
        WifiPutString("Get MAC address...\r\n");
        WifiCommand(WIFI_TX_INIT, 0, NULL);
        
        // Verify the MAC address
        g_moduleIndex = -1;
        for (int i = 0; i < sizeof(g_MACToIP) / sizeof(g_MACToIP[0]); i++)
        {
          const MACIP* macToIP = &g_MACToIP[i];
          bool matches = true;
          for (int j = 0; j < sizeof(macToIP->MAC); j++)
          {
            if (m_payloadRead[j] != macToIP->MAC[j])
            {
              matches = false;
              break;
            }
          }
          
          if (matches)
          {
            g_moduleIndex = i;
            break;
          }
        }
        
        // Unknown module
        if (g_moduleIndex < 0)
        {
          // Force enable the LED
          //g_wifiLEDs = true;
          WifiPutString("Unknown module!\r\n");
          while (1)
          {
            SetLED(LED_LEFT_EYE_TOP, (LEDColor)(LED_RED | LED_BLUE | LED_GREEN));
            MicroWait(500000);
            SetLED(LED_LEFT_EYE_TOP, LED_OFF);
            MicroWait(500000);
          }
        }
        
        WifiPutString("Join access point...\r\n");
        if (WifiJoinAP() < 0)
          return -1;
        
        WifiPutString("Configured, prepare to run!\r\n");
        ConfigureDMA();
        ConfigureIRQ();
        
        // Enable GPIO interrupts on these pins
        EXTI->IMR |= (PIN_INTERRUPT | PIN_SPI_READY);
        
        SetLED(LED_LEFT_EYE_LEFT, LED_GREEN);
        SetLED(LED_LEFT_EYE_RIGHT, LED_OFF);
        
        return 0;
      }
      
      bool WifiInit()
      {
#ifndef ENABLE_WIFI
        return false;
#else 
        // Auto detect wifi module by checking for CS pull-up
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        
        // Check interrupt pin instead - it will be pulled up if module is healthy/booted
        // XXX-NDM:  This doesn't work right since CS is pulled up with 47K instead of 10K
        GPIO_RESET(GPIO_SPI_CS, PIN_SPI_CS);
        PIN_OUT(GPIO_SPI_CS, SOURCE_SPI_CS);        
        MicroWait(1);  // Drain pin
        PIN_IN(GPIO_SPI_CS, SOURCE_SPI_CS);
        MicroWait(1);  // Float pin
        
        if (GPIO_READ(GPIO_SPI_CS) & PIN_SPI_CS)
          return WifiConfigure() >= 0;    // Bad is -1, good is 0.. grr!
        else
          return false;
#endif        
      }
      
      bool WifiHasClient()
      {
        return m_isClientConnected;
      }
    }
  }
}

extern "C"
{
  // Used for SPI transfer-complete interrupt
  void DMA1_Stream7_IRQHandler()
  {
    using namespace Anki::Cozmo::HAL;
    // Clear the transfer-complete interrupt flag
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7);
   
    m_isDMAComplete = true;
    
    // Run the state machine
    WifiStateMachine();
  }

  // Used for SPI_READY GPIO interrupt on rising and falling edges and the
  // INTERRUPT pin on rising edge
  void EXTI15_10_IRQHandler()
  {
    using namespace Anki::Cozmo::HAL;
    
    // Clear the pending bits
    EXTI->PR = (PIN_SPI_READY | PIN_INTERRUPT);
    
    // Run the state machine
    WifiStateMachine();
  }
}
