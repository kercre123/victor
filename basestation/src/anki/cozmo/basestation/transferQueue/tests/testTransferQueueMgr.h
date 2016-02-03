/**
 * File: testTransferQueueMgr.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase for push data from the server.
 *              An example might be config downloads or just text banner upsells on the main screen.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_UNIT_TEST_TRANSFER_H
#define ANKI_COZMO_UNIT_TEST_TRANSFER_H


 #include "../TransferQueueMgr.h"

namespace Anki {
  
  namespace Util {
    
    
    class TestTransferQueueMgr : public TransferQueueMgr
    {
    public:
      void InitServices();
    protected:
      
    }; // class
    
    
  } // namespace Util
} // namespace Anki
#endif 