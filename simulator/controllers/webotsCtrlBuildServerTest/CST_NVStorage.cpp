/**
 * File: CST_NVStorage.cpp
 *
 * Author: Al Chaussee
 * Created: 4/25/2016
 *
 * Description: See TestStates below
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "simulator/game/cozmoSimTestController.h"
#include "engine/robot.h"
#include "util/random/randomGenerator.h"


namespace Anki {
  namespace Vector {
    
    enum class TestState {
      WriteSingleBlob,
      ReadSingleBlob,
      VerifySingleBlob,
      
      WriteMultiBlob,
      ReadMultiBlob,
      VerifyMultiBlob,
      
      EraseSingleBlob,
      VerifySingleErase,
      
      EraseMultiBlob,
      VerifyMultiErase,
      
      WritingToInvalidMultiTag,
      
      WriteData,
      WipeAll,
      
      Final
    };
    
    typedef NVStorage::NVEntryTag Tag;
    
    // ============ Test class declaration ============
    class CST_NVStorage : public CozmoSimTestController {
      
    private:
    
      void AppendRandomData(int length, std::vector<uint8_t>& data);
      
      bool IsDataSame(const uint8_t* d1, const uint8_t* d2, int length);
      
      void ClearAcks();
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::WriteSingleBlob;
      
      // Message handlers
      virtual void HandleNVStorageOpResult(const ExternalInterface::NVStorageOpResult &msg) override;
    
      Util::RandomGenerator r;
      
      const Tag singleBlobTag = NVStorage::NVEntryTag::NVEntry_GameUnlocks;
      const Tag multiBlobTag = NVStorage::NVEntryTag::NVEntry_FaceAlbumData;
      const static int numMultiBlobs = 5;
      
      uint8_t _dataWritten[numMultiBlobs][1024];
      
      bool _writeAckd = false;
      bool _readAckd = false;
      bool _eraseAckd = false;
      
      int _numWrites = 0;
      
      NVStorage::NVResult _lastResult = NVStorage::NVResult::NV_OKAY;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_NVStorage);
    
    
    void CST_NVStorage::AppendRandomData(int length, std::vector<uint8_t>& data)
    {
      for(int i=0;i<length;i++)
      {
        int n = r.RandInt(256);
        data.push_back((uint8_t)n);
      }
    }
    
    bool CST_NVStorage::IsDataSame(const uint8_t* d1, const uint8_t* d2, int length)
    {
      return (memcmp(d1, d2, length) == 0);
    }
    
    void CST_NVStorage::ClearAcks()
    {
      _writeAckd = false;
      _readAckd = false;
      _eraseAckd = false;
      _numWrites = 0;
    }
    
    
    // =========== Test class implementation ===========
    
    s32 CST_NVStorage::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::WriteSingleBlob:
        {
          ExternalInterface::NVStorageWriteEntry msg;
          AppendRandomData(5, msg.data);
          memcpy(_dataWritten[0], msg.data.data(), 5);
          msg.tag = singleBlobTag;
          msg.index = 0;
          msg.numTotalBlobs = 1;
          
          ExternalInterface::MessageGameToEngine message;
          message.Set_NVStorageWriteEntry(msg);
          SendMessage(message);
          
          SET_TEST_STATE(ReadSingleBlob);
          break;
        }
        case TestState::ReadSingleBlob:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                _writeAckd,
                                                _lastResult == NVStorage::NVResult::NV_OKAY)
          {
            ClearAcks();
          
            ExternalInterface::NVStorageReadEntry msg;
            msg.tag = singleBlobTag;
            
            ExternalInterface::MessageGameToEngine message;
            message.Set_NVStorageReadEntry(msg);
            SendMessage(message);
          
            SET_TEST_STATE(VerifySingleBlob);
          }
          break;
        }
        case TestState::VerifySingleBlob:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                _readAckd,
                                                _lastResult == NVStorage::NVResult::NV_OKAY)
          {
            ClearAcks();
            
            const std::vector<u8>* data = GetReceivedNVStorageData(singleBlobTag);
            
            CST_ASSERT(IsDataSame(_dataWritten[0], data->data(), 5),
                       "Data written to and read from single blob is not the same");
            CST_ASSERT(data->size() == 8, "Data read from single blob is not expected word-aligned size");
            
            SET_TEST_STATE(WriteMultiBlob);
          }
          break;
        }
        case TestState::WriteMultiBlob:
        {
          for(int i = 0; i < numMultiBlobs; i++)
          {
            ExternalInterface::NVStorageWriteEntry msg;
            AppendRandomData(1024, msg.data);
            memcpy(_dataWritten[i], msg.data.data(), 1024);
            msg.tag = multiBlobTag; // First multiblob tag
            msg.index = i;
            msg.numTotalBlobs = numMultiBlobs;
            
            ExternalInterface::MessageGameToEngine message;
            message.Set_NVStorageWriteEntry(msg);
            SendMessage(message);
          }
          
          SET_TEST_STATE(ReadMultiBlob);
          break;
        }
        case TestState::ReadMultiBlob:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                                _writeAckd,
                                                _lastResult == NVStorage::NVResult::NV_OKAY)
          {
            ClearAcks();
            
            ExternalInterface::NVStorageReadEntry msg;
            msg.tag = multiBlobTag;
            
            ExternalInterface::MessageGameToEngine message;
            message.Set_NVStorageReadEntry(msg);
            SendMessage(message);
            
            SET_TEST_STATE(VerifyMultiBlob);
          }
          break;
        }
        case TestState::VerifyMultiBlob:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                _readAckd,
                                                _lastResult == NVStorage::NVResult::NV_OKAY)
          {
            ClearAcks();
            
            const std::vector<u8>* data = GetReceivedNVStorageData(multiBlobTag);
            
            for(int i = 0; i < numMultiBlobs; i++)
            {
              uint8_t d2[1024];
              memcpy(d2, data->data() + (i*1024), 1024);
              CST_ASSERT(IsDataSame(_dataWritten[i], d2, 1024),
                         "Data written to and read from multi blob is not the same");
            }
            CST_ASSERT(data->size() == numMultiBlobs*1024,
                       "Data read from multi blob is not expected word-aligned size");
            
            SET_TEST_STATE(EraseSingleBlob);
          }
          break;
        }
        case TestState::EraseSingleBlob:
        {
          // Erase
          ExternalInterface::NVStorageEraseEntry msg;
          msg.tag = singleBlobTag;
          
          ExternalInterface::MessageGameToEngine message;
          message.Set_NVStorageEraseEntry(msg);
          SendMessage(message);
          
          // Try to read
          ExternalInterface::NVStorageReadEntry msg1;
          msg1.tag = singleBlobTag;
          
          ExternalInterface::MessageGameToEngine message1;
          message1.Set_NVStorageReadEntry(msg1);
          SendMessage(message1);
        
          SET_TEST_STATE(VerifySingleErase);
          break;
        }
        case TestState::VerifySingleErase:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20, _eraseAckd, _readAckd)
          {
            ClearAcks();
            
            CST_ASSERT(_lastResult == NVStorage::NVResult::NV_NOT_FOUND, "Read data after erasing");
           
            SET_TEST_STATE(EraseMultiBlob);
          }
          break;
        }
        case TestState::EraseMultiBlob:
        {
          // Erase
          ExternalInterface::NVStorageEraseEntry msg;
          msg.tag = multiBlobTag;
          
          ExternalInterface::MessageGameToEngine message;
          message.Set_NVStorageEraseEntry(msg);
          SendMessage(message);
          
          // Try to read
          ExternalInterface::NVStorageReadEntry msg1;
          msg1.tag = multiBlobTag;
          
          ExternalInterface::MessageGameToEngine message1;
          message1.Set_NVStorageReadEntry(msg1);
          SendMessage(message1);
          
          SET_TEST_STATE(VerifyMultiErase);
        
          break;
        }
        case TestState::VerifyMultiErase:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20, _eraseAckd, _readAckd)
          {
            ClearAcks();
            
            CST_ASSERT(_lastResult == NVStorage::NVResult::NV_NOT_FOUND, "Read data after erasing");
            
            SET_TEST_STATE(WritingToInvalidMultiTag);
          }
          break;
        }
        case TestState::WritingToInvalidMultiTag:
        {
          ExternalInterface::NVStorageWriteEntry msg;
          AppendRandomData(1024, msg.data);
          msg.tag = (Tag)((uint32_t)multiBlobTag + 1); // Invalid multiblob tag
          msg.index = 0;
          msg.numTotalBlobs = 1;
          
          ExternalInterface::MessageGameToEngine message;
          message.Set_NVStorageWriteEntry(msg);
          SendMessage(message);
          
          SET_TEST_STATE(WriteData);
          break;
        }
        case TestState::WriteData:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                _writeAckd,
                                                _lastResult == NVStorage::NVResult::NV_BAD_ARGS)
          {
            ClearAcks();
            
            ExternalInterface::NVStorageWriteEntry msg;
            AppendRandomData(5, msg.data);
            msg.tag = singleBlobTag;
            msg.index = 0;
            msg.numTotalBlobs = 1;
            
            ExternalInterface::MessageGameToEngine message;
            message.Set_NVStorageWriteEntry(msg);
            SendMessage(message);
            
            for(int i = 0; i < numMultiBlobs; i++)
            {
              ExternalInterface::NVStorageWriteEntry msg;
              AppendRandomData(1024, msg.data);
              memcpy(_dataWritten[i], msg.data.data(), 1024);
              msg.tag = multiBlobTag; // First multiblob tag
              msg.index = i;
              msg.numTotalBlobs = numMultiBlobs;
              
              ExternalInterface::MessageGameToEngine message;
              message.Set_NVStorageWriteEntry(msg);
              SendMessage(message);
            }
            
            SET_TEST_STATE(WipeAll);
          }
          break;
        }
        case TestState::WipeAll:
        {
          // Wait for acks from the singleBlob write and the multi-blob write.
          IF_CONDITION_WITH_TIMEOUT_ASSERT(_numWrites == 2, 20)
          {
            ClearAcks();
            
            // Erase all
            ExternalInterface::NVStorageWipeAll msg1;
            
            ExternalInterface::MessageGameToEngine message1;
            message1.Set_NVStorageWipeAll(msg1);
            SendMessage(message1);
            
            // Try to read
            ExternalInterface::NVStorageReadEntry msg;
            msg.tag = singleBlobTag;
            
            ExternalInterface::MessageGameToEngine message;
            message.Set_NVStorageReadEntry(msg);
            SendMessage(message);
            
            SET_TEST_STATE(Final);
          }
          break;
        }
        case TestState::Final:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                _eraseAckd,
                                                _readAckd,
                                                _lastResult == NVStorage::NVResult::NV_NOT_FOUND)
          {
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
    
    
    // ================ Message handler callbacks ==================
    void CST_NVStorage::HandleNVStorageOpResult(const ExternalInterface::NVStorageOpResult &msg)
    {
      if(msg.op == NVStorage::NVOperation::NVOP_READ)
      {
        _readAckd = true;
      }
      else if(msg.op == NVStorage::NVOperation::NVOP_WRITE)
      {
        _writeAckd = true;
        _numWrites++;
      }
      else
      {
        _eraseAckd = true;
      }
      _lastResult = msg.result;
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Vector
} // end namespace Anki

