/**
 * File: connectionFlow.h
 *
 * Author: Al Chaussee
 * Created: 02/28/2018
 *
 * Description: Functions for updating what to display on the face
 *              during various parts of the connection flow
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef ANIM_CONNECTION_FLOW_H
#define ANIM_CONNECTION_FLOW_H

#include <string>

#ifdef USES_CLAD_CPPLITE
#define CLAD_VECTOR(ns) CppLite::Anki::Vector::ns
#else
#define CLAD_VECTOR(ns) ns
#endif

#ifdef USES_CLAD_CPPLITE
namespace CppLite {
#endif
namespace Anki {
namespace Vector {
namespace SwitchboardInterface {
  struct SetConnectionStatus;
}
}
}
#ifdef USES_CLAD_CPPLITE
}
#endif

namespace Anki {
namespace Vector {
namespace Anim {
  class AnimContext;
  class AnimationStreamer;
}

void SetBLEPin(uint32_t pin);

bool InitConnectionFlow(Anim::AnimationStreamer* animStreamer);

void UpdateConnectionFlow(const CLAD_VECTOR(SwitchboardInterface)::SetConnectionStatus& msg,
                          Anim::AnimationStreamer* animStreamer,
                          const Anim::AnimContext* context);

}
}

#undef CLAD_VECTOR

#endif
