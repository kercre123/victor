/**
 * File: sim_nvStorage.cpp
 *
 * Author: Al Chaussee
 * Created: 4/21/2016
 *
 * Description:
 *    Simulated NVStorage for simulating reading and writing data to robot flash
 *    Is able to save and load data from file simulating how the NVStorage is constant
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef SIMULATOR
#error This file (sim_nvStorage.cpp) should not be used without SIMULATOR defined.
#endif

#include "sim_nvStorage.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "util/fileUtils/fileUtils.h"

// Whether or not to save to/load from file
#define LOAD_FROM_FILE 0

namespace Anki {
  namespace Cozmo {
    
    SimNVStorage::SimNVStorage()
    {
      if(LOAD_FROM_FILE)
      {
        // Reads from nvstorage.txt
        if(false == Util::FileUtils::CreateDirectory("nvStorage", false, true))
        {
          printf("Sim_Robot.SimNVStorage: Failed to create nvStorage directory\n");
          return;
        }
        std::string file = Util::FileUtils::ReadFile("nvStorage/nvstorage.txt");
        if(file.length() == 0)
        {
          return;
        }
        const char* fileContents = file.data();
        
        // Parses the contents of nvstorage.txt into blobs and adds them to nvStorage
        bool finishedParsing = false;
        uint32_t sectionStart = 0;
        while(!finishedParsing)
        {
          uint32_t tag = (fileContents[sectionStart] & 0xff) |
                         (fileContents[sectionStart+1] & 0xff) << 8 |
                         (fileContents[sectionStart+2] & 0xff) << 16 |
                         (fileContents[sectionStart+3] & 0xff) << 24;
          
          sectionStart += 4;
          
          uint32_t blob_length = (fileContents[sectionStart] & 0xff) |
                                 (fileContents[sectionStart+1] & 0xff) << 8 |
                                 (fileContents[sectionStart+2] & 0xff) << 16 |
                                 (fileContents[sectionStart+3] & 0xff) << 24;
          
          sectionStart += 4;
          
          NVStorage::NVStorageBlob b;
          b.tag = tag;
          b.blob_length = blob_length;
          memcpy(b.blob, fileContents+sectionStart, blob_length);
          
          sectionStart += blob_length;
          
          printf("Sim_Robot.LoadNVStorage: Adding blob with tag:%u len:%d\n", b.tag, b.blob_length);
          nvStorage_.insert( std::pair<uint32_t, NVStorage::NVStorageBlob>(b.tag, b) );
          
          curStorageSize_bytes_ += b.Size();
          
          if(sectionStart >= file.length())
          {
            finishedParsing = true;
          }
        }
      }
    }
    
    SimNVStorage::~SimNVStorage()
    {
      if(LOAD_FROM_FILE)
      {
        // Saves each of the stored blobs to nvstorage.txt
        std::vector<uint8_t> contents;
        for(auto iter = nvStorage_.begin(); iter != nvStorage_.end(); ++iter)
        {
          uint8_t* blob = iter->second.GetBuffer();
          for(uint32_t i = 0; i < iter->second.Size(); i++)
          {
            contents.push_back(blob[i]);
          }
        }
        Util::FileUtils::WriteFile("nvStorage/nvstorage.txt", contents);
      }
    }
    
    bool SimNVStorage::TagExists(uint32_t tag)
    {
      return (nvStorage_.find(tag) != nvStorage_.end());
    }
    
    bool SimNVStorage::IsValidRange(uint32_t start, uint32_t end)
    {
      return (start <= end);
    }
    
    NVStorage::NVStorageBlob SimNVStorage::MakeWordAligned(NVStorage::NVStorageBlob blob)
    {
      uint8_t numBytesToMakeAligned = 4 - (blob.blob_length % 4);
      if(numBytesToMakeAligned < 4)
      {
        memset(blob.blob + blob.blob_length, 0, numBytesToMakeAligned);
        blob.blob_length += numBytesToMakeAligned;
      }
      return blob;
    }
    
    void SimNVStorage::NVWrite(Anki::Cozmo::NVStorage::NVStorageWrite const& msg)
    {
      // Write
      if(msg.writeNotErase)
      {
        printf("Sim_Robot.NVWrite: Writing tag %u\n", msg.entry.tag);
        // Safe to overwrite existing tags
        if(nvStorage_.find(msg.entry.tag) != nvStorage_.end())
        {
          // Make the blob aligned and add it to nvStorage
          NVStorage::NVStorageBlob aligned = MakeWordAligned(msg.entry);
          curStorageSize_bytes_ += aligned.Size() - nvStorage_[msg.entry.tag].Size();
          nvStorage_[msg.entry.tag] = aligned;
        }
        else
        {
          // If no room for blob then NV_NO_ROOM
          if(curStorageSize_bytes_ + msg.entry.Size() > maxStorageSize_bytes)
          {
            RobotInterface::NVOpResultToEngine m;
            m.robotAddress = 1;
            m.report.tag = static_cast<u32>(msg.entry.tag);
            m.report.result = NVStorage::NV_NO_ROOM;
            m.report.write = true;
            nvOpResultMsgQueue_.push(m);
            return;
          }
        
          // Make the blob aligned and add it to nvStorage
          NVStorage::NVStorageBlob aligned = MakeWordAligned(msg.entry);
          nvStorage_.insert( std::pair<uint32_t, NVStorage::NVStorageBlob>(msg.entry.tag, aligned));
          curStorageSize_bytes_ += aligned.Size();
        }
        if(msg.reportEach || msg.reportDone)
        {
          RobotInterface::NVOpResultToEngine m;
          m.robotAddress = 1;
          m.report.tag = static_cast<u32>(msg.entry.tag);
          m.report.result = NVStorage::NV_OKAY;
          m.report.write = true;
          nvOpResultMsgQueue_.push(m);
        }
      }
      // Erase
      else
      {
        // Whether of not we are erasing multiple blobs
        const bool multiErase = msg.entry.tag != msg.rangeEnd;
        
        // Erasing single blob
        if(!multiErase || msg.rangeEnd == NVStorage::NVEntry_Invalid)
        {
          printf("Sim_Robot.NVErase: Erasing tag 0x%x\n", msg.entry.tag);
          nvStorage_.erase(msg.entry.tag);
          
          if(msg.reportEach)
          {
            RobotInterface::NVOpResultToEngine m;
            m.robotAddress = 1;
            m.report.tag = static_cast<u32>(msg.entry.tag);
            m.report.result = NVStorage::NV_OKAY;
            m.report.write = true;
            nvOpResultMsgQueue_.push(m);
          }
        }
        // Erasing multi-blob
        else
        {
          if(!IsValidRange(msg.entry.tag, msg.rangeEnd))
          {
            RobotInterface::NVOpResultToEngine m;
            m.robotAddress = 1;
            m.report.tag = static_cast<u32>(msg.entry.tag);
            m.report.result = NVStorage::NV_BAD_ARGS;
            m.report.write = false;
            nvOpResultMsgQueue_.push(m);
            return;
          }
        
          for(auto iter = nvStorage_.begin(); iter != nvStorage_.end(); )
          {
            if(iter->first >= msg.entry.tag && iter->first <= msg.rangeEnd)
            {
              uint32_t tag = iter->first;
              iter = nvStorage_.erase(iter);
              printf("Sim_Robot.NVErase: Erasing tag 0x%x\n", tag);
              
              if(msg.reportEach)
              {
                RobotInterface::NVOpResultToEngine m;
                m.robotAddress = 1;
                m.report.tag = static_cast<u32>(tag);
                m.report.result = NVStorage::NV_OKAY;
                m.report.write = true;
                nvOpResultMsgQueue_.push(m);
              }
            } else {
              ++iter;
            }
          }
          
          if(msg.reportDone)
          {
            RobotInterface::NVOpResultToEngine m;
            m.robotAddress = 1;
            m.report.tag = static_cast<u32>(msg.entry.tag);
            m.report.result = NVStorage::NV_OKAY;
            m.report.write = true;
            nvOpResultMsgQueue_.push(m);
          }
        }
      }
    }
    
    void SimNVStorage::NVRead(Anki::Cozmo::NVStorage::NVStorageRead const& msg)
    {
      // If tag doesn't exist NV_NOT_FOUND
      if(!TagExists(msg.tag))
      {
        RobotInterface::NVOpResultToEngine m;
        m.robotAddress = 1;
        m.report.tag = static_cast<u32>(msg.tag);
        m.report.result = NVStorage::NV_NOT_FOUND;
        m.report.write = false;
        nvOpResultMsgQueue_.push(m);
        return;
      }
      
      // Multi tag
      if(msg.tag != msg.tagRangeEnd && msg.tagRangeEnd != NVStorage::NVEntry_Invalid)
      {
        auto lower = nvStorage_.lower_bound(msg.tag);
        auto upper = nvStorage_.upper_bound(msg.tagRangeEnd);
        
        while(lower != upper)
        {
          printf("Sim_Robot.NVRead: Reading tag 0x%x\n", lower->first);
          RobotInterface::NVReadResultToEngine m;
          m.robotAddress = 1;
          m.blob = lower->second;
          nvReadResultMsgQueue_.push(m);
          ++lower;
        }
        
        RobotInterface::NVOpResultToEngine m;
        m.robotAddress = 1;
        m.report.tag = static_cast<u32>(msg.tag);
        m.report.result = NVStorage::NV_OKAY;
        m.report.write = false;
        nvOpResultMsgQueue_.push(m);
      }
      // Single tag
      else
      {
        printf("Sim_Robot.NVRead: Reading tag 0x%x\n", msg.tag);
        RobotInterface::NVReadResultToEngine m;
        m.robotAddress = 1;
        m.blob = nvStorage_[msg.tag];
        nvReadResultMsgQueue_.push(m);
      }
    }
    
    void SimNVStorage::NVUpdate()
    {
      if(HAL::GetTimeStamp() - lastSendTime_ > delayBetweenMessages_ms)
      {
        lastSendTime_ = HAL::GetTimeStamp();
        if(nvReadResultMsgQueue_.size() > 0)
        {
          RobotInterface::SendMessage(nvReadResultMsgQueue_.front());
          nvReadResultMsgQueue_.pop();
        }
        else if(nvOpResultMsgQueue_.size() > 0)
        {
          RobotInterface::SendMessage(nvOpResultMsgQueue_.front());
          nvOpResultMsgQueue_.pop();
        }
      }
    }
    
    void SimNVStorage::NVWipeAll(Anki::Cozmo::NVStorage::NVWipeAll const& msg)
    {
      RobotInterface::NVOpResultToEngine m;
      m.robotAddress = 1;
      m.report.tag = NVStorage::NVEntry_WipeAll;
      m.report.write = true;
      
      if ( strncmp(msg.key, "Yes I really want to do this!", msg.key_length) != 0 ) {
        m.report.result = NVStorage::NV_BAD_ARGS;
      } else {
        if (msg.includeFactory) {
          nvStorage_.clear();
        } else {
          // Find first and last non-factory entry
          auto firstEntry = nvStorage_.begin();
          if (firstEntry->first < 0x80000000) {
            auto lastEntry = nvStorage_.lower_bound(0x80000000);
            nvStorage_.erase(firstEntry, lastEntry);
          }
        }
        m.report.result = NVStorage::NV_OKAY;
      }

      nvOpResultMsgQueue_.push(m);
    }
    
    namespace SimNVStorageSpace
    {
      namespace
      {
        SimNVStorage simNVStorage;
      }
      
      void Update()
      {
        simNVStorage.NVUpdate();
      }
      
      void Write(Anki::Cozmo::NVStorage::NVStorageWrite const& msg)
      {
        simNVStorage.NVWrite(msg);
      }
      
      void Read(Anki::Cozmo::NVStorage::NVStorageRead const& msg)
      {
        simNVStorage.NVRead(msg);
      }
        
      void WipeAll(Anki::Cozmo::NVStorage::NVWipeAll const& msg)
      {
        simNVStorage.NVWipeAll(msg);
      }
    }
  }
}
