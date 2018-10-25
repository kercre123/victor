/**
 * File: alexaImpl.h
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Component that integrates with the Alexa Voice Service (AVS) SDK
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXAIMPL_H
#define ANIMPROCESS_COZMO_ALEXAIMPL_H

#include "util/helpers/noncopyable.h"
#include <functional>
#include <string>

namespace Anki {
namespace Vector {
  
enum class AlexaAuthState : uint8_t;
class AnimContext;

class AlexaImpl : private Util::noncopyable
{
public:
  
  AlexaImpl();
  ~AlexaImpl();
  
  bool Init(const AnimContext* context);
  
  void Update();
  
  // Callback setters
  using OnAlexaAuthStateChanged = std::function<void(AlexaAuthState, const std::string&, const std::string&)>;
  void SetOnAlexaAuthStateChanged( const OnAlexaAuthStateChanged& callback ) { _onAlexaAuthStateChanged = callback; }
  
private:
  
  const AnimContext* _context = nullptr;
  std::string _alexaFolder;
  
  // callbacks
  OnAlexaAuthStateChanged _onAlexaAuthStateChanged;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAIMPL_H
