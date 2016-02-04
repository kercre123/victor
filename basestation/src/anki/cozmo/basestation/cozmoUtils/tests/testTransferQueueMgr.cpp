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

#include "testTransferQueueMgr.h"
#include "testTransferUL.h"
#include "testTransferDL.h"

namespace Anki {
  
  namespace Util {
    
    void TestTransferQueueMgr::InitServices()
    {
      /*Anki::Util::TestTransferUL* p_upload_service = new Anki::Util::TestTransferUL(this);
      p_upload_service->SetData("some_saved_string2");
      delete p_upload_service;*/
      
      
      /*std::string str;
      std::string str2;
      p_upload_service->PushData(str,str2);*/
    }

  } // namespace Cozmo
} // namespace Anki