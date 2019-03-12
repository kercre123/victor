/**
* File: emrHelper.h
*
* Author: Al Chaussee
* Date:   1/30/2018
*
* Description: Electronic medical(manufacturing) record written to the robot at the factory
*              DO NOT MODIFY
*              Do not include this file directly, instead include emrHelper.h
*
* Copyright: Anki, Inc. 2018
**/

#ifndef FACTORY_EMR_MAC_H
#define FACTORY_EMR_MAC_H

#include "emr.h"
#include "util/logging/logging.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using emrHelper_mac.h
#endif

namespace Anki {
namespace Vector {

namespace Factory {

  namespace 
  {
    // The simulated EMR is created at runtime by webotsCtrlRobot2.cpp which allows us
    // to determine if the simulated robot is Whiskey or Vector. As such it needs to be created
    // in a shared location so all webots controllers that need access to it can easily access it.
    // We already create and write files to the playbackLogs dir so it felt like the best place to
    // stick the EMR
    static const char* kEMRFile = "../../../_build/mac/Debug/playbackLogs/emr";

    static Factory::EMR* _emr = nullptr;

    static Factory::EMR _fakeEMR = {0};

    // Open the emr with the provided flags
    // Memory maps the EMR file and returns a pointer to it
    static Factory::EMR* OpenEMR(int openFlags, int protFlags)
    {
      int fd = open(kEMRFile, openFlags);

      if(fd == -1)
      {
        PRINT_NAMED_INFO("Factory.CheckEMRForPackout.OpenFailed", "%d %s", errno, kEMRFile);
        return &_fakeEMR;
      }

      (void)truncate(kEMRFile, sizeof(Factory::EMR));

      // Memory map the file as shared since multiple processes have access to these functions
      Factory::EMR* emr = (Factory::EMR*)(mmap(nullptr, sizeof(EMR), protFlags, MAP_SHARED, fd, 0));
      
      if(emr == MAP_FAILED)
      {
        LOG_ERROR("Factory.CheckEMRForPackout.MmapFailed", "%d", errno);
        close(fd);
        return &_fakeEMR;
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
        // O_SYNC because multiple processes may have the file mapped
        _emr = OpenEMR(O_RDWR | O_SYNC | O_CREAT, PROT_READ | PROT_WRITE);
      }
    }
  }

  // Write data of size len into the EMR at offset
  // Ex: WriteEMR(offsetof(Factory::EMR, playpen)/sizeof(uint32_t), buf, sizeof(buf));
  //     to write buf in the playpen member of the EMR
  static void WriteEMR(size_t offset, void* data, size_t len)
  {
    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    memcpy(_emr->data + offset, data, len);
  }

  static void WriteEMR(size_t offset, uint32_t data)
  {
    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    _emr->data[offset] = data;
  }

  static const Factory::EMR* const GetEMR()
  {
    // Attempt to read the EMR, will do nothing if it has already been read
    ReadEMR();

    return _emr;
  }

static void CreateFakeEMR()
{
  FILE* f = fopen(kEMRFile, "w+");
  if(f != nullptr)
  {
    Anki::Vector::Factory::EMR emr;
    memset(&emr, 0, sizeof(emr));
    (void)fwrite(&emr, sizeof(emr), 1, f);
    (void)fclose(f);
  }
  else
  {
    LOG_WARNING("Factory.CreateFakeEMR.OpenFailed", "%d", errno);
  }
}

  static void SetWhiskey(bool whiskey)
  {
    WriteEMR(offsetof(Factory::EMR::Fields, HW_VER)/sizeof(uint32_t), (whiskey ? 7 : 6));
  } 

  class ScopedWhiskey
  {
  public:
    ScopedWhiskey() { SetWhiskey(true); }
    ~ScopedWhiskey() { SetWhiskey(false); }
  };
}

#define UNIT_TEST_WHISKEY Factory::ScopedWhiskey whiskeyEMR;

static inline bool IsWhiskey()
{
  /* 
    From robot/fixture/stm/hwid.h
    #define HEADID_HWREV_EMPTY      0 //unprogrammed/empty value
    #define HEADID_HWREV_DEBUG      1 //debug use and DVT1-3
    #define HEADID_HWREV_DVT4       4
    #define HEADID_HWREV_PVT        5
    #define HEADID_HWREV_MP         6
    #define HEADID_HWREV_WHSK_DVT1  7 //Whiskey (Vector 2019)
  */
  
  return (Factory::GetEMR()->fields.HW_VER >= 7);
}

}
}

#endif
