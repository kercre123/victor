#ifndef __CONTACTS_H
#define __CONTACTS_H

#include "messages.h"

namespace Contacts {
  extern bool dataQueued;
  void init(void);
  void tick(void);
  void forward(const ContactData& data);
}

#endif
