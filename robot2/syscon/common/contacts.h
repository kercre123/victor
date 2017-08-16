#ifndef __CONTACTS_H
#define __CONTACTS_H

#include "messages.h"

using namespace Anki::Cozmo::Spine;

namespace Contacts {
  void init(void);
  void forward(const ContactData& data);
  bool transmit(ContactData& data);
}

#endif
