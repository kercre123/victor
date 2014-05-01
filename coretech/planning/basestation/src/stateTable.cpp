/**
 * File: stateTable.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: Table of entries for each expanded state
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#include "stateTable.h"

namespace Anki
{
namespace Planning
{

StateTable::StateTable()
{
}

void StateTable::Clear()
{
  table_.clear();
}


}
}
