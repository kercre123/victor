/**
 * File: mailbox.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014 (copied from original implementation on Cozmo robot)
 *
 * Description: "Mailboxes" are used for leaving messages from one thread to another.
 *
 *   There is no use of STL, so these are potentially suitable for use in an
 *   embedded environment, so long as templates are allowed.
 *
 *   Templated implementations are in mailbox_impl.h
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_CORETECH_COMMON_MAILBOX_H
#define ANKI_CORETECH_COMMON_MAILBOX_H


#include <mutex>

namespace Anki {
  
  // Single-message Mailbox Class
  template<typename MsgType>
  class Mailbox
  {
  public:
    
    Mailbox();
    
    bool putMessage(const MsgType& newMsg);
    bool putMessage(MsgType&& newMsg);
    bool getMessage(MsgType& msgOut);
    
  protected:
    MsgType message_;
    bool    beenRead_;
    std::mutex mutex_;
  };
  
  // Multiple-message Mailbox Class
  template<typename MSG_TYPE, u8 NUM_BOXES>
  class MultiMailbox
  {
  public:
    
    MultiMailbox();
    
    bool putMessage(const MSG_TYPE& newMsg);
    bool putMessage(MSG_TYPE&& newMsg);
    bool getMessage(MSG_TYPE& msg);
    
  protected:
    Mailbox<MSG_TYPE> mailboxes_[NUM_BOXES];
    u8 readIndex_, writeIndex_;
    std::mutex mutex_;
    
    void advanceIndex(u8 &index);
  };
  
} // namespace Anki

#endif // ANKI_CORETECH_COMMON_MAILBOX_H
