/**
 * File: testTransferUL.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase for push data from the server.
 *              An example might be wanting to upload a save file to some service for a public facing
 *              achievements leaderboard website to look at.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_TEST_TRANSFER_UL_H
#define ANKI_COZMO_TEST_TRANSFER_UL_H

#include "../ITransferable.h"
#include <string>

namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr;
    
    class TestTransferUL : public ITransferable
    {
    public:
      TestTransferUL(TransferQueueMgr* transferQueueMgr);
      virtual ~TestTransferUL();
      virtual bool SendData( TransferQueueMgr::NativeSendFunc func_native_send );
      
      void SetData(std::string str);
      std::string GetData() { return m_Data; }
      
    protected:
      std::string m_Data;
      
    }; // class
    
    
  } // namespace Util
} // namespace Anki

#endif 