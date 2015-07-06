cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8.8)
project(util)
set(configuration ".")
enable_language(ASM)
set(builddir "${CMAKE_BINARY_DIR}")
set(obj "${builddir}/obj")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)


#util.gyp:kazmath#target
set(TARGET "kazmath")
set(TOOLSET "target")
list(APPEND kazmath__other_srcs
  "anki-util/source/3rd/kazmath/src/kazmath.h"
  "anki-util/source/3rd/kazmath/src/aabb2.h"
  "anki-util/source/3rd/kazmath/src/aabb3.h"
  "anki-util/source/3rd/kazmath/src/GL/mat4stack.h"
  "anki-util/source/3rd/kazmath/src/GL/matrix.h"
  "anki-util/source/3rd/kazmath/src/mat3.h"
  "anki-util/source/3rd/kazmath/src/mat4.h"
  "anki-util/source/3rd/kazmath/src/plane.h"
  "anki-util/source/3rd/kazmath/src/quaternion.h"
  "anki-util/source/3rd/kazmath/src/ray2.h"
  "anki-util/source/3rd/kazmath/src/ray3.h"
  "anki-util/source/3rd/kazmath/src/utility.h"
  "anki-util/source/3rd/kazmath/src/vec2.h"
  "anki-util/source/3rd/kazmath/src/vec3.h"
  "anki-util/source/3rd/kazmath/src/vec4.h")
add_library(kazmath STATIC ${kazmath__other_srcs}
  anki-util/source/3rd/kazmath/src/aabb2.c
  anki-util/source/3rd/kazmath/src/aabb3.c
  anki-util/source/3rd/kazmath/src/GL/mat4stack.c
  anki-util/source/3rd/kazmath/src/GL/matrix.c
  anki-util/source/3rd/kazmath/src/mat3.c
  anki-util/source/3rd/kazmath/src/mat4.c
  anki-util/source/3rd/kazmath/src/plane.c
  anki-util/source/3rd/kazmath/src/quaternion.c
  anki-util/source/3rd/kazmath/src/ray2.c
  anki-util/source/3rd/kazmath/src/ray3.c
  anki-util/source/3rd/kazmath/src/utility.c
  anki-util/source/3rd/kazmath/src/vec2.c
  anki-util/source/3rd/kazmath/src/vec3.c
  anki-util/source/3rd/kazmath/src/vec4.c
)
set_source_files_properties(${kazmath__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(kazmath PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/project/gyp")
set_target_properties(kazmath PROPERTIES PREFIX "lib")
set_target_properties(kazmath PROPERTIES ARCHIVE_OUTPUT_NAME "kazmath")
set_target_properties(kazmath PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/project/gyp/libkazmath.a PROPERTIES GENERATED "TRUE")
set_target_properties(kazmath PROPERTIES EXCLUDE_FROM_ALL "FALSE")
list(APPEND kazmath__include_dirs
  "${CMAKE_SOURCE_DIR}/anki-util/tools/message-buffers/support/cpp/include"
  "${CMAKE_SOURCE_DIR}/anki-util/source/anki"
  "${CMAKE_SOURCE_DIR}/anki-util/libs/packaged/include"
  "${CMAKE_SOURCE_DIR}/anki-util/source/3rd")
set_property(TARGET kazmath APPEND PROPERTY INCLUDE_DIRECTORIES ${kazmath__include_dirs})
set_target_properties(kazmath PROPERTIES COMPILE_FLAGS "-std=c11 -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g ")
set_target_properties(kazmath PROPERTIES LINK_FLAGS "-g ")
unset(TOOLSET)
unset(TARGET)

#util.gyp:jsoncpp#target
set(TARGET "jsoncpp")
set(TOOLSET "target")
list(APPEND jsoncpp__other_srcs
  "anki-util/source/3rd/jsoncpp/json/json.h"
  "anki-util/source/3rd/jsoncpp/json/json-forwards.h")
add_library(jsoncpp STATIC ${jsoncpp__other_srcs}
  anki-util/source/3rd/jsoncpp/jsoncpp.cpp
)
set_source_files_properties(${jsoncpp__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(jsoncpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/project/gyp")
set_target_properties(jsoncpp PROPERTIES PREFIX "lib")
set_target_properties(jsoncpp PROPERTIES ARCHIVE_OUTPUT_NAME "jsoncpp")
set_target_properties(jsoncpp PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/project/gyp/libjsoncpp.a PROPERTIES GENERATED "TRUE")
set_target_properties(jsoncpp PROPERTIES EXCLUDE_FROM_ALL "FALSE")
list(APPEND jsoncpp__include_dirs
  "${CMAKE_SOURCE_DIR}/anki-util/tools/message-buffers/support/cpp/include"
  "${CMAKE_SOURCE_DIR}/anki-util/source/3rd/jsoncpp")
set_property(TARGET jsoncpp APPEND PROPERTY INCLUDE_DIRECTORIES ${jsoncpp__include_dirs})
set_target_properties(jsoncpp PROPERTIES COMPILE_FLAGS "-std=c11 -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g -std=c++11 -stdlib=libc++ -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g ")
set_target_properties(jsoncpp PROPERTIES LINK_FLAGS "-g ")
unset(TOOLSET)
unset(TARGET)

#util.gyp:util#target
set(TARGET "util")
set(TOOLSET "target")
list(APPEND util__other_srcs
  "anki-util/source/anki/util/bitFlags/bitFlags.h"
  "anki-util/source/anki/util/color/color.h"
  "anki-util/source/anki/util/console/consoleArgument.h"
  "anki-util/source/anki/util/console/consoleArgument_priv.h"
  "anki-util/source/anki/util/console/consoleChannel.h"
  "anki-util/source/anki/util/console/consoleChannelFile.h"
  "anki-util/source/anki/util/console/consoleFunction.h"
  "anki-util/source/anki/util/console/consoleInterface.h"
  "anki-util/source/anki/util/console/consoleMacro.h"
  "anki-util/source/anki/util/console/consoleSupportedTypes.def"
  "anki-util/source/anki/util/console/consoleSystem.h"
  "anki-util/source/anki/util/console/consoleSystemAPI.h"
  "anki-util/source/anki/util/console/consoleTypes.h"
  "anki-util/source/anki/util/console/consoleVariable.h"
  "anki-util/source/anki/util/container/slotArray.h"
  "anki-util/source/anki/util/crc/crc.h"
  "anki-util/source/anki/util/dataPacking/dataPacking.h"
  "anki-util/source/anki/util/debug/messageDebugging.h"
  "anki-util/source/anki/util/dispatchQueue/dispatchQueue.h"
  "anki-util/source/anki/util/dispatchQueue/iTaskHandle.h"
  "anki-util/source/anki/util/dispatchQueue/taskExecutor.h"
  "anki-util/source/anki/util/export/export.h"
  "anki-util/source/anki/util/fileUtils/fileUtils.h"
  "anki-util/source/anki/util/global/globalDefinitions.h"
  "anki-util/source/anki/util/hashing/hashing.h"
  "anki-util/source/anki/util/helpers/arrayHelpers.h"
  "anki-util/source/anki/util/helpers/assertHelpers.h"
  "anki-util/source/anki/util/helpers/base64.h"
  "anki-util/source/anki/util/helpers/boundedWhile.h"
  "anki-util/source/anki/util/helpers/callbackProvider.h"
  "anki-util/source/anki/util/helpers/cConversionHelpers.h"
  "anki-util/source/anki/util/helpers/cppToCConversionHelpers.h"
  "anki-util/source/anki/util/helpers/cWrappers.h"
  "anki-util/source/anki/util/helpers/enumToString.h"
  "anki-util/source/anki/util/helpers/includeFstream.h"
  "anki-util/source/anki/util/helpers/includeGTest.h"
  "anki-util/source/anki/util/helpers/includeIostream.h"
  "anki-util/source/anki/util/helpers/includeIstream.h"
  "anki-util/source/anki/util/helpers/includeOstream.h"
  "anki-util/source/anki/util/helpers/includeSstream.h"
  "anki-util/source/anki/util/helpers/noncopyable.h"
  "anki-util/source/anki/util/helpers/templateHelpers.h"
  "anki-util/source/anki/util/helpers/vectorToArray.h"
  "anki-util/source/anki/util/jsonWriter/jsonWriter.h"
  "anki-util/source/anki/util/logging/channelFilter.h"
  "anki-util/source/anki/util/logging/eventKeys.h"
  "anki-util/source/anki/util/logging/iEntityLoggerComponent.h"
  "anki-util/source/anki/util/logging/iEventProvider.h"
  "anki-util/source/anki/util/logging/iLoggerProvider.h"
  "anki-util/source/anki/util/logging/iTickTimeProvider.h"
  "anki-util/source/anki/util/logging/logging.h"
  "anki-util/source/anki/util/logging/printfLoggerProvider.h"
  "anki-util/source/anki/util/math/math.h"
  "anki-util/source/anki/util/math/mathStructures.h"
  "anki-util/source/anki/util/md5/md5.h"
  "anki-util/source/anki/util/messageDispatch/messageDispatch_fwd.h"
  "anki-util/source/anki/util/messageDispatch/messageDispatcher.h"
  "anki-util/source/anki/util/messageDispatch/messageReceiver.h"
  "anki-util/source/anki/util/parsingConstants/namedConstants.def"
  "anki-util/source/anki/util/parsingConstants/namedConstants.h"
  "anki-util/source/anki/util/parsingConstants/parsingConstants.def"
  "anki-util/source/anki/util/parsingConstants/parsingConstants.h"
  "anki-util/source/anki/util/parsingConstants/soundNames.def"
  "anki-util/source/anki/util/ptree/includeJSONParser.h"
  "anki-util/source/anki/util/ptree/includePtree.h"
  "anki-util/source/anki/util/ptree/ptreeKey.h"
  "anki-util/source/anki/util/ptree/ptreeMandatory.h"
  "anki-util/source/anki/util/ptree/ptreeToJSON.h"
  "anki-util/source/anki/util/ptree/ptreeTools.h"
  "anki-util/source/anki/util/ptree/ptreeTraverser.h"
  "anki-util/source/anki/util/random/randomGenerator.h"
  "anki-util/source/anki/util/signals/signalHolder.h"
  "anki-util/source/anki/util/signals/simpleSignal.hpp"
  "anki-util/source/anki/util/signals/simpleSignal_fwd.h"
  "anki-util/source/anki/util/singleton/dynamicSingleton.h"
  "anki-util/source/anki/util/statEngine/abstractAction.h"
  "anki-util/source/anki/util/statEngine/abstractCondition.h"
  "anki-util/source/anki/util/statEngine/combinationCondition.h"
  "anki-util/source/anki/util/statEngine/countThresholdCondition.h"
  "anki-util/source/anki/util/statEngine/dateCondition.h"
  "anki-util/source/anki/util/statEngine/multiAction.h"
  "anki-util/source/anki/util/statEngine/noticeAction.h"
  "anki-util/source/anki/util/statEngine/resetAction.h"
  "anki-util/source/anki/util/statEngine/statEngine.h"
  "anki-util/source/anki/util/statEngine/statEngineMessages.h"
  "anki-util/source/anki/util/statEngine/statNotice.h"
  "anki-util/source/anki/util/statEngine/statRule.h"
  "anki-util/source/anki/util/statEngine/statRuleBuilder.h"
  "anki-util/source/anki/util/statEngine/timeCondition.h"
  "anki-util/source/anki/util/stats/recentStatsAccumulator.h"
  "anki-util/source/anki/util/stats/runningStat.h"
  "anki-util/source/anki/util/stats/statsAccumulator.h"
  "anki-util/source/anki/util/stringTable/hashStringTable.h"
  "anki-util/source/anki/util/stringTable/stringID.h"
  "anki-util/source/anki/util/stringTable/stringTable.h"
  "anki-util/source/anki/util/testing/testing.h"
  "anki-util/source/anki/util/time/stepTimers.h"
  "anki-util/source/anki/util/time/stopWatch.h"
  "anki-util/source/anki/util/time/universalTime.h"
  "anki-util/source/anki/util/transport/fakeUDPSocket.h"
  "anki-util/source/anki/util/transport/iIpRetriever.h"
  "anki-util/source/anki/util/transport/iNetTransport.h"
  "anki-util/source/anki/util/transport/iNetTransportDataReceiver.h"
  "anki-util/source/anki/util/transport/iUDPSocket.h"
  "anki-util/source/anki/util/transport/iUnreliableTransport.h"
  "anki-util/source/anki/util/transport/netEmulatorUDPSocket.h"
  "anki-util/source/anki/util/transport/packetByteArray.h"
  "anki-util/source/anki/util/transport/pendingMultiPartMessage.h"
  "anki-util/source/anki/util/transport/pendingReliableMessage.h"
  "anki-util/source/anki/util/transport/reliableConnection.h"
  "anki-util/source/anki/util/transport/reliableMessageTypes.h"
  "anki-util/source/anki/util/transport/reliableSequenceId.h"
  "anki-util/source/anki/util/transport/reliableTransport.h"
  "anki-util/source/anki/util/transport/srcBufferSet.h"
  "anki-util/source/anki/util/transport/transportAddress.h"
  "anki-util/source/anki/util/transport/transportStats.h"
  "anki-util/source/anki/util/transport/udpTransport.h"
  "anki-util/source/anki/util/utilMessaging/utilMessaging.h"
  "anki-util/source/anki/util/UUID/UUID.h")
add_library(util STATIC ${util__other_srcs}
  anki-util/source/anki/util/color/color.cpp
  anki-util/source/anki/util/console/consoleArgument.cpp
  anki-util/source/anki/util/console/consoleChannel.cpp
  anki-util/source/anki/util/console/consoleChannelFile.cpp
  anki-util/source/anki/util/console/consoleFunction.cpp
  anki-util/source/anki/util/console/consoleSystem.cpp
  anki-util/source/anki/util/console/consoleSystemAPI.cpp
  anki-util/source/anki/util/console/consoleVariable.cpp
  anki-util/source/anki/util/crc/crc.c
  anki-util/source/anki/util/dataPacking/dataPacking.cpp
  anki-util/source/anki/util/debug/messageDebugging.cpp
  anki-util/source/anki/util/dispatchQueue/dispatchQueue.cpp
  anki-util/source/anki/util/dispatchQueue/taskExecutor.cpp
  anki-util/source/anki/util/fileUtils/fileUtils.cpp
  anki-util/source/anki/util/global/globalDefinitions.cpp
  anki-util/source/anki/util/hashing/hashing.cpp
  anki-util/source/anki/util/helpers/base64.cpp
  anki-util/source/anki/util/helpers/callbackProvider.cpp
  anki-util/source/anki/util/helpers/cConversionHelpers.cpp
  anki-util/source/anki/util/helpers/cppToCConversionHelpers.cpp
  anki-util/source/anki/util/jsonWriter/jsonWriter.cpp
  anki-util/source/anki/util/logging/channelFilter.cpp
  anki-util/source/anki/util/logging/eventKeys.cpp
  anki-util/source/anki/util/logging/logging.cpp
  anki-util/source/anki/util/logging/printfLoggerProvider.cpp
  anki-util/source/anki/util/math/mathStructures.cpp
  anki-util/source/anki/util/md5/md5.cpp
  anki-util/source/anki/util/parsingConstants/namedConstants.cpp
  anki-util/source/anki/util/parsingConstants/parsingConstants.cpp
  anki-util/source/anki/util/ptree/includePtree.cpp
  anki-util/source/anki/util/ptree/ptreeKey.cpp
  anki-util/source/anki/util/ptree/ptreeMandatory.cpp
  anki-util/source/anki/util/ptree/ptreeToJSON.cpp
  anki-util/source/anki/util/ptree/ptreeTools.cpp
  anki-util/source/anki/util/ptree/ptreeTraverser.cpp
  anki-util/source/anki/util/random/randomGenerator.cpp
  anki-util/source/anki/util/statEngine/abstractCondition.cpp
  anki-util/source/anki/util/statEngine/combinationCondition.cpp
  anki-util/source/anki/util/statEngine/countThresholdCondition.cpp
  anki-util/source/anki/util/statEngine/dateCondition.cpp
  anki-util/source/anki/util/statEngine/multiAction.cpp
  anki-util/source/anki/util/statEngine/noticeAction.cpp
  anki-util/source/anki/util/statEngine/resetAction.cpp
  anki-util/source/anki/util/statEngine/statEngine.cpp
  anki-util/source/anki/util/statEngine/statEngineMessages.cpp
  anki-util/source/anki/util/statEngine/statNotice.cpp
  anki-util/source/anki/util/statEngine/statRule.cpp
  anki-util/source/anki/util/statEngine/statRuleBuilder.cpp
  anki-util/source/anki/util/statEngine/timeCondition.cpp
  anki-util/source/anki/util/stats/recentStatsAccumulator.cpp
  anki-util/source/anki/util/stats/runningStat.cpp
  anki-util/source/anki/util/stringTable/hashStringTable.cpp
  anki-util/source/anki/util/stringTable/stringID.cpp
  anki-util/source/anki/util/stringTable/stringTable.cpp
  anki-util/source/anki/util/time/stepTimers.cpp
  anki-util/source/anki/util/time/stopWatch.cpp
  anki-util/source/anki/util/time/universalTime.cpp
  anki-util/source/anki/util/transport/fakeUDPSocket.cpp
  anki-util/source/anki/util/transport/iNetTransportDataReceiver.cpp
  anki-util/source/anki/util/transport/netEmulatorUDPSocket.cpp
  anki-util/source/anki/util/transport/packetByteArray.cpp
  anki-util/source/anki/util/transport/pendingMultiPartMessage.cpp
  anki-util/source/anki/util/transport/pendingReliableMessage.cpp
  anki-util/source/anki/util/transport/reliableConnection.cpp
  anki-util/source/anki/util/transport/reliableMessageTypes.cpp
  anki-util/source/anki/util/transport/reliableSequenceId.cpp
  anki-util/source/anki/util/transport/reliableTransport.cpp
  anki-util/source/anki/util/transport/srcBufferSet.cpp
  anki-util/source/anki/util/transport/transportAddress.cpp
  anki-util/source/anki/util/transport/transportStats.cpp
  anki-util/source/anki/util/transport/udpTransport.cpp
  anki-util/source/anki/util/utilMessaging/utilMessaging.c
  anki-util/source/anki/util/UUID/UUID.c
  anki-util/tools/message-buffers/support/cpp/source/SafeMessageBuffer.cpp
)
set_source_files_properties(${util__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(util PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/project/gyp")
set_target_properties(util PROPERTIES PREFIX "lib")
set_target_properties(util PROPERTIES ARCHIVE_OUTPUT_NAME "util")
set_target_properties(util PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/project/gyp/libutil.a PROPERTIES GENERATED "TRUE")
set_target_properties(util PROPERTIES EXCLUDE_FROM_ALL "FALSE")
list(APPEND util__include_dirs
  "${CMAKE_SOURCE_DIR}/anki-util/tools/message-buffers/support/cpp/include"
  "${CMAKE_SOURCE_DIR}/anki-util/source/anki"
  "${CMAKE_SOURCE_DIR}/anki-util/libs/packaged/include"
  "${CMAKE_SOURCE_DIR}/anki-util/source/3rd"
  "${CMAKE_SOURCE_DIR}/anki-util/source/3rd/jsoncpp"
  "${CMAKE_SOURCE_DIR}/anki-util/source/3rd")
set_property(TARGET util APPEND PROPERTY INCLUDE_DIRECTORIES ${util__include_dirs})
set_source_files_properties(
  anki-util/source/anki/util/crc/crc.c
  anki-util/source/anki/util/utilMessaging/utilMessaging.c
  anki-util/source/anki/util/UUID/UUID.c
 PROPERTIES
  COMPILE_FLAGS "-std=c11 -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g "
)
set_source_files_properties(
  anki-util/source/anki/util/color/color.cpp
  anki-util/source/anki/util/console/consoleArgument.cpp
  anki-util/source/anki/util/console/consoleChannel.cpp
  anki-util/source/anki/util/console/consoleChannelFile.cpp
  anki-util/source/anki/util/console/consoleFunction.cpp
  anki-util/source/anki/util/console/consoleSystem.cpp
  anki-util/source/anki/util/console/consoleSystemAPI.cpp
  anki-util/source/anki/util/console/consoleVariable.cpp
  anki-util/source/anki/util/dataPacking/dataPacking.cpp
  anki-util/source/anki/util/debug/messageDebugging.cpp
  anki-util/source/anki/util/dispatchQueue/dispatchQueue.cpp
  anki-util/source/anki/util/dispatchQueue/taskExecutor.cpp
  anki-util/source/anki/util/fileUtils/fileUtils.cpp
  anki-util/source/anki/util/global/globalDefinitions.cpp
  anki-util/source/anki/util/hashing/hashing.cpp
  anki-util/source/anki/util/helpers/base64.cpp
  anki-util/source/anki/util/helpers/callbackProvider.cpp
  anki-util/source/anki/util/helpers/cConversionHelpers.cpp
  anki-util/source/anki/util/helpers/cppToCConversionHelpers.cpp
  anki-util/source/anki/util/jsonWriter/jsonWriter.cpp
  anki-util/source/anki/util/logging/channelFilter.cpp
  anki-util/source/anki/util/logging/eventKeys.cpp
  anki-util/source/anki/util/logging/logging.cpp
  anki-util/source/anki/util/logging/printfLoggerProvider.cpp
  anki-util/source/anki/util/math/mathStructures.cpp
  anki-util/source/anki/util/md5/md5.cpp
  anki-util/source/anki/util/parsingConstants/namedConstants.cpp
  anki-util/source/anki/util/parsingConstants/parsingConstants.cpp
  anki-util/source/anki/util/ptree/includePtree.cpp
  anki-util/source/anki/util/ptree/ptreeKey.cpp
  anki-util/source/anki/util/ptree/ptreeMandatory.cpp
  anki-util/source/anki/util/ptree/ptreeToJSON.cpp
  anki-util/source/anki/util/ptree/ptreeTools.cpp
  anki-util/source/anki/util/ptree/ptreeTraverser.cpp
  anki-util/source/anki/util/random/randomGenerator.cpp
  anki-util/source/anki/util/statEngine/abstractCondition.cpp
  anki-util/source/anki/util/statEngine/combinationCondition.cpp
  anki-util/source/anki/util/statEngine/countThresholdCondition.cpp
  anki-util/source/anki/util/statEngine/dateCondition.cpp
  anki-util/source/anki/util/statEngine/multiAction.cpp
  anki-util/source/anki/util/statEngine/noticeAction.cpp
  anki-util/source/anki/util/statEngine/resetAction.cpp
  anki-util/source/anki/util/statEngine/statEngine.cpp
  anki-util/source/anki/util/statEngine/statEngineMessages.cpp
  anki-util/source/anki/util/statEngine/statNotice.cpp
  anki-util/source/anki/util/statEngine/statRule.cpp
  anki-util/source/anki/util/statEngine/statRuleBuilder.cpp
  anki-util/source/anki/util/statEngine/timeCondition.cpp
  anki-util/source/anki/util/stats/recentStatsAccumulator.cpp
  anki-util/source/anki/util/stats/runningStat.cpp
  anki-util/source/anki/util/stringTable/hashStringTable.cpp
  anki-util/source/anki/util/stringTable/stringID.cpp
  anki-util/source/anki/util/stringTable/stringTable.cpp
  anki-util/source/anki/util/time/stepTimers.cpp
  anki-util/source/anki/util/time/stopWatch.cpp
  anki-util/source/anki/util/time/universalTime.cpp
  anki-util/source/anki/util/transport/fakeUDPSocket.cpp
  anki-util/source/anki/util/transport/iNetTransportDataReceiver.cpp
  anki-util/source/anki/util/transport/netEmulatorUDPSocket.cpp
  anki-util/source/anki/util/transport/packetByteArray.cpp
  anki-util/source/anki/util/transport/pendingMultiPartMessage.cpp
  anki-util/source/anki/util/transport/pendingReliableMessage.cpp
  anki-util/source/anki/util/transport/reliableConnection.cpp
  anki-util/source/anki/util/transport/reliableMessageTypes.cpp
  anki-util/source/anki/util/transport/reliableSequenceId.cpp
  anki-util/source/anki/util/transport/reliableTransport.cpp
  anki-util/source/anki/util/transport/srcBufferSet.cpp
  anki-util/source/anki/util/transport/transportAddress.cpp
  anki-util/source/anki/util/transport/transportStats.cpp
  anki-util/source/anki/util/transport/udpTransport.cpp
  anki-util/tools/message-buffers/support/cpp/source/SafeMessageBuffer.cpp
 PROPERTIES
  COMPILE_FLAGS "-std=c11 -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g -std=c++11 -stdlib=libc++ -fdiagnostics-show-category=name -Wall -Woverloaded-virtual -Werror -Wundef -Wheader-guard -fsigned-char -fvisibility-inlines-hidden -fvisibility=default -Wshorten-64-to-32 -Winit-self -Wconditional-uninitialized -Wno-deprecated-register -Wa,--noexecstack -Wformat -Werror=format-security -g "
)
set_target_properties(util PROPERTIES LINK_FLAGS "-g ")
unset(TOOLSET)
unset(TARGET)
