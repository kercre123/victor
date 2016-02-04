/**
 * File: testTransferDL.h
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

#ifndef ANKI_COZMO_TEST_TRANSFER_DL_H
#define ANKI_COZMO_TEST_TRANSFER_DL_H


 #include "../ITransferable.h"
#include "../http/abstractHttpAdapter.h"
#include "util/dispatchQueue/dispatchQueue.h"

namespace Anki {
  
  namespace Util {
    
    
    class TestTransferDL : public ITransferable
    {
    public:
      TestTransferDL(TransferQueueMgr* transferQueueMgr, IHttpAdapter* httpAdapter);
      ~TestTransferDL();
      virtual bool RequestData(IHttpAdapter* httpAdapter);
    protected:
      Util::Dispatch::Queue* _dispatchQueue;
    }; // class
    
    
  } // namespace Util
} // namespace Anki

#endif 