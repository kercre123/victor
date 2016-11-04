/**
 * File: petWorld.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 10-24-2016
 *
 * Description: Implements a container for mirroring on the main thread any pet faces
 *              detected on the vision system thread.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace  Anki {
namespace Cozmo {
  
PetWorld::PetWorld(Robot& robot)
: _robot(robot)
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PetWorld::Update(const std::list<Vision::TrackedPet>& pets)
{
  // For now, we just keep up with what was seen in the most recent image,
  // while maintaining numTimesObserved
  {
    PetContainer newKnownPets;
    
    for(auto & petDetection : pets)
    {
      // Insert into the new map
      auto inserted = newKnownPets.emplace(petDetection.GetID(), petDetection);
      
      auto existingIter = _knownPets.find(petDetection.GetID());
      if(existingIter != _knownPets.end())
      {
        // If we already knew about this ID, keep and increment its num times seen
        inserted.first->second.SetNumTimesObserved(existingIter->second.GetNumTimesObserved() + 1);
      }
      else
      {
        // First time seen: initialze num times observed
        inserted.first->second.SetNumTimesObserved(1);
      }
      
    }
    
    std::swap(newKnownPets, _knownPets);
  }
  
  // Now that knownPets is upated, Broadcast and Visualize
  for(auto const& knownPetEntry : _knownPets)
  {
    const Vision::TrackedPet& knownPet = knownPetEntry.second;
    
    // Log to DAS if this is the first detection:
    if(knownPet.GetNumTimesObserved() == 1)
    {
      Util::sEventF("robot.vision.detected_pet", {{DDATA, EnumToString(knownPet.GetType())}}, "%d", knownPet.GetID());
    }
    
    // Broadcast the detection for Game/SDK
    {
      using namespace ExternalInterface;
      
      RobotObservedPet observedPet(knownPet.GetID(),
                                   knownPet.GetTimeStamp(),
                                   knownPet.GetNumTimesObserved(),
                                   knownPet.GetScore(),
                                   CladRect(knownPet.GetRect().GetX(),
                                            knownPet.GetRect().GetY(),
                                            knownPet.GetRect().GetWidth(),
                                            knownPet.GetRect().GetHeight()),
                                   knownPet.GetType());
      
      _robot.Broadcast(MessageEngineToGame(std::move(observedPet)));
    }
    
    // Visualize the detection
    {
      const ColorRGBA& vizColor = ColorRGBA::CreateFromColorIndex(knownPet.GetID());
      _robot.GetContext()->GetVizManager()->DrawCameraOval(Point2f(knownPet.GetRect().GetXmid(),
                                                                   knownPet.GetRect().GetYmid()),
                                                           knownPet.GetRect().GetWidth() * 0.5f,
                                                           knownPet.GetRect().GetHeight() * 0.5f,
                                                           vizColor);
      
      
      
      const s32 kStrLen = 16;
      char strbuffer[kStrLen];
      snprintf(strbuffer, kStrLen, "%s%d[%d]",
               knownPet.GetType() == Vision::PetType::Cat ? "CAT" : "DOG",
               knownPet.GetID(),
               knownPet.GetNumTimesObserved());
      
      _robot.GetContext()->GetVizManager()->DrawCameraText(Point2f(knownPet.GetRect().GetX(),
                                                                   knownPet.GetRect().GetY()),
                                                           strbuffer, vizColor);
    }
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<Vision::FaceID_t> PetWorld::GetKnownPetsWithType(Vision::PetType type) const
{
  std::set<Vision::FaceID_t> matchingPets;
  for(auto & pet : _knownPets)
  {
    if(Vision::PetType::Unknown == type || pet.second.GetType() == type)
    {
      matchingPets.insert(pet.first);
    }
  }
  
  return matchingPets;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::TrackedPet* PetWorld::GetPetByID(Vision::FaceID_t faceID) const
{
  auto iter = _knownPets.find(faceID);
  if(iter == _knownPets.end())
  {
    return nullptr;
  }
  else
  {
    return &iter->second;
  }
}

} // namespace Cozmo
} // namespace Anki
