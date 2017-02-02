/** Implementation of robot NVStorage using HAL
 * @author Daniel Casner <daniel@anki.com>
 */

#include "nvStorage.h"
#include "anki/cozmo/robot/logging.h"
#ifdef TARGET_ESPRESSIF
  extern "C" {
    #include "mem.h"
    #include "client.h"
  }
  #define FIXTURE_DATA_NUM_ENTRIES (16)
  #define FIXTURE_DATA_ADDRESS_MASK (FIXTURE_DATA_NUM_ENTRIES - 1)
  #define FIXTURE_DATA_READ_SIZE (1024)
  #include "anki/cozmo/robot/esp.h"
  #include "anki/cozmo/transport/reliableTransport.h"
  #include "clad/types/birthCertificate.h"
#else 
  #include <cstdlib>
  #include "../sim_hal/sim_nvStorage.h"
#endif

#define DEBUG_NVS 0

#if DEBUG_NVS > 0
  #ifdef TARGET_ESPRESSIF
    #define db_printf(...) os_printf(__VA_ARGS__)
  #else
    #define db_printf(...) printf(__VA_ARGS__)
  #endif
#else
  #define db_printf(...)
#endif


namespace Anki {
namespace Cozmo {
namespace NVStorage {

/// Structure for storing command in process
struct CommandState
{
  NVCommand cmd; ///< Command being processed
  NVOperationCallback callback; ///< Callback for when complete
  uint32_t index; ///< Where we are operating in flash right now
  int16_t  loopCount; ///< How many update loops we've used
  int8_t   retries; ///< How many times we've retried a flash operation
};

#define INVALID_FLASH_ADDRESS (0xFFFFffff)
#define MAX_FLASH_OP_RETRIES (3)
#define MAX_LOOPS (8000)
#define MAX_READ_SIZE (1024)

static CommandState* state;


#ifdef TARGET_ESPRESSIF
 #define PacketBufferHasRoom() clientCanTransmit(MAX_READ_SIZE)
 #define PlatformGetFixtureData(self) EspressifGetFixtureData(self)
 #define PlatformGetFactoryData(self) EspressifGetFactoryData(self)
#elif defined(SIMULATOR)
 #define PacketBufferHasRoom() true
 #define PlatformGetFixtureData(self) Complete((self), NV_NOT_FOUND); //no support in Simulator
 #define PlatformGetFactoryData(self) SimulatorGetFactoryData(self)
#else  
 #error No factory nvstorage read defined for this build TARGET_ESPRESSIF
#endif
  

static inline bool isReadBlocked()
{
  if (!HAL::RadioIsConnected()) return false;
  else if (!PacketBufferHasRoom() ) return true;
  else if ((int)HAL::RadioQueueAvailable() < ((int)sizeof(Anki::Cozmo::NVStorage::NVOpResult) + 10))
  {
#if DEBUG_NVS > 1
    db_printf("Client Q blocked\r\n");
#endif
    return true;
  }
  else return false;
}

/// Protected callback function to be used for erase and write, not for read
static void Complete(CommandState* self, NVResult result)
{
  if (self->callback != NULL)
  {
    NVOpResult msg;
    msg.address   = self->cmd.address;
    msg.offset    = self->index;
    msg.operation = self->cmd.operation;
    msg.result    = result;
    msg.blob_length = 0;
    db_printf("Complete: addr: %x\tofst: %x\toper: %x\trslt: %d\r\n", msg.address, msg.offset, msg.operation, msg.result);
    self->callback(msg);
  }
  
  // When operating normally
  if (self == state)
  {
    db_printf("Reseting\r\n");
    free(state);
    state = NULL;
  }
}

#ifdef TARGET_ESPRESSIF
static inline void EspressifGetFixtureData(CommandState* self)
{
  db_printf("\tFixture Data\r\n");
  if (self->index == INVALID_FLASH_ADDRESS)
  { // First time through, validate arguments
    const u32 tag = self->cmd.address & (~NVConst_FIXTURE_DATA_BIT);
    if (tag >= FIXTURE_DATA_NUM_ENTRIES) Complete(self, NV_NOT_FOUND);
    else if (self->cmd.length < 1) Complete(self, NV_BAD_ARGS);
    else 
    {
      if (tag + self->cmd.length > FIXTURE_DATA_NUM_ENTRIES) self->cmd.length = FIXTURE_DATA_NUM_ENTRIES - tag;
      self->index = 0;
      self->retries = 0;
    }
  }
  else
  {
    const u32 remaining = self->cmd.length - self->index;
    const u32 readAddress = (FIXTURE_STORAGE_SECTOR * SECTOR_SIZE) + (self->index * FIXTURE_DATA_READ_SIZE) + (self->cmd.address & FIXTURE_DATA_ADDRESS_MASK);
    NVOpResult msg;
    msg.address     = self->cmd.address;
    msg.operation   = self->cmd.operation;
    msg.offset      = self->index;
    msg.blob_length = 0;
    msg.result = HAL::FlashRead(readAddress, reinterpret_cast<u32*>(msg.blob), FIXTURE_DATA_READ_SIZE);
    if (msg.result != NV_OKAY)
    {
      if (self->retries++ >= MAX_FLASH_OP_RETRIES)
      {
        self->loopCount = 0;
        if (self == state)
        {
          free(state);
          state = NULL;
        }
      }
    }
    else
    {
      msg.blob_length = FIXTURE_DATA_READ_SIZE;
      db_printf("\tcallback\r\n");
      if (remaining > 1)
      {
        msg.result = NV_MORE;
        self->callback(msg);
        self->index++;
      }
      else
      {
        self->callback(msg);
        if (self == state)
        {
          free(state);
          state = NULL;
        }
      }
    }
  }
}

struct FactoryNVEntryHeader {
  u32 size;      ///< Size of the entry data plus this header
  u32 tag;       ///< Data identification for this structure
  u32 successor; /**< Left 0xFFFFffff when written, set to 0x00000000 for deletion or written address of successor if
                      replaced. @warning MUST be the last member of the struct. */
};
//Each entry is followed by a footer word which is set to 0 when valid
typedef u32 FactoryNVEntryFooter;

#define FOOTER(header) (((u32*)(header))[(header->size/sizeof(u32)) - 1]) //make sure size is validated before using this

#define MAX_ENTRY_SIZE (sizeof(FactoryNVEntryHeader) + MAX_READ_SIZE + sizeof(FactoryNVEntryFooter))

static inline void EspressifGetFactoryData(CommandState* self)
{
  static u8 numFound;
  db_printf("\tFactoryData\r\n");
  if (self->index == INVALID_FLASH_ADDRESS)
  { // First pass through, initalizing
    self->index = FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE;
    numFound = 0;
  }
  else if (self->index >= (FIXTURE_STORAGE_SECTOR * SECTOR_SIZE))
  {
    self->index = 0;
    Complete(self, numFound > 0 ? NV_OKAY : NV_NOT_FOUND);
  }
  else
  {
    u32 readBuffer[MAX_ENTRY_SIZE/sizeof(u32)]; // Big enough to read largest possible entry + header and footer
    FactoryNVEntryHeader* header = reinterpret_cast<FactoryNVEntryHeader*>(readBuffer);
    s8 retries = 0;
    NVResult rslt;
    do
    {
      rslt = HAL::FlashRead(self->index, readBuffer, sizeof(readBuffer));
      db_printf("NV Read: ind %x\trst: %x\ttag=%x\tsize=%x\tsucc=%x\t\r\n", state->index, rslt, header->tag, header->size, header->successor);
      if (retries++ > MAX_FLASH_OP_RETRIES)
      {
        Complete(self, rslt);
        return;
      }
    } while (rslt != NV_OKAY);
    if (header->tag == NVEntry_Invalid)
    {
      self->index = 0;
      Complete(self, numFound > 0 ? NV_OKAY : NV_NOT_FOUND);
    }
    else if ((header->size < sizeof(FactoryNVEntryHeader)+sizeof(FactoryNVEntryFooter)) ||
             header->size > MAX_ENTRY_SIZE)
    {
      self->index = 0;
      Complete(self, NV_CORRUPT);
    }
    else if ((header->successor != 0xFFFFffff) || FOOTER(header) != 0)
    { // Entry is invalid, advance to next one
      self->index += header->size;
    }
    else if ((header->tag >= self->cmd.address) && (header->tag < (self->cmd.address + self->cmd.length)))
    { // Valid entry that we want
      NVOpResult msg;
      msg.address   = self->cmd.address;
      msg.offset    = header->tag - self->cmd.address;
      msg.operation = self->cmd.operation;
      msg.result    = self->cmd.length > 1 ? NV_MORE : NV_OKAY;
      msg.blob_length = header->size - sizeof(FactoryNVEntryHeader) - sizeof(FactoryNVEntryFooter);
      memcpy(msg.blob, readBuffer + (sizeof(FactoryNVEntryHeader)/sizeof(u32)), msg.blob_length);
      self->callback(msg);
      db_printf("Callback done\r\n");
      self->loopCount = 0;
      if (self->cmd.length == 1)
      { // If we were only reading one, we're done
        if (self == state)
        {
          free(state);
          state = NULL;
          db_printf("Freed\r\n");
        }
      }
      else
      {
        numFound++;
        self->index += header->size;
      }
    }
    else
    { // Valid entry but not what we wanted
      self->index += header->size;
    }
  }
}
#endif


#if FACTORY_BIRTH_CERTIFICATE_CHECK_ENABLED
#define INVALID_BIRTH_SECOND (0x81)

static bool hasValidBirthCertificate;
extern "C" void NVStorage_ReadBirthCertificate(void) {

  struct BirthCertificateEnvelope {
    struct FactoryNVEntryHeader magicHeader;
    u32 factoryMagic;
    FactoryNVEntryFooter magicFooter;
    struct FactoryNVEntryHeader certificateHeader;
    BirthCertificate certificate;
    FactoryNVEntryFooter certificateFooter;
  } bcEnvelope;
  if (spi_flash_read((FACTORY_NV_STORAGE_SECTOR*SECTOR_SIZE), reinterpret_cast<u32*>(&bcEnvelope), sizeof(bcEnvelope)) == SPI_FLASH_RESULT_OK) {
    if (bcEnvelope.magicHeader.tag == NVEntry_VersionMagic &&
        bcEnvelope.factoryMagic >= NVConst_VERSION_MAGIC &&
        bcEnvelope.certificateHeader.tag == NVEntry_BirthCertificate &&
        bcEnvelope.certificate.second != INVALID_BIRTH_SECOND &&
        bcEnvelope.certificateHeader.successor == 0xFFFFffff)
    {
      hasValidBirthCertificate = true;  //ALL GOOD
    }
  }
  else {
    hasValidBirthCertificate = false;
  }
}

extern "C" bool hasBirthCertificate(void)
{
  return hasValidBirthCertificate;
}
#else
extern "C" bool hasBirthCertificate(void) { return true;  }  //failure not allowed
#endif

#ifdef SIMULATOR
static inline void SimulatorGetFactoryData(CommandState* self)
{
  if (self->cmd.address != NVEntry_CameraCalib)
  {
    Complete(self, NV_NOT_FOUND); // CameraCalib is only type supported
    return;
  }
  
  // Store camera calibration in nvStorage
  const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();
  if(headCamInfo == NULL) {
    AnkiWarn( 163, "nvstorage.simulator_get_camera_calib.calib_not_found", 359, "NULL HeadCamInfo retrieved from HAL.", 0);
    Complete(self, NV_NOT_FOUND);
  }
  else if(self->callback) {
    CameraCalibration headCalib{
      headCamInfo->focalLength_x,
      headCamInfo->focalLength_y,
      headCamInfo->center_x,
      headCamInfo->center_y,
      headCamInfo->skew,
      headCamInfo->nrows,
      headCamInfo->ncols
    };
    
    for(s32 iCoeff=0; iCoeff<NUM_RADIAL_DISTORTION_COEFFS; ++iCoeff) {
      headCalib.distCoeffs[iCoeff] = headCamInfo->distortionCoeffs[iCoeff];
    }
    
    NVOpResult msg;
    msg.address   = self->cmd.address;
    msg.operation = self->cmd.operation;
    msg.offset    = 0;
    msg.result    = NV_OKAY;
    memcpy(msg.blob, headCalib.GetBuffer(), headCalib.Size());
    msg.blob_length = headCalib.Size();
    self->callback(msg);
    
    if (self == state)
    {
      free(state);
      state = NULL;
    }
  }
}
#endif

static inline Result Erase(CommandState* self)
{
  if (self->index == INVALID_FLASH_ADDRESS)
  { // First time through, validate arguments
    if (self->cmd.operation == NVOP_WIPEALL)
    {
      self->cmd.address = NVConst_MIN_ADDRESS;
      self->cmd.length  = NVConst_MAX_ADDRESS - NVConst_MIN_ADDRESS;
      self->index = 0;
      self->retries = 0;
    }
    // Ensure erase starts on a sector boundary
    else if ((self->cmd.address & SECTOR_MASK) != 0) { Complete(self, NV_BAD_ARGS); }
    // Ensure erase is a whole number of sectors
    else if ((self->cmd.length  & SECTOR_MASK) != 0) { Complete(self, NV_BAD_ARGS); }
    // Ensure all data is in allowed region
    else if (! HAL::FlashWriteOkay(self->cmd.address, self->cmd.length) ) { Complete(self, NV_BAD_ARGS); }
    else
    {
      self->index = 0;
      self->retries = 0;
    }
  }
  else if ((self->index * SECTOR_SIZE) >= self->cmd.length)
  { // Reached the end of the erase
    Complete(self, NV_OKAY);
  }
  else
  { // Still erasing
    u32 eraseAddress = self->cmd.address + (self->index * SECTOR_SIZE);
    const NVResult rslt = HAL::FlashErase(eraseAddress);
    if (rslt != NV_OKAY && rslt != NV_BAD_ARGS) // Skip over unerasable sectors
    {
      if (self->retries++ >= MAX_FLASH_OP_RETRIES)
      {
        Complete(self, rslt);
      }
    }
    else
    {
      self->index++;
      self->retries = 0;
    }
  }
  return RESULT_OK;
}

static inline Result Write(CommandState* self)
{
  if (self->cmd.blob_length > MAX_READ_SIZE) Complete(self, NV_BAD_ARGS);
  else
  {
    u32 writeAddress = self->cmd.address;
    const NVResult rslt = HAL::FlashWrite(writeAddress, reinterpret_cast<u32*>(self->cmd.blob), self->cmd.blob_length);
    if (rslt != NV_OKAY)
    {
      if (self->retries++ >= MAX_FLASH_OP_RETRIES)
      {
        Complete(self, rslt);
      }
    }
    else Complete(self, NV_OKAY);
  }
  return RESULT_OK;
}
  
  
static inline Result Read(CommandState* self)
{
  if (isReadBlocked()) return RESULT_OK;
  
  if (self->callback == NULL) return RESULT_FAIL_INVALID_PARAMETER;
  
  if (self->cmd.address & NVConst_FACTORY_DATA_BIT)
  {
    if ((self->cmd.address & NVConst_FIXTURE_DATA_BIT) == NVConst_FIXTURE_DATA_BIT)
    {
      PlatformGetFixtureData(self);
    }
    else // Factory data
    {
      PlatformGetFactoryData(self);
    }
  }
  else // Done with special cases
  {
    if (self->index == INVALID_FLASH_ADDRESS)
    { // First time through, validate arguments
      if (self->cmd.address < NV_STORAGE_START_ADDRESS) Complete(self, NV_BAD_ARGS);
      else if ((self->cmd.address + self->cmd.length) > NV_STORAGE_END_ADDRESS) Complete(self, NV_BAD_ARGS);
      else
      {
        self->index = 0;
      }
    }
    else
    {
      NVOpResult msg;
      u32 readAddr  = self->cmd.address + (self->index * MAX_READ_SIZE);
      const s32 remaining = (s32)self->cmd.length - (s32)(self->index * MAX_READ_SIZE);
      const s32 readLen   = MIN(remaining, NV_STORAGE_CHUNK_SIZE);
      msg.address     = self->cmd.address;
      msg.operation   = self->cmd.operation;
      msg.offset      = self->index;
      msg.blob_length = 0;
      msg.result = HAL::FlashRead(readAddr, reinterpret_cast<u32*>(msg.blob), readLen);
      self->loopCount = 0;
      if (msg.result != NV_OKAY)
      {
        if (self->retries++ >= MAX_FLASH_OP_RETRIES)
        {
          self->callback(msg);
          if (self == state)
          {
            free(state);
            state = NULL;
          }
        }
      }
      else
      {
        msg.blob_length = readLen;
        if (remaining > NV_STORAGE_CHUNK_SIZE)
        {
          msg.result = NV_MORE;
          self->callback(msg);
          self->index++;
        }
        else
        {
          self->callback(msg);
          if (self == state)
          {
            free(state);
            state = NULL;
          }
        }
      }
    }
  }
  return RESULT_OK;
}

bool Init()
{
  HAL::FlashInit();
  state = NULL;
  return true;
}

Result Update()
{
  if (state != NULL) 
  {
    if (state->loopCount++ > MAX_LOOPS)
    {
      Complete(state, NV_LOOP);
    }
    else
    {
      db_printf("NV Update: addr %x\tlen %u\top %x\tind %x\tret: %x\r\n", state->cmd.address, state->cmd.length, state->cmd.operation, state->index, state->loopCount);
      switch (state->cmd.operation)
      {
        case NVOP_READ:
          return Read(state);
        case NVOP_WRITE:
          return Write(state);
        case NVOP_ERASE:
        case NVOP_WIPEALL:
          return Erase(state);
        default:
          AnkiWarn( 375, "nvstorage.invalid_operation", 611, "operation=%d", 1, state->cmd.operation);
          return RESULT_FAIL_INVALID_PARAMETER;
      }
    }
  }
  return RESULT_OK;
}

NVResult Command(const NVCommand& cmd, NVOperationCallback callback)
{
  // Are we already busy with a previous command?
  if (state != NULL) return NV_BUSY;
  else
  {
    state = reinterpret_cast<CommandState*>(malloc(sizeof(CommandState)));
    if (state == NULL) return NV_NO_MEM;
    else
    {
      memcpy(&(state->cmd), &cmd, sizeof(NVCommand));
      state->callback  = callback;
      state->index     = INVALID_FLASH_ADDRESS;
      state->loopCount = 0;
      state->retries   = 0;
      return NV_SCHEDULED;
    }
  }
}

} // Namespace NVStorage
} // Namespace Cozmo
} // Namespace Anki
