/*
 * File: hijackFxFactory.h
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: Hijack Plug-in common header to hook the plug-in into Wwise’s plug-in create & set parameters
 *              function pointers.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __AnkiAudio_PlugIns_HijackFxFactory_H__
#define __AnkiAudio_PlugIns_HijackFxFactory_H__

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <cstdint>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

#if !defined AK_3DS && !defined AK_VITA_HW
	AK_STATIC_LINK_PLUGIN(HijackFx)
#endif

} // PlugIns
} // AudioEngine
} // Anki

#endif // EXCLUDE_ANKI_AUDIO_LIBS
#endif /* __AnkiAudio_PlugIns_HijackFxFactory_H__ */
