/**
* File: emrHelper.h
*
* Author: Al Chaussee
* Date:   1/30/2018
*
* Description: Electronic medical(manufacturing) record written to the robot at the factory
*              DO NOT MODIFY
*
* Copyright: Anki, Inc. 2018
**/

#ifndef FACTORY_EMR_H
#define FACTORY_EMR_H

#include "emr.h"
#include "util/logging/logging.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace Anki {
namespace Cozmo {

namespace Factory {

  namespace 
  {
    static const char* kEMRFile = "/dev/block/bootdevice/by-name/emr";

    static Factory::EMR* _emr = nullptr;

    // Open the emr with the provided flags
    // Memory maps the EMR file and returns a pointer to it
    static Factory::EMR* OpenEMR(int openFlags, int protFlags)
    {
      int fd = open(kEMRFile, openFlags);
      
      if(fd == -1)
      {
        LOG_ERROR("Factory.CheckEMRForPackout.OpenFailed", "%d", errno);
        return nullptr; // exit instead? will probably end up crashing in WriteEMR or GetEMR will return null
      }

      // Memory map the file as shared since multiple processes have access to these functions
      Factory::EMR* emr = (Factory::EMR*)(mmap(nullptr, sizeof(EMR), protFlags, MAP_SHARED, fd, 0));
      
      if(emr == MAP_FAILED)
      {
        LOG_ERROR("Factory.CheckEMRForPackout.MmapFailed", "%d", errno);
        close(fd);
        return nullptr; // exit instead? will probably end up crashing in WriteEMR or GetEMR will return null
      }

      // Close our fd, mmap will add a reference to it so the mapping will still be valid
      close(fd);
      
      return emr;
    }
  
    // Attempt to read the emr file with the appropriate flags
    // First opens as readonly and then may reopen as writable depending
    // on flags and build type
    static void ReadEMR()
    {
      // If the EMR has not been read yet
      if(_emr == nullptr)
      {
        // First open as read only to check if we have been packed out
        _emr = OpenEMR(O_RDONLY, PROT_READ);

        // If this is the factory test and the emr is null (doesn't exist) or we have not been packed out then
        // /factory should be writable so close the emr and reopen as writable
        if(FACTORY_TEST && (_emr == nullptr || !_emr->fields.PACKED_OUT_FLAG))
        {
          PRINT_NAMED_INFO("Factory.ReadEMR.ReopeningAsWritable","");

          int rc = munmap(_emr, sizeof(_emr));
          if(rc < 0)
          {
            LOG_ERROR("Factory.ReadEMR.UnmapFailed", "%d", errno);
          }

          // O_SYNC because multiple processes may have the file mapped
          _emr = OpenEMR(O_RDWR | O_SYNC | O_CREAT, PROT_READ | PROT_WRITE);
        }
      }
    }
  }

  // Write data of size len into the EMR at offset
  // Ex: WriteEMR(offsetof(Factory::EMR, playpen)/sizeof(uint32_t), buf, sizeof(buf));
  //     to write buf in the playpen member of the EMR
  static void WriteEMR(size_t offset, void* data, size_t len)
  {
    #ifdef SIMULATOR
    return;
    #endif

    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    if(FACTORY_TEST)
    {
      memcpy(_emr->data + offset, data, len);
    }
    else
    {
      LOG_WARNING("Factory.WriteEMR.FailedWrite", "Failed to write to EMR not factory test");
    }
  }

  static void WriteEMR(size_t offset, uint32_t data)
  {
    #ifdef SIMULATOR
    return;
    #endif

    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    if(FACTORY_TEST)
    {
      _emr->data[offset] = data;
    }
    else
    {
      LOG_WARNING("Factory.WriteEMR.FailedWrite", "Failed to write to EMR not factory test");
    }
  }

  static const Factory::EMR* const GetEMR()
  {
    #ifdef SIMULATOR
      static Factory::EMR emr;
      return &emr;
    #endif

    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    if(_emr == nullptr)
    {
      LOG_WARNING("Factory.GetEMR.NullEMR", "EMR is null, is /factory writable?");
    }

    return _emr;
  }
}

}
}

#endif
