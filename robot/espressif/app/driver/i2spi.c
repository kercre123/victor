 /*
 * @file I2S driver for Espressif chip.
 * @author Daniel Casner <daniel@anki.com>
 *
 * Original code taken from Espressif MP3 decoder project. Heavily reworked to liberate it from FreeRTOS and enable
 * bidirectional communication.
 */

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "driver/i2s_reg.h"
#include "driver/slc_register.h"
#include "driver/sdio_slv.h"
#include "driver/i2spi.h" 
#include "driver/i2s_ets.h"
#include "backgroundTask.h"
#include "foregroundTask.h"
#include "imageSender.h"


#define I2SPI_DEBUG 1
#if I2SPI_DEBUG
#define debug(...) os_printf(__VA_ARGS__)
#define dbpc(char) os_put_char(char)
#define dbph(num, nibbles) os_put_hex(num, nibbles)
#define dbnl() do { os_put_char('\r'); os_put_char('\n'); } while (false)
#define assert(cond, ...) if ((cond) == false) { while (true) { os_printf(__VA_ARGS__); }}
#else
#define debug(...)
#define dbpc(...)
#define dbph(...)
#define dbnl()
#define assert(...)
#endif

#define I2SPI_ISR_PROFILE_FULL 1
#define I2SPI_ISR_PROFILE_RX   2
#define I2SPI_ISR_PROFILE_TX   3
#define I2SPI_ISR_PROFILE_SYNC 4
#define I2SPI_ISR_PROFILE_BOOT 5
#define I2SPI_ISR_PROFILE_NORM 6
#define I2SPI_ISR_PROFILE_TMD  7

#define I2SPI_ISR_PROFILING I2SPI_ISR_PROFILE_FULL
#if I2SPI_ISR_PROFILING
#define PROFILING_PIN 0
#define isrProfStart(which) if ((which) == I2SPI_ISR_PROFILING) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<PROFILING_PIN)
#define isrProfEnd(which)   if ((which) == I2SPI_ISR_PROFILING) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<PROFILING_PIN)
#else
#define isrProfStart(which)
#define isrProfEnd(which)
#endif

extern bool i2spiSynchronizedCallback(uint32 param);
extern bool i2spiRecoveryCallback(uint32 param);

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define asDesc(x) ((struct sdio_queue*)(x))

/// Enumerated state values for bootloaderCommandPhase
enum {
  BLCP_command_done = -3,
  BLCP_flash_header = -2,
  BLCP_none         = -1,
  BLCP_fwb_start    =  0,
};

// Group state variables for driver into a struct to make the code easier to read and improve code speed
typedef struct {
  uint32_t* pendingFWB; ///< Pointer to pending firmware block data
  uint32_t  txOverflowCount;
  uint32_t  rxOverflowCount;
  uint32_t  phaseErrorCount;
  int32_t   integralDrift;
  int16_t   dropPhase; ///< Stores the estimated alignment for incomming drops in half words
  int16_t   outgoingPhase; ///< Stores the alignment for outgoing drops in half words
  volatile uint16_t messageBufferWind; ///< Write index into message buffer
  volatile uint16_t messageBufferRind; ///< Read index into message buffer
  volatile uint16_t rtipBufferWind; ///< Write index into relay buffer
  volatile uint16_t rtipBufferRind; ///< Read index into relay buffer
  int16_t   rtipBootloaderState; ///< The last state we've received from the RTIP bootloader regarding it's state
  int16_t   rtipRXQueueEstimate; ///< The number of bytes we estimate are in the RTIP's CLAD rx queue
  int16_t   bootloaderCommandPhase; ///< Operational phase for bootloader commands
  I2SPIMode mode; ///< The current mode of the driver
} I2SpiDriver;

// Group all audio buffer state together
typedef struct {
  volatile int16_t rind;
  volatile int16_t wind;
  volatile int16_t silenceSamples;
} AudioBufferState;

// Group screen buffer state together
typedef struct {
  uint32_t buffer[SCREEN_BUFFER_SIZE];
  uint32_t rectFlags;
  volatile uint8_t wind;
  volatile uint8_t rind;
} ScreenBufferState;
ct_assert(SCREEN_BUFFER_SIZE < 256); // Required for uint8_t indexes
ct_assert(SCREEN_BUFFER_SIZE <= 32); // Required for rect bit flag array

/// Buffer size must match I2S TX FIFO depth
#define DMA_BUF_SIZE (512) /// This must be 512 Espressif DMA to work and for logic in i2spi.c to work
ASSERT_IS_MULTIPLE_OF_TWO(DMA_BUF_SIZE); /// Must be a multiple of two for half word counters to work
/// How many buffers are required given the above constraints.
#define DMA_BUF_COUNT (4) // Need at least two for the peripheral and two to operate on
/// Buffer size for sending messages to the RTIP
#define I2SPI_MESSAGE_BUF_SIZE (1024)
ASSERT_IS_POWER_OF_TWO(I2SPI_MESSAGE_BUF_SIZE); // Required for mask below
#define I2SPI_MESSAGE_BUF_SIZE_MASK (I2SPI_MESSAGE_BUF_SIZE-1)
/// Buffer size size requirements for messages from RTIP to WiFi
#define RELAY_BUFFER_SIZE (512)
ct_assert(RELAY_BUFFER_SIZE > RTIP_MAX_CLAD_MSG_SIZE + DROP_TO_WIFI_MAX_PAYLOAD);
ASSERT_IS_POWER_OF_TWO(RELAY_BUFFER_SIZE);
#define RELAY_BUFFER_SIZE_MASK (RELAY_BUFFER_SIZE-1)
// Number of image data buffers to use
#define NUM_IMAGE_DATA_BUFFERS (2)
ASSERT_IS_POWER_OF_TWO(NUM_IMAGE_DATA_BUFFERS);
#define NUM_IMAGE_DATA_BUFFER_MASK (NUM_IMAGE_DATA_BUFFERS-1)

/// DMA I2SPI transmit buffers
static unsigned int txBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue txQueue[DMA_BUF_COUNT];
/// DMA I2SPI transmit buffers
static unsigned int rxBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue rxQueue[DMA_BUF_COUNT];
/// Buffer for message to send to the RTIP
static uint8_t messageBuffer[I2SPI_MESSAGE_BUF_SIZE];
/// Buffer for messages from the RTIP
static uint8 relayBuffer[RELAY_BUFFER_SIZE];
/// Buffer storing 
static STORE_ATTR s8 audioBuffer[AUDIO_BUFFER_SIZE];
/// Main driver state structure
static I2SpiDriver self;
/// Audio buffer state information
static AudioBufferState audio;
/// All state for screen
static ScreenBufferState screen;
/// Task queue for sending image data
static os_event_t imageSendTaskQueue[IMAGE_SEND_TASK_QUEUE_DEPTH];
/// Buffers for sending image data, make sure we have a place to put new data while waiting for task to send over WiFi
static ImageDataBuffer imageBuffers[NUM_IMAGE_DATA_BUFFERS];

/** Phase relationship between incoming drops and outgoing drops
 * This comes from the phase relationship between I2SPI transmission and reception required for DMA timing on the K02,
 * the phase offset introduced by the I2S FIFO on the Espressif, aiming for the middle of the SPI receive buffer on the
 * K02 and experimentally determined fudge factor.
 */
#define DROP_TX_PHASE_ADJUST ((DROP_SPACING-12-(6*DMA_BUF_COUNT))/2)

inline void    i2spiSetAudioSilenceSamples(const int16_t silence) { audio.silenceSamples = silence; }
inline int16_t i2spiGetAudioSilenceSamples(void) { return audio.silenceSamples; }
int16_t ICACHE_FLASH_ATTR i2spiGetAudioBufferAvailable(void)
{
  return AUDIO_BUFFER_SIZE - ((audio.wind - audio.rind) & AUDIO_BUFFER_SIZE_MASK);
}
void ICACHE_FLASH_ATTR i2spiBufferAudio(uint8_t* buffer, const int16_t length)
{
  const int16_t wind = audio.wind;
  const int16_t firstCopy = AUDIO_BUFFER_SIZE - wind;
  if (unlikely(firstCopy < length))
  {
    os_memcpy(audioBuffer + wind, buffer, firstCopy);
    os_memcpy(audioBuffer, buffer + firstCopy, length - firstCopy);
  }
  else
  {
    os_memcpy(audioBuffer + wind, buffer, length);
  }  
  audio.wind = (wind + length) & AUDIO_BUFFER_SIZE_MASK;
}

int8_t ICACHE_FLASH_ATTR i2spiGetScreenBufferAvailable(void)
{
  return (SCREEN_BUFFER_SIZE - ((screen.wind - screen.rind) & SCREEN_BUFFER_SIZE_MASK)) - 1;
}

inline void ICACHE_FLASH_ATTR i2spiPushScreenData(const uint32_t* data, const bool rect)
{
  const uint8_t wind = screen.wind;
  screen.buffer[wind] = *data;
  if (rect) screen.rectFlags |= 1<<wind;
  else screen.rectFlags &= ~(1<<wind);
  screen.wind = (wind + 1) & SCREEN_BUFFER_SIZE_MASK;
}

inline uint8_t PumpScreenData(uint8_t* dest)
{
  static u8 transmitChain = 0;
  if (transmitChain == MAX_TX_CHAIN_COUNT)
  {
    transmitChain = 0;
    return 0;
  }
  else
  {
    const int8_t wind = screen.wind;
    const int8_t rind = screen.rind;
    if (wind == rind)
    {
      transmitChain = 0;
      return 0;
    }
    else
    {
      transmitChain++;
      os_memcpy(dest, &(screen.buffer[rind]), MAX_SCREEN_BYTES_PER_DROP);
      screen.rind = (rind + 1) & SCREEN_BUFFER_SIZE_MASK;
      return screenDataValid | ((screen.rectFlags & 1<<rind) ? screenRectData : 0);
    }
  }
}

inline uint8_t PumpAudioData(uint8_t* dest)
{
  const int16_t wind = audio.wind;
  int16_t rind = audio.rind;
  // Decrement silence if we have any
  if (audio.silenceSamples > 0)
  {
    audio.silenceSamples -= MAX_AUDIO_BYTES_PER_DROP;
    if (audio.silenceSamples < 0) audio.silenceSamples = 0; // Don't go below 0
    return 0;
  }
  else if (rind == wind) // Check for empty buffer
  {
    return 0;
  }
  else
  {
    int i;
    for (i=0; i<MAX_AUDIO_BYTES_PER_DROP; ++i)
    {
      dest[i] = audioBuffer[rind];
      rind = (rind + 1) & AUDIO_BUFFER_SIZE_MASK;
    }
    audio.rind = rind;
    return audioDataValid;
  }
}

void ICACHE_FLASH_ATTR i2spiUpdateRtipQueueEstimate(void)
{
  static uint32_t lastFlushTime = 0;
  const uint32_t now = system_get_time();
  const int delta = now - lastFlushTime;
  const int flush = (RTIP_RX_FLUSH_PER_DROP * DROPS_PER_SECOND * delta) / 1000000;
  if (flush > 0)
  {
    if (flush > self.rtipRXQueueEstimate) self.rtipRXQueueEstimate = 0;
    else self.rtipRXQueueEstimate -= flush;
    lastFlushTime = now; // Only update when actually do something in case called too often
  }
}

bool pokeRTIPqueue(const int bytes)
{
  if ((RTIP_RX_MAX_BUFFER - self.rtipRXQueueEstimate) > bytes)
  {
    self.rtipRXQueueEstimate += bytes;
    return true;
  }
  else return false;
}


/// Prep an sdio_queue structure (DMA descriptor) for (re)use
void prepSdioQueue(struct sdio_queue* desc, uint8 eof)
{
  desc->owner     = 1;
  desc->eof       = eof;
  desc->sub_sof   = 0;
  desc->datalen   = DMA_BUF_SIZE;
  desc->blocksize = DMA_BUF_SIZE;
  desc->unused    = 0;
}

/** Processes an incomming drop from the RTIP to the WiFi
 * @param drop A pointer to a complete drop
 */
inline void processDrop(DropToWiFi* drop)
{
  const int8 rxJpegLen = (drop->droplet & jpegLenMask) * 4;
  if (rxJpegLen > 0) // Handle jpeg data
  {
    static int8_t activeImgBuffer = 0;
    static bool skipFrame = true; // Starts true because there's no sense sending anything until a full frame
    if (skipFrame) // Lost at least some of the data from this frame to skip the rest
    {
      // Got to the end of last frame so try next one
      if (drop->droplet & jpegEOF)
      {
        const uint32_t* meta = (uint32_t*)(drop->payload + rxJpegLen - 8);
        if (system_os_post(IMAGE_SEND_TASK_PRIO, QIS_resume, meta[1]))
        {
          skipFrame = false;
        }
      }
    }
    else if (imageBuffers[activeImgBuffer].status != 0) // If currently busy sending a chunk
    {
      // If this isn't the end of the frame, we have to skip the rest
      if ((drop->droplet & jpegEOF) == 0) 
      {
        skipFrame = true;
        system_os_post(IMAGE_SEND_TASK_PRIO, QIS_overflow, 0);
      }
    }
    else
    {
      int remainingJpegSpace = IMAGE_DATA_MAX_LENGTH - imageBuffers[activeImgBuffer].length;
      assert(remainingJpegSpace >= rxJpegLen, "WTF: %x %x %x\r\n", imageBuffers[activeImgBuffer].length, remainingJpegSpace, rxJpegLen);
      // drop payload is 4 byte aligned and rxJpegLen is always a multiple of 4 so 4 byte aligned copy will always work
      os_memcpy(imageBuffers[activeImgBuffer].data + imageBuffers[activeImgBuffer].length, drop->payload, rxJpegLen);
      imageBuffers[activeImgBuffer].length += rxJpegLen;
      remainingJpegSpace -= rxJpegLen;
      // Send this data if appropriate
      if (drop->droplet & jpegEOF) // If end of frame send imeediately
      {
        if(system_os_post(IMAGE_SEND_TASK_PRIO, QIS_endOfFrame, (uint32_t)&imageBuffers[activeImgBuffer]))
        {
          imageBuffers[activeImgBuffer].status = 1;
          activeImgBuffer = (activeImgBuffer + 1) & NUM_IMAGE_DATA_BUFFER_MASK;
        }
        else
        {
          imageBuffers[activeImgBuffer].length = 0; // If we couldn't queue the task, reset the image
        }
      }
      else if (remainingJpegSpace < (int)(jpegLenMask*4)) // If there might not be enough room for the nect drop, send
      {
        if (system_os_post(IMAGE_SEND_TASK_PRIO, QIS_none, (uint32_t)&imageBuffers[activeImgBuffer]))
        {
          imageBuffers[activeImgBuffer].status = 1;
          activeImgBuffer = (activeImgBuffer + 1) & NUM_IMAGE_DATA_BUFFER_MASK;
        }
        else // If we couldn't queue the task
        {
          skipFrame = true; // Have to skip the rest of this frame
          imageBuffers[activeImgBuffer].length = 0; // Reset the image
        }
      }
    } // End not skipping frame
  } // End handling JPEG data
  
  if (drop->payloadLen > 0) // Handling CLAD data
  {
    const uint16_t rind = self.rtipBufferRind;
    uint16_t wind = self.rtipBufferWind;
    const uint16_t available = RELAY_BUFFER_SIZE - ((wind - rind) & RELAY_BUFFER_SIZE_MASK);
    if (likely(drop->payloadLen < available)) // Leave space for head and tail to not touch
    {
      const int firstCopy = RELAY_BUFFER_SIZE - wind;
      if (unlikely(firstCopy < drop->payloadLen))
      {
        os_memcpy(relayBuffer + wind, drop->payload + rxJpegLen, firstCopy);
        os_memcpy(relayBuffer, drop->payload + rxJpegLen + firstCopy, drop->payloadLen - firstCopy);
      }
      else
      {
        os_memcpy(relayBuffer + wind, drop->payload + rxJpegLen, drop->payloadLen);
      }
      self.rtipBufferWind = (wind + drop->payloadLen) & RELAY_BUFFER_SIZE_MASK;
    }
    else
    {
      self.rxOverflowCount++;
      self.rtipBufferRind = 0; // Reset buffer
      self.rtipBufferWind = 0;
      os_memset(relayBuffer, 0 , RELAY_BUFFER_SIZE);
      dbpc('C'); dbpc('R'); dbpc('O'); dbpc('F'); dbph(self.rtipBufferRind, 4); dbph(wind, 4);
    }
  }
}



ct_assert(DMA_BUF_SIZE == 512); // We assume that the DMA buff size is 128 32bit words in a lot of logic below.
#define DRIFT_MARGIN 2

// Subhandler for dmaisr for receive interrupts
inline void receiveCompleteHandler(void)
{
  uint32_t eofDesAddr = READ_PERI_REG(SLC_TX_EOF_DES_ADDR);
  struct sdio_queue* desc = asDesc(eofDesAddr);
  static DropToWiFi drop;
  static uint8 dropRdInd = 0; // In 16bit half-words
  static int16_t drift = 0;
  uint16_t* buf = (uint16_t*)(desc->buf_ptr);
  
  while(true)
  {
    if (dropRdInd == 0) // If we're looking for the next drop, not in the middle of an existing one
    {
      while(self.dropPhase < DMA_BUF_SIZE/2) // Search for preamble
      {
        if (buf[self.dropPhase] == TO_WIFI_PREAMBLE)
        {
          if (unlikely(drift > DRIFT_MARGIN))
          {
            self.phaseErrorCount++;
            isrProfStart(I2SPI_ISR_PROFILE_TMD);
            dbpc('!'); dbpc('T'); dbpc('M'); dbpc('D'); dbph(drift, 4);
            system_deep_sleep(0);
            isrProfEnd(I2SPI_ISR_PROFILE_TMD);
          }
          self.outgoingPhase += drift;
          self.integralDrift += drift; // Track integral drift
          drift = -DRIFT_MARGIN; // Reset search
          break;
        } // End found drop
        self.dropPhase++;
        drift++;
      } // End searching for preamble
    } // End looking for next drop
    
    if (likely(self.dropPhase < DMA_BUF_SIZE/2)) // If we found a header
    {
      const int bytesToRead = min(DROP_TO_WIFI_SIZE-(dropRdInd*2), DMA_BUF_SIZE - (self.dropPhase*2));
      os_memcpy(((uint16_t*)&drop) + dropRdInd, buf + self.dropPhase + dropRdInd, bytesToRead);
      dropRdInd += bytesToRead/2;
      if (dropRdInd*2 == DROP_TO_WIFI_SIZE) // The end of the drop was in this buffer
      {
        processDrop(&drop);
        dropRdInd = 0;
        self.dropPhase += (DROP_SPACING/2) - DRIFT_MARGIN;
      }
      else break; // The end of the drop wasn't in this buffer, go on to the next one
    } // End found a header
    else break; // Didn't find a drop in this buffer
  } // Done processing this buffer
  self.dropPhase -= DMA_BUF_SIZE/2; // Now looking in next buffer
  prepSdioQueue(desc, 0);
}

/** Fills a drop into the outgoing DMA buffers
 * Uses the outgoingPhase module variables and advances them as nessisary
 * Data is gathered by calling PumpAudioData and PumpScreenData and pulling from messageBuffer which is filled by
 * i2spiQueueMessage.
 * @return Whether there is room for another drop in this buffer after the one that has just been filled.
 */
inline bool makeDrop(uint16_t* txBuf)
{
  static DropToRTIP drop;
  static uint8_t dropWrInd = 0;
    
  if (dropWrInd == 0) // Populating a new drop, not finishing an old one this call to makeDrop
  {
    drop.preamble   = TO_RTIP_PREAMBLE;
    drop.payloadLen = 0;
    
    // Fill in the drop itself
    drop.droplet = PumpAudioData(drop.audioData) |
                   PumpScreenData(drop.screenData);
    
    const uint16_t wind = self.messageBufferWind;
    uint16_t rind = self.messageBufferRind;
    if (rind != wind) // Have CLAD payload to send
    {
      const uint16_t messageAvailable = I2SPI_MESSAGE_BUF_SIZE - ((rind - wind) & I2SPI_MESSAGE_BUF_SIZE_MASK);
      const uint8_t messageSize = messageBuffer[rind];
      assert(messageSize <= messageAvailable, "ERROR I2SPI messageBuffer is corrupt! %d > %d, %02x\r\n", messageSize, messageAvailable, messageBuffer[rind]);
      if (pokeRTIPqueue(messageSize)) // Estimate that the RTIP has room for this
      {
        rind = (rind + 1) & I2SPI_MESSAGE_BUF_SIZE_MASK; // Advance past size
        drop.payloadLen = messageSize;
        const int firstCopy = I2SPI_MESSAGE_BUF_SIZE - rind;
        if (unlikely(firstCopy < messageSize))
        {
          os_memcpy(drop.payload, messageBuffer + rind, firstCopy);
          os_memcpy(drop.payload + firstCopy, messageBuffer, messageSize - firstCopy);
        }
        else
        {
          os_memcpy(drop.payload, messageBuffer + rind, messageSize);
        }
        self.messageBufferRind = (rind + messageSize) & I2SPI_MESSAGE_BUF_SIZE_MASK;
      }
    }
  }
  else // Finishing drop started in last pass
  {
    os_memcpy(txBuf, ((uint8_t*)&drop) + dropWrInd, DROP_TO_RTIP_SIZE - dropWrInd);
    dropWrInd = 0;
    // Don't advance outgoingPhase because it was already done when we started the drop
    return true; // We just started this buffer so definitely still room
  }
  
  if (self.outgoingPhase >= 0)
  {
    const int remaining = DMA_BUF_SIZE - (self.outgoingPhase*2);
    if (remaining >= (int)DROP_TO_RTIP_SIZE) // Whole drop fits in this buffer
    {
      os_memcpy(txBuf + self.outgoingPhase, &drop, DROP_TO_RTIP_SIZE);
      self.outgoingPhase += DROP_SPACING/2;
      if ((self.outgoingPhase * 2) < DMA_BUF_SIZE) return true;
      else return false;
    }
    else // Cannot fit the whole drop in this buffer 
    {
      dropWrInd = remaining;
      os_memcpy(txBuf + self.outgoingPhase, &drop, remaining);
      self.outgoingPhase += DROP_SPACING/2;
      return false; // Didn't even have room for this whole drop
    }
  }
  else // self.outgoingPhase < 0
  {
    /* Unfortunately, if the drift adjustment from the RX interrupt handler has put the drop start before the start of
       this buffer, we can't go back and fix it so we put it as early as possible and keep the adjustment for the next
       drop. */
    os_memcpy(txBuf, &drop, DROP_TO_RTIP_SIZE);
    self.outgoingPhase += DROP_SPACING/2;
    return true; // Definitely have room
  }  
}

/** DMA buffer complete ISR in normal operation
 * Called for both completed SLC_TX (I2SPI receive) and SLC_RX (I2SPI transmit) events
 * @warnings ISRs cannot call printf or any radio related functions and must return in under 10us
 */
void dmaisrNormal(void* arg) {
  isrProfStart(I2SPI_ISR_PROFILE_FULL);
  isrProfStart(I2SPI_ISR_PROFILE_NORM);
  
  //Grab int status
  const uint32 slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
  //clear all intr flags
  WRITE_PERI_REG(SLC_INT_CLR, slc_intr_status);
  
  if (slc_intr_status & SLC_TX_EOF_INT_ST)
  {
    isrProfStart(I2SPI_ISR_PROFILE_RX);
    receiveCompleteHandler(); // Receive complete interrupt
    isrProfEnd(I2SPI_ISR_PROFILE_RX);
  }
  
  if (slc_intr_status & SLC_RX_EOF_INT_ST) // Transmit complete interrupt
  {
    isrProfEnd(I2SPI_ISR_PROFILE_RX);
    const uint32_t eofDesAddr = READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
    struct sdio_queue* desc = asDesc(eofDesAddr);
    uint16_t* txBuf = (uint16_t*)(desc->buf_ptr);
    const int16_t remaining = DMA_BUF_SIZE - (self.outgoingPhase*2);
    if (remaining < (int16_t)DROP_TO_RTIP_SIZE)
    {
      self.outgoingPhase -= DMA_BUF_SIZE/2; // Handle wrap around before calling make drop
    }
    while (makeDrop(txBuf));
    isrProfEnd(I2SPI_ISR_PROFILE_TX);
  }
  
  isrProfEnd(I2SPI_ISR_PROFILE_FULL);
  isrProfEnd(I2SPI_ISR_PROFILE_NORM);
}

/** DMA Buffer complete ISR for bootloader operation
 */
void dmaisrBootloader(void* arg)
{
  isrProfStart(I2SPI_ISR_PROFILE_FULL);
  isrProfStart(I2SPI_ISR_PROFILE_BOOT);

  //Grab int status
  const uint32 slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
  //clear all intr flags
  WRITE_PERI_REG(SLC_INT_CLR, slc_intr_status);
  
  // Transmit handler is more expensive so put this condition first
  isrProfStart(I2SPI_ISR_PROFILE_TX);
  if (slc_intr_status & SLC_RX_EOF_INT_ST)  // Transmit complete interrupt
  {
    const uint32_t eofDesAddr = READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
    struct sdio_queue* desc = asDesc(eofDesAddr);
    uint32_t* txBuf = (uint32_t*)(desc->buf_ptr);
    int word;
    for (word=0; word<DMA_BUF_SIZE/4; word++) 
    {
      // Use ifelse tree because faster than switch
      if (self.bootloaderCommandPhase >= 0)
      {
        txBuf[word] = self.pendingFWB[self.bootloaderCommandPhase++];
        if (self.bootloaderCommandPhase >= sizeof(FirmwareBlock)/sizeof(uint32_t))
        {
          self.bootloaderCommandPhase = BLCP_none;
          self.pendingFWB = NULL;
        }
      }
      else if (self.bootloaderCommandPhase == BLCP_flash_header)
      {
        txBuf[word] = (COMMAND_HEADER) | (COMMAND_FLASH << 16);
        self.bootloaderCommandPhase = BLCP_fwb_start;
      }
      else if (self.bootloaderCommandPhase == BLCP_command_done)
      {
        txBuf[word] = (COMMAND_HEADER) | (COMMAND_DONE << 16);
        self.bootloaderCommandPhase = BLCP_none;
        dbpc('B'); dbpc('L'); dbpc('S'); dbpc('C'); dbpc('D'); dbph(txBuf[word], 8);
      }
      else // (self.bootloaderCommandPhase == BLCP_none)
      {
        txBuf[word] = 0;
      }
    }
  }
  isrProfEnd(I2SPI_ISR_PROFILE_TX);
  
  isrProfStart(I2SPI_ISR_PROFILE_RX);
  if (slc_intr_status & SLC_TX_EOF_INT_ST) // Receive complete interrupt
  {
    const uint32_t eofDesAddr = READ_PERI_REG(SLC_TX_EOF_DES_ADDR);
    struct sdio_queue* desc = asDesc(eofDesAddr);
    const uint16_t* buf = (const uint16_t*)(desc->buf_ptr);
    const uint16_t newState = buf[DMA_BUF_SIZE/2 - 1]; // Last half-word in buffer is latest state we know
    if (self.rtipBootloaderState == STATE_BUSY && newState == STATE_IDLE) self.rtipBootloaderState = STATE_ACK;
    else self.rtipBootloaderState = newState;
    prepSdioQueue(desc, 0);
  }
  isrProfEnd(I2SPI_ISR_PROFILE_RX);

  isrProfEnd(I2SPI_ISR_PROFILE_FULL);
  isrProfEnd(I2SPI_ISR_PROFILE_BOOT);
}

/** DMA buffer complete ISR in synchronization phase
 */
void dmaisrSync(void* arg)
{
  isrProfStart(I2SPI_ISR_PROFILE_SYNC);
  
  //Grab int status
  const uint32 slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
  //clear all intr flags
  WRITE_PERI_REG(SLC_INT_CLR, slc_intr_status);
  
  isrProfStart(I2SPI_ISR_PROFILE_RX);
  // Receive handler is more expensive so put first
  if (slc_intr_status & SLC_TX_EOF_INT_ST) // Receive complete interrupt
  {
    uint32_t eofDesAddr = READ_PERI_REG(SLC_TX_EOF_DES_ADDR);
    struct sdio_queue* desc = asDesc(eofDesAddr);
    uint16_t* buf = (uint16_t*)(desc->buf_ptr);
    int i;
    int16_t dp = 0;
    for (i=(DMA_BUF_SIZE/2)-1; i>=0; --i) // Search backwards to find the last one
    {
      if (buf[i] == TO_WIFI_PREAMBLE)
      {
        dp = i;
        break;
      }
    }
    if (((dp * 2) + DROP_SPACING) > DMA_BUF_SIZE) // Had a buffer full of drops
    {
      self.outgoingPhase = dp + DROP_TX_PHASE_ADJUST;
      if (self.outgoingPhase > (DMA_BUF_SIZE/2)) self.outgoingPhase -= DMA_BUF_SIZE/2; // Handle wrap around
      dp += (DROP_SPACING/2) - DRIFT_MARGIN;
      if (dp < (DMA_BUF_SIZE/2)) dp += DROP_SPACING/2; // Make sure we're looking in the next buffer
      self.dropPhase = dp - (DMA_BUF_SIZE/2); // Set up to look in the next buffer
      foregroundTaskPost(i2spiSynchronizedCallback, self.dropPhase);
      i2spiSwitchMode(I2SPI_NORMAL);
    }
    else if (buf[DMA_BUF_SIZE/2] == STATE_IDLE) // Did we find a boot loader?
    {
      i2spiSwitchMode(I2SPI_BOOTLOADER);
      self.rtipBootloaderState = STATE_IDLE;
    }
    prepSdioQueue(desc, 0);
  }
  isrProfEnd(I2SPI_ISR_PROFILE_RX);

  isrProfStart(I2SPI_ISR_PROFILE_TX);
  if (slc_intr_status & SLC_RX_EOF_INT_ST)  // Transmit complete interrupt
  {
    const uint32_t eofDesAddr = READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
    struct sdio_queue* desc = asDesc(eofDesAddr);
    uint32_t* buf = (uint32_t*)(desc->buf_ptr);
    int word;
    for (word=0; word<DMA_BUF_SIZE/4; word++) buf[word] = 0x80000000; // Just send out sync pattern
  }
  isrProfEnd(I2SPI_ISR_PROFILE_TX);
  
  isrProfEnd(I2SPI_ISR_PROFILE_SYNC);
}

//Initialize I2S subsystem for DMA circular buffer use
int8_t ICACHE_FLASH_ATTR i2spiInit() {
  int i;

  system_os_task(sendImageTask, IMAGE_SEND_TASK_PRIO, imageSendTaskQueue, IMAGE_SEND_TASK_QUEUE_DEPTH);

  os_memset(&self, 0, sizeof(I2SpiDriver));
  self.pendingFWB        = NULL;
  self.rtipBootloaderState    = STATE_UNKNOWN;
  self.bootloaderCommandPhase = BLCP_none;
  
  os_memset(&audio,  0, sizeof(AudioBufferState));
  os_memset(&screen, 0, sizeof(ScreenBufferState));
  os_memset(&imageBuffers, 0, sizeof(ImageDataBuffer)*NUM_IMAGE_DATA_BUFFERS);
  
  os_memset(messageBuffer, 0, I2SPI_MESSAGE_BUF_SIZE);
  os_memset(relayBuffer,   0, RELAY_BUFFER_SIZE);
  os_memset(audioBuffer,   0, AUDIO_BUFFER_SIZE);
  
  os_memset(txBufs, 0xff, DMA_BUF_COUNT*DMA_BUF_SIZE);
  os_memset(rxBufs, 0xff, DMA_BUF_COUNT*DMA_BUF_SIZE);
  
  // Setup the buffers and descriptors
  for (i=0; i<DMA_BUF_COUNT; ++i)
  {
    prepSdioQueue(&txQueue[i], 1);
    prepSdioQueue(&rxQueue[i], 0);
    txQueue[i].buf_ptr = (uint32_t)&txBufs[i];
    txQueue[i].next_link_ptr = (int)((i<(DMA_BUF_COUNT-1))?(&txQueue[i+1]):(&txQueue[0]));
    //os_printf("TX Q 0x%08x\tB 0x%08x\t",  (unsigned int)&txQueue[i], txQueue[i].buf_ptr);
    rxQueue[i].buf_ptr = (uint32_t)&rxBufs[i];
    rxQueue[i].next_link_ptr = (int)((i<(DMA_BUF_COUNT-1))?(&rxQueue[i+1]):(&rxQueue[0]));
    //os_printf("RX Q 0x%08x\tB 0x%08x\r\n",  (unsigned int)&rxQueue[i], rxQueue[i].buf_ptr);
  }

  //Reset DMA
  SET_PERI_REG_MASK(  SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);
  CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);

  //Clear DMA int flags
  SET_PERI_REG_MASK(  SLC_INT_CLR,  0xffffffff);
  CLEAR_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);

  //Enable and configure DMA
  CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
  SET_PERI_REG_MASK(  SLC_CONF0, (1<<SLC_MODE_S));
  SET_PERI_REG_MASK(  SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
  CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN       | SLC_RX_EOF_MODE     | SLC_RX_FILL_MODE);

  /* Feed DMA the buffer descriptors
     TX and RX are from the DMA's perspective so we use the TX LINK part to receive data from the I2S DMA and the RX LINK
     part to send data out the DMA to I2S.
  */
  CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
  SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&rxQueue[0])  & SLC_TXLINK_DESCADDR_MASK);
  CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
  SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&txQueue[0]) & SLC_RXLINK_DESCADDR_MASK);

  //Attach the DMA interrupt
  ets_isr_attach(ETS_SLC_INUM, dmaisrSync, NULL);
  //Enable DMA operation intr Want to get interrupts for both end of transmits
  WRITE_PERI_REG(SLC_INT_ENA,  SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA | SLC_RX_UDF_INT_ENA | SLC_TX_DSCR_ERR_INT_ENA);
  //clear any interrupt flags that are set
  WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
  ///enable DMA intr in cpu
  ets_isr_unmask(1<<ETS_SLC_INUM);

  // Start DMA transmitting
  SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
  // Start DMA receiving
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

//----

  //Init pins to i2s functions
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,  FUNC_I2SO_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,  FUNC_I2SI_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,  FUNC_I2SI_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U , FUNC_I2SI_WS);

#if I2SPI_ISR_PROFILING
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, 1<<PROFILING_PIN);
#endif

  //Enable clock to i2s subsystem
  i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

  //Reset I2S subsystem
  CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
  CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);

  //Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
  CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN|(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
  //Enable DMA in i2s subsystem
  SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);
  
  //set I2S_CHAN 
  //set rx,tx channel mode, both are "two channel" here
  SET_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

  //set RX eof num
  WRITE_PERI_REG(I2SRXEOF_NUM, 128);
  
  // Tweak the I2S timing
  CLEAR_PERI_REG_MASK(I2STIMING, (I2S_RECE_SD_IN_DELAY    << I2S_RECE_SD_IN_DELAY_S)   |
                                 (I2S_RECE_WS_IN_DELAY    << I2S_RECE_WS_IN_DELAY_S)   |
                                 (I2S_RECE_BCK_IN_DELAY   << I2S_RECE_BCK_IN_DELAY_S)  |
                                 (I2S_TRANS_SD_OUT_DELAY  << I2S_TRANS_SD_OUT_DELAY_S) |
                                 (I2S_TRANS_WS_OUT_DELAY  << I2S_TRANS_WS_IN_DELAY_S)  |
                                 (I2S_TRANS_BCK_OUT_DELAY << I2S_TRANS_BCK_IN_DELAY_S));

  SET_PERI_REG_MASK(I2STIMING, ((0 & I2S_RECE_SD_IN_DELAY)    << I2S_RECE_SD_IN_DELAY_S)   |
                               ((0 & I2S_RECE_WS_IN_DELAY)    << I2S_RECE_WS_IN_DELAY_S)   |
                               ((0 & I2S_RECE_BCK_IN_DELAY)   << I2S_RECE_BCK_IN_DELAY_S)  |
                               ((0 & I2S_TRANS_SD_OUT_DELAY)  << I2S_TRANS_SD_OUT_DELAY_S) |
                               ((0 & I2S_TRANS_WS_OUT_DELAY)  << I2S_TRANS_WS_IN_DELAY_S)  |
                               ((0 & I2S_TRANS_BCK_OUT_DELAY) << I2S_TRANS_BCK_IN_DELAY_S));// | I2S_TRANS_BCK_IN_INV);// | I2S_TRANS_DSYNC_SW);
  //trans master&rece slave,MSB shift,right_first,msb right
  CLEAR_PERI_REG_MASK(I2SCONF, I2S_RECE_SLAVE_MOD|I2S_TRANS_SLAVE_MOD|
                   (I2S_BITS_MOD<<I2S_BITS_MOD_S)|
                   (I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
                   (I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
  SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|/*I2S_MSB_RIGHT|*/I2S_RECE_SLAVE_MOD|//I2S_TRANS_SLAVE_MOD|
                             I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT |
                             (( 4&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)| // Clock counter, must be a multiple of 2 for 50% duty cycle
                             (( 4&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)| // Clock prescaler
                             (  0<<I2S_BITS_MOD_S)); // Dead bit insertion?

  //clear int
  SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
                                  I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
  CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
                                  I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
 
  //enable int
  SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|I2S_I2S_RX_REMPTY_INT_ENA|
  I2S_I2S_RX_WFULL_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

  // Start
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START | I2S_I2S_RX_START);

  return 0;
}

bool ICACHE_FLASH_ATTR i2spiQueueMessage(const uint8_t* msgData, const int msgLen)
{
  const uint16_t rind = self.messageBufferRind;
  uint16_t wind = self.messageBufferWind;
  if (unlikely(self.mode != I2SPI_NORMAL))
  {
    return false;
  }
  else if (unlikely(msgLen > DROP_TO_RTIP_MAX_VAR_PAYLOAD))
  {
    os_printf("ERROR i2spiQueueMessage(%02x..., %d) bigger than limit %d\r\n", msgData[0], msgLen, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
    return false;
  }
  else
  {
    const uint16_t available = I2SPI_MESSAGE_BUF_SIZE - ((wind - rind) & I2SPI_MESSAGE_BUF_SIZE_MASK);
    if (unlikely((msgLen + 1) >= available)) // Leave space for length field and to prevent index overlap
    {
      return false;
    }
    else
    {
      messageBuffer[wind] = msgLen;
      wind = (wind + 1) & I2SPI_MESSAGE_BUF_SIZE_MASK; // Advance past size field
      const int firstCopy = I2SPI_MESSAGE_BUF_SIZE - wind;
      if (unlikely(firstCopy < msgLen))
      {
        os_memcpy(messageBuffer + wind, msgData, firstCopy);
        os_memcpy(messageBuffer, msgData + firstCopy, msgLen - firstCopy);
      }
      else
      {
        os_memcpy(messageBuffer + wind, msgData, msgLen);
      }
      self.messageBufferWind = (wind + msgLen) & I2SPI_MESSAGE_BUF_SIZE_MASK; // Advance the write index
      return true;
    }
  }
}

bool ICACHE_FLASH_ATTR i2spiMessageQueueIsEmpty(void)
{
  return self.messageBufferRind == self.messageBufferWind;
}

int ICACHE_FLASH_ATTR i2spiGetCladMessage(uint8_t* data)
{
  const uint16_t wind = self.rtipBufferWind;
  uint16_t rind = self.rtipBufferRind;
  if (wind == rind) return 0; // Don't have anything
  else
  {
    const int available = RELAY_BUFFER_SIZE - ((rind - wind) & RELAY_BUFFER_SIZE_MASK);
    const uint8_t size = relayBuffer[rind];
    const uint8_t sizeWHeader = size + 1;
    if (available < sizeWHeader) return 0;
    else
    {
      rind = (rind + 1) & RELAY_BUFFER_SIZE_MASK;
      const int firstCopy = RELAY_BUFFER_SIZE - rind;
      if (unlikely(firstCopy < size))
      {
        os_memcpy(data, relayBuffer + rind, firstCopy);
        os_memcpy(data + firstCopy, relayBuffer, size - firstCopy);
      }
      else
      {
        os_memcpy(data, relayBuffer + rind, size);
      }
      self.rtipBufferRind = (rind + size) & RELAY_BUFFER_SIZE_MASK;
      return size;
    }
  }
}

int16_t ICACHE_FLASH_ATTR i2spiGetRtipBootloaderState(void)
{
  if (self.mode == I2SPI_NORMAL) return STATE_RUNNING; 
  else return self.rtipBootloaderState;
}

bool ICACHE_FLASH_ATTR i2spiBootloaderPushChunk(FirmwareBlock* chunk)
{
  if (self.bootloaderCommandPhase == BLCP_none)
  {
    self.pendingFWB = (uint32_t*)chunk;
    self.bootloaderCommandPhase = BLCP_flash_header;
    return true;
  }
  else return false;
}

bool ICACHE_FLASH_ATTR i2spiBootloaderCommandDone(void)
{
  if (self.bootloaderCommandPhase == BLCP_none)
  {
    self.bootloaderCommandPhase = BLCP_command_done;
    return true;
  }
  else return false;
}

bool i2spiSwitchMode(const I2SPIMode mode)
{
  if (mode == I2SPI_SYNC)
  {
    self.mode = I2SPI_SYNC;
    self.rtipBootloaderState = STATE_UNKNOWN;
    ets_isr_attach(ETS_SLC_INUM, dmaisrSync, NULL);
    return true;
  }
  else if (self.mode != I2SPI_SYNC) return false;
  else
  {
    if (mode == I2SPI_NORMAL)
    {
      self.mode = I2SPI_NORMAL;
      ets_isr_attach(ETS_SLC_INUM, dmaisrNormal, NULL);
      return true;
    }
    else if (mode == I2SPI_BOOTLOADER)
    {
      self.mode = I2SPI_BOOTLOADER;
      ets_isr_attach(ETS_SLC_INUM, dmaisrBootloader, NULL);
      return true;
    }
    // Invalid state
    return false;
  }
}

uint32_t i2spiGetTxOverflowCount(void) { return self.txOverflowCount; }
uint32_t i2spiGetRxOverflowCount(void) { return self.rxOverflowCount; }
uint32_t i2spiGetPhaseErrorCount(void) { return self.phaseErrorCount; }
 int32_t i2spiGetIntegralDrift(void)   { return self.integralDrift;   }
