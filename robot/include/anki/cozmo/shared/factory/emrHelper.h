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

#define BEGIN_DONT_RUN_AFTER_PACKOUT \
  if(!Factory::GetEMR()->PACKED_OUT_FLAG) {

#define END_DONT_RUN_AFTER_PACKOUT \
  }

namespace Anki {
namespace Cozmo {

namespace Factory {

  namespace 
  {
    static const char* kEMRFile = "/factory/birthcertificate";
    // static const char* kEMRFile = "/data/data/com.anki.victor/birthcertificate";

    static Factory::EMR* _emr = nullptr;
  
    // Memory maps the EMR file to _emr
    // Will create the file if it doesn't exist and fill it with 0
    static void ReadEMR()
    {
      // If the EMR has not been read yet
      if(_emr == nullptr)
      {
        // Only the factory test can create and write to the EMR
        // O_SYNC because multiple processes may have the file mapped
        int flags = -1;
        #if FACTORY_TEST
          flags = O_RDWR | O_SYNC | O_CREAT;
        #else
          flags = O_RDONLY;
        #endif

        int fd = open(kEMRFile, flags);
        
        if(fd == -1)
        {
          LOG_ERROR("Factory.ReadEMR.OpenFailed", "%d", errno);
          return; // exit instead? will probably end up crashing in WriteEMR or GetEMR will return null
        }
        
        // Makes the file the correct size, only does something if the file was just
        // created otherwise it should always be the size of the EMR struct
        ftruncate(fd, sizeof(Factory::EMR));

        // Only factory test can write
        int prot = PROT_READ;
        #if FACTORY_TEST
          prot |= PROT_WRITE;
        #endif

        // Memory map the file as shared since multiple processes have access to these functions
        _emr = (Factory::EMR*)(mmap(nullptr, sizeof(EMR), prot, MAP_SHARED, fd, 0));

        if(_emr == MAP_FAILED)
        {
          _emr == nullptr;
          LOG_ERROR("Factory.ReadEMR.MmapFailed", "%d", errno);
          return; // exit instead? will probably end up crashing in WriteEMR or GetEMR will return null
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
