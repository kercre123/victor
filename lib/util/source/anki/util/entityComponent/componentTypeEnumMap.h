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

#include <string>

namespace Anki {

// Macro to make template specialization less verbose
#define LINK_COMPONENT_TYPE_TO_ENUM(componentType, enumType, enumValue) \
template<>\
void GetComponentIDForType<Vector::enumType, Vector::componentType>(Vector::enumType& enumToSet){enumToSet = Vector::enumType::enumValue;}

template<typename EnumType, typename ClassType>
void GetComponentIDForType(EnumType& enumToSet);

template<typename EnumType>
std::string GetEntityNameForEnumType();

template<typename EnumType>
std::string GetComponentStringForID(EnumType enumID);

} // namespace Anki

#endif // __Util_EntityComponent_ComponentTypeEnumMap_H__
