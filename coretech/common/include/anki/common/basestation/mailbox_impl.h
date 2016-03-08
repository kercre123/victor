/**
 * File: mailbox_impl.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014 (copied from original implementation on Cozmo robot)
 *
 * Description: Implementation of templated mailboxes -- see also mailboxes.h
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "mailbox.h"

namespace Anki {
  
  //
  // Templated Mailbox Implementations
  //
  template<typename MSG_TYPE>
  Mailbox<MSG_TYPE>::Mailbox()
  : beenRead_(true)
  {
    
  }

  template<typename MSG_TYPE>
  bool Mailbox<MSG_TYPE>::putMessage(const MSG_TYPE& newMsg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    message_  = newMsg;
    beenRead_ = false;
    return true;
  }
  
  template<typename MSG_TYPE>
  bool Mailbox<MSG_TYPE>::putMessage(MSG_TYPE&& newMsg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    message_  = newMsg;
    beenRead_ = false;
    return true;
  }

  template<typename MSG_TYPE>
  bool Mailbox<MSG_TYPE>::getMessage(MSG_TYPE& msgOut)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!beenRead_) {
      msgOut = message_;
      beenRead_ = true;
      return true;
    }
    return false;
  }


  //
  // Templated MultiMailbox Implementations
  //

  template<typename MSG_TYPE, u8 NUM_BOXES>
  MultiMailbox<MSG_TYPE, NUM_BOXES>::MultiMailbox()
  : readIndex_(0)
  , writeIndex_(0)
  {
    
  }
  
  template<typename MSG_TYPE, u8 NUM_BOXES>
  bool MultiMailbox<MSG_TYPE,NUM_BOXES>::putMessage(const MSG_TYPE& newMsg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if(mailboxes_[writeIndex_].putMessage(newMsg) == true) {
      advanceIndex(writeIndex_);
      return true;
    }
    return false;
  }
  
  
  template<typename MSG_TYPE, u8 NUM_BOXES>
  bool MultiMailbox<MSG_TYPE,NUM_BOXES>::putMessage(MSG_TYPE&& newMsg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if(mailboxes_[writeIndex_].putMessage(newMsg) == true) {
      advanceIndex(writeIndex_);
      return true;
    }
    return false;
  }

  template<typename MSG_TYPE, u8 NUM_BOXES>
  bool MultiMailbox<MSG_TYPE,NUM_BOXES>::getMessage(MSG_TYPE& msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if(mailboxes_[readIndex_].getMessage(msg) == true) {
      // we got a message out of the mailbox (it wasn't locked and there
      // was something in it), so move to the next mailbox
      advanceIndex(readIndex_);
      return true;
    }
    return false;
  }

  template<typename MSG_TYPE, u8 NUM_BOXES>
  void MultiMailbox<MSG_TYPE,NUM_BOXES>::advanceIndex(u8 &index)
  {
    ++index;
    if(index == NUM_BOXES) {
      index = 0;
    }
  }

} // namespace Anki

