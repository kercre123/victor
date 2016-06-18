/** @file Non Volatile storage on the Espressif Header
 * This module is intended for storing small amounts of persistent data on the robot. Total storage < 256kB and each
 * stored element <4kb.
 * Due to SPI flash timing restrictions, almost all operations are done with only flash reads. See maintenance functions
 * below for details.
 * @author Daniel Casner <daniel@anki.com>
 */
 
#ifndef __ESP_APPLICATION_NV_STORAGE_H
#define __ESP_APPLICATION_NV_STORAGE_H

extern "C" {
  #include "user_interface.h"
  #include "flash_map.h"
}
#include "anki/types.h"
#include "clad/types/nvStorage.h"

/// Header on the start of each data segment. Not written until the entire segment is valid.
struct NVDataAreaHeader
{
  u32 dataAreaMagic;    ///< Must equal NV_STORAGE_AREA_HEADER_MAGIC
  s32 nvStorageVersion; ///< Version of the NVStorage protocol
  s32 journalNumber;    ///< Monotonically incrementing number indicating which region is more recent
};

/// 0th word of valid NVStorage area
#define NV_STORAGE_AREA_HEADER_MAGIC 0xDEADBEEF

/// The version number for data in flash, magic to distinquish from all previous data
#define NV_STORAGE_FORMAT_VERSION 2

#define NV_STORAGE_NUM_AREAS 2
#define NV_STORAGE_START_ADDRESS (NV_STORAGE_SECTOR * SECTOR_SIZE)
#define NV_STORAGE_END_ADDRESS   (ESP_INIT_DATA_SECTOR * SECTOR_SIZE)
#define NV_STORAGE_AREA_SIZE     ((NV_STORAGE_END_ADDRESS - NV_STORAGE_START_ADDRESS) / NV_STORAGE_NUM_AREAS)
#define NV_STORAGE_CAPACITY      (NV_STORAGE_AREA_SIZE - sizeof(NVDataAreaHeader))

namespace Anki {
  namespace Cozmo {
    namespace NVStorage {
      
      /** Callback prototype for NVCheckIntegrity completion
       * parameter receives if the flash system is OK or corrupted
       */
      typedef void (*NVInitDoneCB)(const int8_t);
      /** Callback prototype for NV Write operations.
       * Parameters are the tag written and success
       */
      typedef void (*WriteDoneCB)(const NVStorageBlob*, const NVResult);
      /** Callback prototype for NV Erase operations
       * Parameters are the tag erased and success
       */
      typedef void (*EraseDoneCB)(const u32, const NVResult);
      /** Callback prototype for NV Read operations
       * Parameters are the retrieved storage entry and result
       */
      typedef void (*ReadDoneCB)(NVStorageBlob*, const NVResult);
      /** Callback prototype for NV multi-operations
       */
      typedef void (*MultiOpDoneCB)(const u32, const NVResult);
      
      /// Run pending tasks for NVStorage system and call any pending callbacks
      Result Update();
      
      /** Writes an NV storage entry into flash.
       * @param[in] entry The data to write. If the entry tag matches and existing entry, the existing entry will be
       *              overwritten.
       * @param callback Function to call when write operation completes either successfully or with error
       * @return Result code for the operation
       */
      NVResult Write(NVStorageBlob* entry, WriteDoneCB callback=0);
      
      /** Erases (invalidates) entry for given tag if present
       * @param tag The tag to erase
       * @param callback Function to call when erase operation completes either successfully or with error
       * @return Either scheduled or busy
       */
      NVResult Erase(const u32 tag, EraseDoneCB callback=0);
      
      /** Erases (invalidates) a range of tags if present
       * @warning entries are not garunteed to be erased in any particular order
       * @param start The lowest tag value to invalidate (inclusive)
       * @param end The highest tag value to invalidate (inclusive)
       * @param eachCallback Function to call after each individual erase operation
       * @param finishCallback Function to call after all entries in range have been erased
       * @return Either scheduled or busy
       */
      NVResult EraseRange(const u32 start, const u32 end,
                          EraseDoneCB eachCallback=0, MultiOpDoneCB finishedCallback=0);
      
      /** Reads an NV storage entry from flash.
       * @param tag The tag to retrieve
       * @param callback Function to call with read data result. Must not be NULL.
       * @return Either scheduled or busy
       */
       NVResult Read(const u32 tag, ReadDoneCB callback);
       
       /** Reads all entries in NV storage who's tags fall in the range
        * @warning entries are not garunteed to be retrieved in any particular order
        * @param start The lowest tag value to retrieve (inclusive)
        * @param end The highes tag value to retreive (inclusive)
        * @param readCallback Function to call with each retrieved entry. Must not be NULL
        * @param finishCallback Function to call after all entries in range have been read.
        * @return either scheduled or busy
        */
      NVResult ReadRange(const u32 start, const u32 end, ReadDoneCB readCallback, MultiOpDoneCB finishedCallback=0);
      
      /** Wipe all contents of NVStorage
       * This does more than erase all entries, it actually erases all the sectors of flash which store NVData
       * @param includeFactory whether or not to also wipe factory NVStorage.
       * @param callback a function to call when the wipe is complete, default none
       * @param fork whether to run the wipe in a task or in the calling task, default true
       * @param reboot whether to reboot after applying the update
       * @return either scheduled or busy
       */
      NVResult WipeAll(const bool includeFactory, EraseDoneCB callback=0, const bool fork=true, const bool reboot=false);

      /** Return true if factory storage is empty - in other words, this robot has not visited the Playpen or has been WipeAlled
       */
      bool IsFactoryStorageEmpty();
      
       extern "C" {
         /** Run garbage collection
          * @warning This method must only be called when it is safe to do flash erase operations.
          * @param finishedCallback Function to call when garbage collection is complete
          * @return Result code for garbage collection. NV_SCHEDULED is the normal case.
          */
         int8_t GarbageCollect(NVInitDoneCB finishedCallback);
         
         /** Initalizes and checks the integrity of the non-volatile storge flash file system and completes any
          * interrupted operations.
          * @param garbageCollect If true, run garbage collection.
          * @param finishCallback Function to call when integrity check is complete
          * @return 0 If the integrity check was successfully started or something else on error.
          */
        int8_t NVInit(const bool garbageCollect, NVInitDoneCB finishedCallback);
       }
    }
  }
}

#endif
