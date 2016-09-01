/** Implementation for non-volatile storage on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 *
 * Theory of operation:
 * All stored data is stored as entries each of which has an NVEntryHeader followed by the entry data. The header is
 * used for retrieving entries and for storing the status of entries.
 *
 */

#include "nvStorage.h"
extern "C" {
#include "osapi.h"
#include "mem.h"
#include "client.h"
#include "foregroundTask.h"
#include "driver/i2spi.h"
}
#include "face.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/esp.h"

#define READ_BACKOFF_INTERVAL_us (1000000)

#define FACTORY_DATA_BIT (0x80000000)
#define FIXTURE_DATA_BIT (0xC0000000)
#define FIXTURE_DATA_NUM_ENTRIES (16)
#define FIXTURE_DATA_ADDRESS_MASK (FIXTURE_DATA_NUM_ENTRIES - 1)
#define FIXTURE_DATA_READ_SIZE (1024)

#define DEBUG_NVS 0

#if DEBUG_NVS > 0
#define db_printf(...) os_printf(__VA_ARGS__)
#else
#define db_printf(...)
#endif

namespace Anki {
namespace Cozmo {
namespace NVStorage {

typedef enum {
  NV_SEGMENT_A = 0,
  NV_SEGMENT_B = 1,
  NV_SEGMENT_F = 0x20,
  NV_SEGMENT_X = 0x40,
  NV_SEGMENT_UNINITALIZED = -128
} NVSegment;

struct NVEntryHeader {
  u32 size;      ///< Size of the entry data plus this header
  u32 tag;       ///< Data identification for this structure
  u32 successor; /**< Left 0xFFFFffff when written, set to 0x00000000 for deletion or written address of successor if
                      replaced. @warning MUST be the last member of the struct. */
};

#define MAX_ENTRY_SIZE (sizeof(NVEntryHeader) + 1024 + 4)

struct NVStorageState {
  NVStorageBlob* pendingWrite;
  WriteDoneCB    pendingWriteDoneCallback;
  u32            pendingOverwriteHeaderAddress; // The header address of an entry that needs to be overwritten
  u32            pendingEraseStart; // Lowest tag we are searching to erase
  u32            pendingEraseEnd; // Highest tag we are searching to erase
  EraseDoneCB    pendingEraseCallback;
  MultiOpDoneCB  pendingMultiEraseDoneCallback;
  u32            pendingReadStart; // Lowest tag we are searching to read
  u32            pendingReadEnd; // Highest tag we are searching to read
  ReadDoneCB     pendingReadCallback;
  MultiOpDoneCB  pendingMultiReadDoneCallback;
  s32            flashPointer;
  s8             phase;
  s8             retries;
  s8             segment;
  bool           multiEraseDidAny;
  bool           multiReadFoundAny;
  bool           waitingForGC;
};

static NVStorageState nv;

static s32 getStartOfSegment(void)
{
  if (nv.segment & NV_SEGMENT_X) return FIXTURE_STORAGE_SECTOR * SECTOR_SIZE;
  else if (nv.segment & NV_SEGMENT_F) return FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE;
  else
  {
    int segment = nv.segment & (NV_STORAGE_NUM_AREAS-1); // Mask for safety
    return NV_STORAGE_START_ADDRESS + (NV_STORAGE_AREA_SIZE * segment) + sizeof(NVDataAreaHeader);
  }
}

static inline s32 getEndOfSegment(void)
{
  return getStartOfSegment() + NV_STORAGE_CAPACITY;
}

static void printNVSS()
{
  db_printf("pw  = %x\tpwdc = %x\tpoha = %x\r\n"
            "pes = %d\tpee  = %d\tpec  = %x\tpmedc = %x\r\n"
            "prs = %d\tpre  = %d\tprc  = %x\tpmrdc = %x\r\n"
            "fp  = %x\tseg  = %x\tsos  = %x\teos   = %x\r\n"
            "p   = %d\tr    = %d\tmeda = %x\tmrfa = %x\r\n\r\n",
  (u32)nv.pendingWrite, (u32)nv.pendingWriteDoneCallback, nv.pendingOverwriteHeaderAddress,
  nv.pendingEraseStart, nv.pendingEraseEnd, (u32)nv.pendingEraseCallback, (u32)nv.pendingMultiEraseDoneCallback,
  nv.pendingReadStart, nv.pendingReadEnd, (u32)nv.pendingReadCallback, (u32)nv.pendingMultiReadDoneCallback,
  nv.flashPointer, nv.segment, getStartOfSegment(), getEndOfSegment(),
  nv.phase, nv.retries, nv.multiEraseDidAny, nv.multiReadFoundAny);
}

#define FLASH_OP_RETRIES_BEFORE_FAIL 10

static void resetWrite()
{
  if (nv.pendingWrite != NULL) os_free(nv.pendingWrite);
  nv.pendingWrite = NULL;
  nv.pendingWriteDoneCallback = NULL;
  nv.pendingOverwriteHeaderAddress = 0;
  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
  nv.phase = 0;
  nv.segment &= ~NV_SEGMENT_F & ~NV_SEGMENT_X;
  nv.flashPointer = getStartOfSegment();
}

static void resetErase()
{
  nv.pendingEraseStart = NVEntry_Invalid;
  nv.pendingEraseEnd   = NVEntry_Invalid;
  nv.pendingEraseCallback = NULL;
  nv.pendingMultiEraseDoneCallback = NULL;
  nv.multiEraseDidAny = false;
  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
  nv.phase = 0;
  nv.segment &= ~NV_SEGMENT_F & ~NV_SEGMENT_X;
  nv.flashPointer = getStartOfSegment();
}

static void resetRead()
{
  nv.pendingReadStart = NVEntry_Invalid;
  nv.pendingReadEnd   = NVEntry_Invalid;
  nv.pendingReadCallback = NULL;
  nv.pendingMultiReadDoneCallback = NULL;
  nv.multiReadFoundAny = false;
  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
  nv.phase = 0;
  nv.segment &= ~NV_SEGMENT_F & ~NV_SEGMENT_X;
  nv.flashPointer = getStartOfSegment();
}

static inline bool isBusy()
{
  return ((nv.pendingWrite      != NULL)            ||
          (nv.pendingEraseStart != NVEntry_Invalid) ||
          (nv.pendingReadStart  != NVEntry_Invalid) ||
           nv.waitingForGC);
}

static inline bool isReadBlocked()
{
  if (!clientConnected()) return false;
  return ((int)clientQueueAvailable() <= (int)sizeof(Anki::Cozmo::NVStorage::NVStorageBlob)) || (xPortGetFreeHeapSize() < 1680);
}

typedef enum
{
  GC_init,
  GC_erase,
  GC_seekEndOfFactory,
  GC_rewrite,
  GC_finalize,
} GCPhase;

struct GarbageCollectionState
{
  NVInitDoneCB finishedCallback;
  NVDataAreaHeader dah;
  u32 areaStart;
  s32 readPointer;
  s32 writePointer;
  s32 factoryPointer;
  int entryCount;
  int flashOpRetries;
  u16 sectorCounter;
  GCPhase phase;
};

#define PRINT_GC_STATE(state) { \
  os_printf("as = %x\trp = %x\twp = %x\r\nec = %d\tr  = %d\tsc = %d\r\nphase = %d\r\n", \
  state->areaStart, state->readPointer, state->writePointer, \
  state->entryCount, state->flashOpRetries, state->sectorCounter, state->phase); \
}

#define FLASH_RESULT_TO_NV_RESULT(fr) (fr == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT)

#define GET_FLASH(address, dest, size) { \
  SpiFlashOpResult r = spi_flash_read(address, dest, size); \
  if (r != SPI_FLASH_RESULT_OK) { \
    if (gc->flashOpRetries-- > 0) { \
      return true; \
    } \
    else { \
      os_printf("GF at %d failed: %x\r\n", __LINE__, r); \
      os_free(gc); \
      nv.waitingForGC = false; \
      return false; \
    } \
  } \
  else gc->flashOpRetries = FLASH_OP_RETRIES_BEFORE_FAIL; \
}

#define PUT_FLASH(address, src, size) { \
  SpiFlashOpResult r = spi_flash_write(address, src, size); \
  if (r != SPI_FLASH_RESULT_OK) { \
    if (gc->flashOpRetries-- > 0) { \
      return true; \
    } \
    else { \
      os_printf("PF at %d failed: %x\r\n", __LINE__, r); \
      os_free(gc); \
      nv.waitingForGC = false; \
      return false; \
    } \
  } \
  else gc->flashOpRetries = FLASH_OP_RETRIES_BEFORE_FAIL; \
}

#if NV_STORAGE_NUM_AREAS==2
static bool GarbageCollectionTask(uint32_t param)
{
  GarbageCollectionState* gc = reinterpret_cast<GarbageCollectionState*>(param);

  switch (gc->phase)
  {
    case GC_init:
    {
      GET_FLASH(getStartOfSegment() - sizeof(NVDataAreaHeader), reinterpret_cast<uint32*>(&gc->dah), sizeof(NVDataAreaHeader));
      if (gc->dah.nvStorageVersion != NV_STORAGE_FORMAT_VERSION && gc->dah.nvStorageVersion != 1)
      {
        os_printf("NVS GC format conversion from %d to %d not supported\r\n", gc->dah.nvStorageVersion, NV_STORAGE_FORMAT_VERSION);
        gc->finishedCallback(NV_ERROR);
        break;
      }
      if (nv.segment == NV_SEGMENT_A) gc->areaStart = NV_STORAGE_START_ADDRESS + NV_STORAGE_AREA_SIZE; // Selected segment A, write to segment B
      else if (nv.segment == NV_SEGMENT_B) gc->areaStart = NV_STORAGE_START_ADDRESS;
      else
      {
        os_printf("NVS GC unexpected start of data %x fail!\r\n", getStartOfSegment());
        gc->finishedCallback(NV_ERROR);
        break;
      }
      gc->readPointer    = getStartOfSegment();
      gc->writePointer   = gc->areaStart + sizeof(NVDataAreaHeader);
      gc->factoryPointer = FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE;
      gc->entryCount     = 0;
      gc->sectorCounter  = 0;
      gc->phase = GC_erase;
      return true;
    }
    case GC_erase:
    {
      const u16 sector = static_cast<u16>(gc->areaStart / SECTOR_SIZE) + gc->sectorCounter;
      const SpiFlashOpResult flashResult = spi_flash_erase_sector(sector);
      if (flashResult != SPI_FLASH_RESULT_OK)
      {
        if (gc->flashOpRetries-- > 0) return true;
        else
        {
          os_printf("GarbageCollectionTask ERROR: Couldn't erase sector %x\r\n", sector);
          gc->finishedCallback(FLASH_RESULT_TO_NV_RESULT(flashResult));
          break;
        }
      }
      else
      {
        gc->flashOpRetries = FLASH_OP_RETRIES_BEFORE_FAIL;
        gc->sectorCounter++;
        if ((gc->sectorCounter * SECTOR_SIZE) >= NV_STORAGE_AREA_SIZE)
        {
          os_printf("GC Erase done\r\n");
          gc->phase = GC_seekEndOfFactory;
          gc->finishedCallback(NV_OKAY);
        }
        return true;
      }
    }
    case GC_seekEndOfFactory:
    {
      NVEntryHeader header;
      GET_FLASH(gc->factoryPointer, reinterpret_cast<uint32_t*>(&header), sizeof(NVEntryHeader));
      if (header.tag == NVEntry_Invalid)
      { // End of stored factory data;
        gc->phase = GC_rewrite;
      }
      else if (header.size > MAX_ENTRY_SIZE)
      {
        os_printf("NVGC invalid header size, %d, in factory NVStorage!\r\n", header.size);
        gc->factoryPointer = FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE + NV_STORAGE_AREA_SIZE; // Put pointer at end of region so we don't try to write anything into it.
        gc->phase = GC_rewrite;
      }
      else
      {
        gc->factoryPointer += header.size;
      }
      return true;
    }
    case GC_rewrite:
    {
      #if DEBUG_NVS > 1
      PRINT_GC_STATE(gc);
      #endif
      if ((gc->readPointer - getStartOfSegment()) >= static_cast<s32>(NV_STORAGE_CAPACITY))
      {
        os_printf("NV GarbageCollect WARNING: read pointer hit end of capacity %x %x\r\n", gc->readPointer, getStartOfSegment());
        db_printf("NVS %d entries, 0x%x bytes used after GC\r\n", gc->entryCount, gc->writePointer-gc->areaStart);
        gc->phase = GC_finalize;
        return true;
      }
      else if ((gc->writePointer - gc->areaStart) >= static_cast<s32>(NV_STORAGE_CAPACITY))
      {
        os_printf("NV GarbageCollect WARNING: write pointer hit end of capacity %x %x\r\n", gc->writePointer, gc->areaStart);
        db_printf("NVS %d entries, 0x%x bytes used after GC\r\n", gc->entryCount, gc->writePointer-gc->areaStart);
        gc->phase = GC_finalize;
        return true;
      }
      else
      {
        u32 buffer[1152]; // Large enough for header + maximum blob + control word
        NVEntryHeader& header = *reinterpret_cast<NVEntryHeader*>(buffer);
        
        GET_FLASH(gc->readPointer, buffer, sizeof(NVEntryHeader));
        if (header.tag == NVEntry_Invalid)
        { // End of stored data
          db_printf("NVS %d entries, 0x%x bytes used after GC\r\n", gc->entryCount, gc->writePointer-gc->areaStart);
          gc->phase = GC_finalize;
          return true;
        }
        else if (header.size > MAX_ENTRY_SIZE)
        {
          os_printf("NVGC, invalid header size! Skipping everything after\r\n");
          db_printf("NVS %d entries, 0x%x bytes used after GC\r\n", gc->entryCount, gc->writePointer-gc->areaStart);
          gc->phase = GC_finalize;
          return true;
        }
        else
        {
          if (header.successor != 0xFFFFffff)
          { // Superceeded entry, skip over
            gc->readPointer += header.size;
            return true;
          }
          else
          {
            GET_FLASH(gc->readPointer + sizeof(NVEntryHeader), buffer + (sizeof(NVEntryHeader)/4), header.size - sizeof(NVEntryHeader));
            if (buffer[(header.size/4)-1] != 0)
            { // Control word is non-zero, this is an invalid entry so skip past
              gc->readPointer += header.size;
              return true;
            }
            else
            { // Copy this to new area
              if ((header.tag & FACTORY_DATA_BIT) &&
                  ((gc->factoryPointer + header.size) < (FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE + NV_STORAGE_AREA_SIZE)))
              { // This belongs in the factory segment and there is room, put it there
                PUT_FLASH(gc->factoryPointer, buffer, header.size);
                gc->factoryPointer += header.size;
              }
              else
              {
                PUT_FLASH(gc->writePointer, buffer, header.size);
                gc->writePointer += header.size;
              }
              gc->readPointer  += header.size;
              gc->entryCount++;
              return true;
            }
          }
        }
      }
    }
    case GC_finalize:
    {
      gc->dah.dataAreaMagic = NV_STORAGE_AREA_HEADER_MAGIC;
      gc->dah.nvStorageVersion = NV_STORAGE_FORMAT_VERSION;
      gc->dah.journalNumber += 1;
      PUT_FLASH(gc->areaStart, reinterpret_cast<uint32_t*>(&gc->dah), sizeof(NVDataAreaHeader));
      nv.segment = nv.segment == NV_SEGMENT_A ? NV_SEGMENT_B : NV_SEGMENT_A; // Swap active segment
      nv.flashPointer = getStartOfSegment();
      #if DEBUG_NVS > 0
      printNVSS();
      #endif
      break;
    }
    default:
    {
       os_printf("GarbageCollectionTask ERROR: Unhandled phase %d\r\n", gc->phase);
    }
  }
  
  os_printf("GarbageCollectionTask exiting\r\n");
  os_free(gc);
  nv.waitingForGC = false;
  return false;
}
#else
#error "GarbageCollection implementation relies on exactly two storage areas"
#endif

extern "C" int8_t GarbageCollect(NVInitDoneCB finishedCallback)
{
  if (isBusy()) return NV_BUSY;
  else
  {
    GarbageCollectionState* state = reinterpret_cast<GarbageCollectionState*>(os_malloc(sizeof(GarbageCollectionState)));
    if (state == NULL) return NV_NO_MEM;
    state->finishedCallback = finishedCallback;
    state->flashOpRetries   = FLASH_OP_RETRIES_BEFORE_FAIL;
    state->phase = GC_init;
    if (foregroundTaskPost(GarbageCollectionTask, reinterpret_cast<u32>(state)) == false)
    {
      os_printf("NVStorage GarbageCollect failed to post task\r\n");
      os_free(state);
      return NV_ERROR;
    }
    else return NV_SCHEDULED;
  }
}


extern "C" int8_t NVInit(const bool gc, NVInitDoneCB finishedCallback)
{
  NVDataAreaHeader dah;
  s32 newestJournalNumber  = -1;
  s32 newestJournalVersion = NV_STORAGE_FORMAT_VERSION;
  s8  newestJournalSegment = NV_SEGMENT_UNINITALIZED;
  s8 area;
  nv.pendingWrite = NULL;
  resetWrite();
  resetErase();
  resetRead();
    
  for (area=0; area<NV_STORAGE_NUM_AREAS; ++area)
  {
    const u32 addr = NV_STORAGE_START_ADDRESS + (NV_STORAGE_AREA_SIZE * area);
    const SpiFlashOpResult flashResult = spi_flash_read(addr, reinterpret_cast<u32*>(&dah), sizeof(NVDataAreaHeader));
    if (flashResult != SPI_FLASH_RESULT_OK)
    {
      os_printf("NVInit Failed (%d) to read flash area header at %x\r\n", flashResult, area);
      finishedCallback(FLASH_RESULT_TO_NV_RESULT(flashResult));
      return -1;
    }
    else
    {
      db_printf("NV Area %d at %x: magic = %x\tversion = %d\tnumber=%d\r\n",
                area, addr, dah.dataAreaMagic, dah.nvStorageVersion, dah.journalNumber);
      if ((dah.dataAreaMagic == NV_STORAGE_AREA_HEADER_MAGIC) && (dah.journalNumber > newestJournalNumber))
      {
        newestJournalVersion = dah.nvStorageVersion;
        newestJournalNumber  = dah.journalNumber;
        newestJournalSegment = area;
      }
    }
  }
  
  if (newestJournalSegment == NV_SEGMENT_UNINITALIZED) // Uninitalized storage
  {
    const u32 addr = NV_STORAGE_START_ADDRESS;
    dah.dataAreaMagic = NV_STORAGE_AREA_HEADER_MAGIC;
    dah.nvStorageVersion = NV_STORAGE_FORMAT_VERSION;
    dah.journalNumber = 1;
    os_printf("NVInit initalizing flash to (%d, %d) at %x\r\n", dah.nvStorageVersion, dah.journalNumber, addr);
    WipeAll(NV_STORAGE_NUM_AREAS, true, 0, false, false);
    const SpiFlashOpResult flashResult = spi_flash_write(addr, reinterpret_cast<u32*>(&dah), sizeof(NVDataAreaHeader));
    if (flashResult != SPI_FLASH_RESULT_OK)
    {
      os_printf("NVInit failed (%d) to initalize flash area header at %x\r\n", flashResult, area);
      finishedCallback(FLASH_RESULT_TO_NV_RESULT(flashResult));
      return -2;
    }
    else
    {
      nv.segment = NV_SEGMENT_A;
      nv.flashPointer = getStartOfSegment();
      printNVSS();
      finishedCallback(NV_OKAY);
      return 0;
    }
  }
  else
  {
    nv.segment = newestJournalSegment;
    nv.flashPointer = getStartOfSegment();
    os_printf("NVInit selecting segment %d at %x\r\n", nv.segment, nv.flashPointer - sizeof(NVDataAreaHeader));
    if ((newestJournalVersion != NV_STORAGE_FORMAT_VERSION) && (gc == false))
    {
      os_printf("NVInit WARNING: flash storage version %d current softare version %d but GC not scheduled!\r\n", newestJournalVersion, NV_STORAGE_FORMAT_VERSION);
      finishedCallback(NV_BAD_ARGS);
      return 1;
    }
  }
  
  if (gc)
  {
    const NVResult ret = GarbageCollect(finishedCallback);
    if (ret != NV_SCHEDULED)
    {
      os_printf("NVInit: Couldn't start garbage collection: %d\r\n", ret);
      finishedCallback(ret);
      nv.waitingForGC = false;
      return ret;
    }
    else
    {
      // Don't call finishedCallback here, GarbageCollect will call it when it's done
      nv.waitingForGC = true;
      return ret;
    }
  }
  else
  {
    //printNVSS();
    nv.waitingForGC = false;
    finishedCallback(NV_OKAY);
    return 0;
  }
}


static void processFailure(const NVResult fail)
{
  AnkiWarn( 145, "NVStorage.Operation.Failure", 401, "Fail = %d, addr = 0x%x", 2, fail, nv.flashPointer);
  
  if ((nv.pendingWrite != NULL) && (nv.pendingWriteDoneCallback != NULL))
  {
    nv.pendingWriteDoneCallback(nv.pendingWrite, fail);
  }
  resetWrite();
  
  if (nv.pendingEraseCallback != NULL) nv.pendingEraseCallback(nv.pendingEraseStart, fail);
  if (nv.pendingMultiEraseDoneCallback != NULL) nv.pendingMultiEraseDoneCallback(nv.pendingEraseStart, fail);
  resetErase();

  if (nv.pendingReadCallback != NULL) {
     NVStorageBlob temp;
     temp.tag = nv.pendingReadStart;
     nv.pendingReadCallback(&temp, fail);
  }
  if (nv.pendingMultiReadDoneCallback != NULL) nv.pendingMultiReadDoneCallback(nv.pendingReadStart, fail);
  resetRead();

  nv.flashPointer = getStartOfSegment();
}

static void endWrite(const NVResult writeResult)
{
  AnkiConditionalWarn(writeResult == NV_OKAY, 155, "NVStorage.WriteFailure", 435, "Failed to write to NV storage %d at 0x%x, phase %d", 3, writeResult, nv.flashPointer, nv.phase);

  if (nv.pendingWriteDoneCallback != NULL)
  {
    nv.pendingWriteDoneCallback(nv.pendingWrite, writeResult);
  }
  resetWrite();
}

Result Update()
{
  if (isBusy() && !isReadBlocked())
  { // If we have anything to do
    static NVEntryHeader header;
    SpiFlashOpResult flashResult;
    if (nv.segment & NV_SEGMENT_X) // Special case, reading fixture data
    {
      if (nv.pendingReadStart > nv.pendingReadEnd)
      { // We've finished with reading factory data
        if (nv.pendingMultiReadDoneCallback != NULL) nv.pendingMultiReadDoneCallback(nv.pendingReadEnd, NV_OKAY);
        resetRead();
      }
      else
      {
        const u32 address = FIXTURE_STORAGE_SECTOR * SECTOR_SIZE + ((nv.pendingReadStart & FIXTURE_DATA_ADDRESS_MASK) * FIXTURE_DATA_READ_SIZE);
        NVStorageBlob entry;
        entry.tag = nv.pendingReadStart;
        flashResult = spi_flash_read(address, reinterpret_cast<uint32*>(entry.blob), FIXTURE_DATA_READ_SIZE);
        if (flashResult != SPI_FLASH_RESULT_OK)
        {
          if (nv.retries-- <= 0)
          {
            const NVResult fail = flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT;
            entry.blob_length = 0;
            if (nv.pendingReadCallback != NULL) nv.pendingReadCallback(&entry, fail);
            if (nv.pendingMultiReadDoneCallback != NULL) nv.pendingMultiReadDoneCallback(nv.pendingReadStart, fail);
            resetRead();
          }
        }
        else
        {
          entry.blob_length = FIXTURE_DATA_READ_SIZE;
          if (nv.pendingReadCallback != NULL) nv.pendingReadCallback(&entry, NV_OKAY);
          nv.pendingReadStart += 1;
        }
      }
    }
    else if ((nv.flashPointer < getStartOfSegment()) || (nv.flashPointer >= getEndOfSegment()))
    {
      header.tag = NVEntry_Invalid;
      flashResult = SPI_FLASH_RESULT_OK;
      nv.flashPointer = getStartOfSegment();
      nv.phase = 1;
    }
    else if (nv.phase == 0)
    {
      flashResult = spi_flash_read(nv.flashPointer, reinterpret_cast<uint32*>(&header), sizeof(header));
      if (flashResult != SPI_FLASH_RESULT_OK)
      {
        if (nv.retries-- <= 0) processFailure(flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT);
      }
      else
      {
        nv.phase = 1;
      }
      db_printf("0 %x %d %x %x\r\n", nv.flashPointer, header.tag, header.size, header.successor);
    }
    else
    {
      if (header.tag == NVEntry_Invalid) // We've reached the end of flash
      {
        if (nv.pendingWrite != NULL) // We have something to write, put it here
        {
          const u32 blobSize  = ((nv.pendingWrite->blob_length + 3)/4)*4;
          const u32 entrySize = blobSize + sizeof(NVEntryHeader) + 4;
          if (nv.flashPointer + (s32)entrySize >= getEndOfSegment()) // No room!
          {
            endWrite(NV_NO_ROOM);
          }
          else // We do have room
          {
            NVEntryHeader wrHeader;
            u32 entryInvalid = 0;
            wrHeader.size = entrySize;
            wrHeader.tag  = nv.pendingWrite->tag;
            switch (nv.phase)
            {
              case 1:
              {
                flashResult = spi_flash_write(nv.flashPointer, reinterpret_cast<uint32*>(&wrHeader), sizeof(NVEntryHeader) - 4); // Write header minus sucessor word
                if (flashResult != SPI_FLASH_RESULT_OK)
                {
                  if (nv.retries-- <= 0) endWrite(FLASH_RESULT_TO_NV_RESULT(flashResult));
                }
                else
                {
                  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                  nv.phase = 2;
                }
                break;
              }
              case 2:
              {
                flashResult = spi_flash_write(nv.flashPointer + sizeof(NVEntryHeader), 
                                              reinterpret_cast<uint32*>(&(nv.pendingWrite->blob)), blobSize);
                if (flashResult != SPI_FLASH_RESULT_OK)
                {
                  if (nv.retries-- <= 0) endWrite(FLASH_RESULT_TO_NV_RESULT(flashResult));
                }
                else
                {
                  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                  nv.phase = 3;
                }
                break;
              }
              case 3:
              {
                if (nv.pendingOverwriteHeaderAddress != 0) // Update overwritten entry if any
                {
                  flashResult = spi_flash_write(nv.pendingOverwriteHeaderAddress + sizeof(NVEntryHeader) - 4, (u32*)&nv.flashPointer, 4);
                  if (flashResult != SPI_FLASH_RESULT_OK)
                  {
                    if (nv.retries-- <= 0)
                    { // Failed to invalidate old entry
                      nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                      nv.phase = 4;
                    }
                  }
                  else
                  {
                    nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                    nv.phase = 5;
                  }
                }
                else // Nothing to overwrite so skip to next step
                {
                  nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                  nv.phase = 5;
                }
                break;
              }
              case 4:
              {
                entryInvalid =  nv.pendingOverwriteHeaderAddress;
                // Explicit fallthrough to next case
              }
              case 5:
              {
                flashResult = spi_flash_write(nv.flashPointer + entrySize - 4, &entryInvalid, 4);
                if (flashResult != SPI_FLASH_RESULT_OK)
                {
                  if (nv.retries-- <= 0)
                  {
                    AnkiError( 153, "NVStorage.WriteFailure.Corruption", 411, "Couldn't overwrite old entry or invalidate new entry", 0);
                    endWrite(FLASH_RESULT_TO_NV_RESULT(flashResult));
                  }
                }
                else if (entryInvalid == 0)
                {
                  endWrite(NV_OKAY);
                }
                else
                {
                  AnkiWarn( 155, "NVStorage.WriteFailure", 412, "Couldn't overwrite old entry.", 0)
                  endWrite(NV_ERROR);
                }
                break;
              }
              default:
              {
                AnkiError( 155, "NVStorage.WriteFailure", 417, "Unexpected nv.phase = %d! aborting all", 1, nv.phase);
                processFailure(NV_ERROR);
              }
            }
          }
        } // Done with write operations at end of flash
        
        else if (nv.pendingEraseStart != NVEntry_Invalid)
        { // We were erasing anything
          if (nv.pendingMultiEraseDoneCallback != NULL)
          {
            nv.pendingMultiEraseDoneCallback(nv.pendingEraseStart, nv.multiEraseDidAny ? NV_OKAY : NV_NO_DO);
          }
          resetErase();
        } // Done with erase operations at end of flash
        
        else if (nv.pendingReadStart != NVEntry_Invalid)
        { // We were reading anything 
          if (nv.segment & NV_SEGMENT_F) // Keep looking in normal region.
          {
            nv.segment &= ~NV_SEGMENT_F;
            nv.flashPointer = getStartOfSegment();
            nv.phase = 0;
          }
          else
          { 
            if (nv.pendingReadStart == nv.pendingReadEnd)
            {
              NVStorageBlob entry;
              entry.tag = nv.pendingReadStart;
              entry.blob_length = 0;
              nv.pendingReadCallback(&entry, NV_NOT_FOUND);
            }
            else if (nv.pendingMultiReadDoneCallback != NULL)
            {
              nv.pendingMultiReadDoneCallback(nv.pendingReadStart, nv.multiReadFoundAny ? NV_OKAY : NV_NOT_FOUND);
            }
            resetRead();
          }
        } // Done with read operations at end of flash
        
        else
        {
          nv.flashPointer = getStartOfSegment();
          nv.phase = 0;
        }
      } // Done with handling end of flash
      
      else if (header.successor == 0) // Erased entry with no successor
      {
        nv.flashPointer += header.size;
        nv.phase = 0;
      }
      else if (header.successor != 0xFFFFffff) // Replaced entry
      {
        if (((nv.pendingWrite != NULL) && (nv.pendingWrite->tag == header.tag)) ||
            ((nv.pendingEraseStart != NVEntry_Invalid) && (nv.pendingEraseStart == nv.pendingEraseEnd)) ||
            ((nv.pendingReadStart  != NVEntry_Invalid) && (nv.pendingReadStart  == nv.pendingReadEnd)))
        { // This points toward the entry we are seeking
          if (((s32)header.successor < getStartOfSegment()) || ((s32)header.successor >= getEndOfSegment()))
          {
            AnkiWarn( 147, "NVStorage.Seek.BadSuccessor", 418, "Bad successor 0x%x on tag 0x%x at 0x%x, skipping to 0x%x", 4, header.successor, header.tag, nv.flashPointer, nv.flashPointer + header.size);
            nv.flashPointer += header.size;
          }
          else
          {
            nv.flashPointer = header.successor;
          }
        }
        else
        {
          nv.flashPointer += header.size;
        }
        nv.phase = 0;
      } // Done handling invalidated entry
      
      else
      { // A valid entry  
        if ((nv.pendingWrite != NULL) && (header.tag == nv.pendingWrite->tag))
        { // This is something we need to invalidate to "overwrite"
          if (nv.segment & NV_SEGMENT_F)
          {
            AnkiWarn( 203, "NVstorage.WriteInvalid", 510, "Cannot overwrite factory partition data", 0);
            if (nv.pendingWriteDoneCallback) nv.pendingWriteDoneCallback(nv.pendingWrite, NV_BAD_ARGS);
            resetWrite();
          }
          else
          {
            if (nv.pendingOverwriteHeaderAddress != 0)
            {
              AnkiWarn( 148, "NVStorage.Seek.MultiplePredicessors", 419, "Multiple predicessors for tag 0x%x, at 0x%x and 0x%x", 3, nv.pendingWrite->tag, nv.pendingOverwriteHeaderAddress, nv.flashPointer);
            }
            else 
            {
              nv.pendingOverwriteHeaderAddress = nv.flashPointer;
              db_printf("Queing overwrite of entry at %d at %x\r\n", header.tag, nv.flashPointer);
            }
            nv.flashPointer += header.size;
            nv.phase = 0;
          }
        } //  Done queing overwrite
        
        else if (nv.pendingEraseStart <= header.tag && header.tag <= nv.pendingEraseEnd)
        { // This is something we need to erase
          uint32 successor = 0;
          flashResult = spi_flash_write(nv.flashPointer + sizeof(NVEntryHeader) - 4, &successor, 4);
          if (flashResult != SPI_FLASH_RESULT_OK)
          {
            if (nv.retries-- <= 0)
            {
              const NVResult fail = FLASH_RESULT_TO_NV_RESULT(flashResult);
              AnkiWarn( 149, "NVStorage.EraseFailure", 420, "Failed to erase entry 0x%x[%d] at 0x%x, %d", 4, header.tag, header.size, nv.flashPointer, fail);
              if (nv.pendingEraseCallback != NULL) nv.pendingEraseCallback(header.tag, fail);
              if (nv.pendingMultiEraseDoneCallback != NULL) nv.pendingMultiEraseDoneCallback(nv.pendingEraseStart, fail);
              resetErase();
            }
          }
          else // Erase successful
          {
            nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
            if (nv.pendingEraseCallback != NULL) nv.pendingEraseCallback(header.tag, NV_OKAY);
            if (nv.pendingEraseStart == nv.pendingEraseEnd) resetErase(); // Was single erase and we did it
            else {
              nv.multiEraseDidAny = true;
              nv.flashPointer += header.size;
              nv.phase = 0;
            }
          }
        } // Done handling erase
        
        else if (nv.pendingReadStart <= header.tag && header.tag <= nv.pendingReadEnd)
        { // This is something we wanted to read
          NVStorageBlob entry;
          entry.tag = header.tag;
          entry.blob_length = 0;
          switch(nv.phase)
          {
            case 1: // read the valid tag first
            {
              u32 entryInvalid = 0xFFFFffff;
              flashResult = spi_flash_read(nv.flashPointer + header.size - 4, &entryInvalid, 4); 
              if (flashResult != SPI_FLASH_RESULT_OK)
              {
                if (nv.retries -- <= 0)
                {
                  const NVResult readResult = FLASH_RESULT_TO_NV_RESULT(flashResult);
                  AnkiWarn( 150, "NVStorage.ReadFailure", 421, "Failed to read entry invalid tag at 0x%x, %d", 2, nv.flashPointer, readResult);
                  AnkiAssert(nv.pendingReadCallback != NULL, 422);
                  nv.pendingReadCallback(&entry, readResult);
                  if (nv.pendingMultiReadDoneCallback != NULL) nv.pendingMultiReadDoneCallback(nv.pendingReadStart, readResult);
                  resetRead();
                }
              }
              else
              {
                nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                if (entryInvalid == 0)
                { // Valid entry, read it
                  nv.phase = 2;
                }
                else // Invalid entry, skip it
                {
                  nv.flashPointer += header.size;
                  nv.phase = 0;
                }
              }
              break;
            }
            case 2: // Now read the entry itself
            {
              const u32 blobSize = header.size - sizeof(NVEntryHeader) - 4;
              flashResult = spi_flash_read(nv.flashPointer + sizeof(NVEntryHeader), reinterpret_cast<uint32*>(entry.blob), blobSize); // Read in the entry
              if (flashResult != SPI_FLASH_RESULT_OK)
              {
                if (nv.retries-- <= 0)
                {
                  const NVResult readResult = FLASH_RESULT_TO_NV_RESULT(flashResult);
                  AnkiWarn( 150, "NVStorage.ReadFailure", 414, "Failed to read entry at 0x%x, %d", 2, nv.flashPointer, readResult);
                  AnkiAssert(nv.pendingReadCallback != NULL, 422);
                  nv.pendingReadCallback(&entry, readResult);
                  if (nv.pendingMultiReadDoneCallback != NULL) nv.pendingMultiReadDoneCallback(nv.pendingReadStart, readResult);
                  resetRead();
                }
              }
              else
              {
                nv.retries = FLASH_OP_RETRIES_BEFORE_FAIL;
                entry.blob_length = blobSize;
                AnkiAssert(nv.pendingReadCallback != NULL, 422);
                db_printf("Found something to read, sending it over\r\n");
                nv.pendingReadCallback(&entry, NV_OKAY);
                if (nv.pendingReadStart == nv.pendingReadEnd) resetRead(); // This was a single read and we did it
                else
                {
                  db_printf("Looking for next entry to read\r\n");
                  nv.multiReadFoundAny = true;
                  nv.flashPointer += header.size;
                  nv.phase = 0;
                }
              }
              break;
            }
            default:
            {
              AnkiError( 150, "NVStorage.ReadFailure", 417, "Unexpected nv.phase = %d! aborting all", 1, nv.phase);
              processFailure(NV_ERROR);
            }
          }
        } // Done handling read
        
        else
        { // This is an entry we don't care about right now, advance past
          nv.flashPointer += header.size;
          nv.phase = 0;
        }
      } // Done with handling valid entry
      
    } // done with successfully read header
  } // Done with having anything pending
  return RESULT_OK;
}


NVResult Write(NVStorageBlob* entry, WriteDoneCB callback)
{
  if (entry == NULL) return NV_BAD_ARGS;
  if (entry->tag == NVEntry_Invalid) return NV_BAD_ARGS;
  if (isBusy()) return NV_BUSY; // Already have something queued
  if ((entry->tag & FIXTURE_DATA_BIT) == FIXTURE_DATA_BIT) return NV_BAD_ARGS; // Can't write fixture data through this interface
  else if (entry->tag & FACTORY_DATA_BIT) 
  {
    nv.segment |= NV_SEGMENT_F;
  }
  nv.flashPointer = getStartOfSegment();
  nv.pendingWrite = reinterpret_cast<NVStorageBlob*>(os_zalloc(sizeof(NVStorageBlob)));
  if (nv.pendingWrite == NULL) return NV_NO_MEM;
  os_memcpy(nv.pendingWrite, entry, sizeof(NVStorageBlob));
  nv.pendingWriteDoneCallback = callback;
  db_printf("NVS Write %d scheduled\r\n", entry->tag);
  return NV_SCHEDULED;
}

NVResult Erase(const u32 tag, EraseDoneCB callback)
{
  // Erase single tag is just a special case of erase range
  return EraseRange(tag, tag, callback, NULL);
}

NVResult EraseRange(const u32 start, const u32 end, EraseDoneCB eachCallback, MultiOpDoneCB finishedCallback)
{
  if (isBusy()) return NV_BUSY; // Already have something queued
  if (start & FACTORY_DATA_BIT) return NV_BAD_ARGS; // Can't erase factory data
  nv.flashPointer = getStartOfSegment();
  nv.pendingEraseStart = start;
  nv.pendingEraseEnd   = end;
  nv.pendingEraseCallback = eachCallback;
  nv.pendingMultiEraseDoneCallback = finishedCallback;
  db_printf("NVS EraseRange %d-%d scheduled\r\n", start, end);
  return NV_SCHEDULED;
}

NVResult Read(const u32 tag, ReadDoneCB callback)
{
  // Read single is just a special case of read range
  return ReadRange(tag, tag, callback, NULL);
}

NVResult ReadRange(const u32 start, const u32 end, ReadDoneCB readCallback, MultiOpDoneCB finishedCallback)
{
  if (readCallback == NULL) return NV_BAD_ARGS;
  if (isBusy()) return NV_BUSY; // Already have something queued
  if ((start & FIXTURE_DATA_BIT) == FIXTURE_DATA_BIT)
  {
    if ((end & FIXTURE_DATA_BIT) != FIXTURE_DATA_BIT) return NV_BAD_ARGS;
    if ((start - FIXTURE_DATA_BIT) >= FIXTURE_DATA_NUM_ENTRIES) return NV_BAD_ARGS;
    else if ((end - FIXTURE_DATA_BIT) >= FIXTURE_DATA_NUM_ENTRIES) return NV_BAD_ARGS;
    else nv.segment |= NV_SEGMENT_X;
  }
  else if (start & FACTORY_DATA_BIT)
  {
    if (!(end & FACTORY_DATA_BIT)) return NV_BAD_ARGS;
    nv.segment |= NV_SEGMENT_F;
  }
  nv.flashPointer = getStartOfSegment();
  nv.pendingReadStart = start;
  nv.pendingReadEnd   = end;
  nv.pendingReadCallback = readCallback;
  nv.pendingMultiReadDoneCallback = finishedCallback;
  db_printf("NVS ReadRange %d-%d scheduled\r\n", start, end);
  return NV_SCHEDULED;
}

typedef enum {
  WAT_pause,
  WAT_segments,
  WAT_factory,
  WAT_resume,
  WAT_callback,
  WAT_reboot,
  WAT_done,
} WipeAllTaskPhase;

struct WipeAllTaskState {
  EraseDoneCB callback;
  int16_t sectorCount;
  int8_t   retries;
  uint8_t  doSegments;
  bool     includeFactory;
  bool     reboot;
  WipeAllTaskPhase phase;
};

bool WipeAllTask(uint32_t param)
{
  WipeAllTaskState* state = reinterpret_cast<WipeAllTaskState*>(param);
  clientUpdate();
  db_printf("wat %d, %x\r\n", state->phase, state->sectorCount);
  switch(state->phase)
  {
    case WAT_pause:
    {
      if (i2spiMessageQueueIsEmpty())
      {
        state->sectorCount = (NV_STORAGE_AREA_SIZE * state->doSegments / SECTOR_SIZE) - 1;
        state->phase = WAT_segments;
      }
      return true;
    }
    case WAT_segments:
    {
      if (state->sectorCount >= 0)
      {
        const SpiFlashOpResult rslt = spi_flash_erase_sector(NV_STORAGE_SECTOR + state->sectorCount);
        if (rslt == SPI_FLASH_RESULT_OK) state->sectorCount -= 1;
      }
      else
      {
        state->sectorCount = (NV_STORAGE_AREA_SIZE / SECTOR_SIZE) - 1;
        state->phase = WAT_factory;
      }
      return true;
    }
    case WAT_factory:
    {
      AnkiDebug( 204, "NVStorage.WipeAllTask", 511, "Refusing to wipe factory", 0);
      state->sectorCount = 0;
      state->phase = WAT_resume;
      return true;
    }
    case WAT_resume:
    {
      state->phase = WAT_callback;
      return true;
    }
    case WAT_callback:
    {
      if (state->callback) state->callback(NVEntry_Invalid, NV_OKAY);
      if (state->reboot) state->phase = WAT_reboot;
      else state->phase = WAT_done;
      return true;
    }
    case WAT_reboot:
    {
      if (i2spiMessageQueueIsEmpty()) 
      {
        state->phase = WAT_done;
      }
      return true;
    }
    case WAT_done:
    default:
    {
      return false;
    }
  }
}

NVResult WipeAll(const u8 doSegments, const bool includeFactory, EraseDoneCB callback, const bool fork, const bool reboot)
{
  if (includeFactory) return NV_BAD_ARGS;
  else if (doSegments > NV_STORAGE_NUM_AREAS) return NV_BAD_ARGS;
  else if (isBusy()) return NV_BUSY;
  else
  {
    WipeAllTaskState* wats = reinterpret_cast<WipeAllTaskState*>(os_malloc(sizeof(WipeAllTaskState)));
    if (wats == NULL) return NV_NO_MEM;
    else
    {
      wats->callback = callback;
      wats->doSegments = doSegments;
      wats->includeFactory = includeFactory;
      wats->reboot = reboot;
      wats->sectorCount = 0;
      wats->retries = FLASH_OP_RETRIES_BEFORE_FAIL;
      wats->phase = WAT_pause;
      if (fork)
      {
        if (foregroundTaskPost(WipeAllTask, reinterpret_cast<uint32_t>(wats)) == false)
        {
          os_free(wats);
          return NV_BUSY;
        }
        else return NV_SCHEDULED;
      }
      else
      {
        while (WipeAllTask(reinterpret_cast<uint32_t>(wats)));
        return NV_OKAY;
      }
    }
  }
}

} // NVStorage
} // Cozmo
} // Anki
