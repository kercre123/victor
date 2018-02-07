/**
*  iManageableComponent.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Defines a decorator for manageable components
*  
**/

#ifndef __Util_EntityComponent_IManageableComponent_H__
#define __Util_EntityComponent_IManageableComponent_H__

namespace Anki {

class IManageableComponent{
public:
  virtual ~IManageableComponent(){};
};

} // namespace Anki

#endif // __Util_EntityComponent_IManageableComponent_H__
