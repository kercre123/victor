/**
 * File: runTransferTestMain.h
 *
 * Author: Molly Jameson
 * Date:   2/1/2016
 *
 * Description: Unit tests for transfer library
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

/*
#include "gtest/gtest.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
*/
//

#include "testTransferQueueMgr.h"
#include "testTransferUL.h"
#include "testTransferDL.h"

#include "../http/httpAdapter_osx_ios.h"


//Anki::Util::PrintfLoggerProvider* loggerProvider = nullptr;
/*
GTEST_TEST(TransferQueueTests, TestTheTestTransfer)
{
  int i = 0;
  int j = 0;
  EXPECT_EQ(i,j);
}

GTEST_TEST(TransferQueueTests, TestUploadPush)
{
  // Create testTransferUL, and set some data
  // Global Context, Transfer Mgr does a pull
  // verify the pull go the same data as the push
  
  // Should be in global context
  Anki::Util::TestTransferQueueMgr tranfer_mgr;
  {
    //Anki::Util::TestTransferUL* p_upload_service = new Anki::Util::TestTransferUL(this);
    Anki::Util::TestTransferUL* p_upload_service = new Anki::Util::TestTransferUL(&tranfer_mgr);
    p_upload_service->SetData("some_saved_string2");
    tranfer_mgr.StartDataPull();
    
    
    delete p_upload_service;
    
    // A generic service that would want a delayed upload, like a remote leaderboard.
//    Anki::Util::TestTransferUL upload_service(&tranfer_mgr);
//    upload_service.SetData("some_saved_string");
    //std::shared_ptr<Anki::Util::TransferQueueMgr> transferQueueMgr(&tranfer_mgr);
    tranfer_mgr.InitServices();
    // In the real project, a derived "CozmoTransferQueueMgr" will create this
    
  }
  
  tranfer_mgr.StartDataPull();
  
}

GTEST_TEST(TransferQueueTests, TestDownloadPull)
{
  // Create testTransferDL
  // Global Context, Transfer Mgr does a push
  // verify the pull go the same data as the push
  Anki::Util::TestTransferQueueMgr tranfer_mgr;
  Anki::Util::HttpAdapter native_http;
  Anki::Util::TestTransferDL download_service(&tranfer_mgr,&native_http);
  download_service.RequestData(&native_http);
}

int main(int argc, char * argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  printf("Transfer Lib Tests running\n");
  return RUN_ALL_TESTS();
}
*/

#import <Foundation/Foundation.h>

int main(int argc, char * argv[])
{
  printf("adsflkj\n");
  Anki::Util::TestTransferQueueMgr tranfer_mgr;
  Anki::Util::HttpAdapter native_http;
  Anki::Util::TestTransferDL download_service(&tranfer_mgr,&native_http);
  download_service.RequestData(&native_http);
  printf("Pass\n");
  int i = 0;
  while(1)
  {
    ++i;
  }
  return 0;
}
