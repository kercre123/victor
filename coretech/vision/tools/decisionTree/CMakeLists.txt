# coretech/common/robot/tools/CMakeLists.txt

# save buffered video

file(GLOB SOURCE_FILES_RUN_TRAIN_DECISION_TREE
  run_trainDecisionTree.cpp
)

file(GLOB SOURCE_FILES_LIB_TRAIN_DECISION_TREE
  trainDecisionTree.cpp
  computeInfoGain.cpp
)

file(GLOB HEADER_FILES
  ../../../common/tools/threads/anki/tools/threads/threadSafeCounter.h
  ../../../common/tools/threads/anki/tools/threads/threadSafeFixedLengthList.h
  ../../../common/tools/threads/anki/tools/threads/threadSafeQueue.h
  ../../../common/tools/threads/anki/tools/threads/threadSafeUtilities.h
  ../../../common/tools/threads/anki/tools/threads/threadSafeVector.h
  trainDecisionTree.h
)

#
# run_trainDecisionTree
#

add_executable(run_trainDecisionTree
  ${SOURCE_FILES_RUN_TRAIN_DECISION_TREE}
  ${HEADER_FILES})

target_link_libraries(run_trainDecisionTree
  CoreTech_Common_Robot
  CoreTech_Vision_Robot
  libTrainDecisionTree
  ${ALL_EMBEDDED_LIBRARIES})

set_target_properties(run_trainDecisionTree PROPERTIES
  FOLDER "Tools"
)

#
# lib_trainDecisionTree
#

add_definitions(-DCORETECH_ROBOT)

add_library(libTrainDecisionTree
  ${SOURCE_FILES_LIB_TRAIN_DECISION_TREE}
  ${HEADER_FILES}
)

set_target_properties(libTrainDecisionTree PROPERTIES
  FOLDER "Tools"
)
