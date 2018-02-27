/**
*  componentTypeEnumMap.h
*
*  Created by Kevin M. Karol on 2/12/18
*  Copyright (c) 2018 Anki, Inc. All rights reserved.
*
*  Map component types to their enum values to allow typesafe storage/access
*  within generic components
*  
**/

#ifndef __Util_EntityComponent_ComponentTypeEnumMap_H__
#define __Util_EntityComponent_ComponentTypeEnumMap_H__


namespace Anki {

template<typename EnumType, typename ClassType>
void GetComponentIDForType(EnumType& enumToSet);

} // namespace Anki

#endif // __Util_EntityComponent_ComponentTypeEnumMap_H__
