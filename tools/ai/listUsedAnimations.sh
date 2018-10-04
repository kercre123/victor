#!/bin/bash
#
# Lists used animation names/groups/triggers, and, if the previous
# build's artifact is available, reports changes in animation usage
#

set -e
set -x

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

ARCHIVE_FILE="animation_lists.tar.gz"
ANIM_LIST_FILE="anim_list.txt"
ANIM_TRIGGER_FILE="anim_trigger_list.txt"
ANIM_GROUP_FILE="anim_group_list.txt"
UNUSED_TRIGGERS_FILE="unused_triggers.txt"
UNUSED_GROUPS_FILE="unused_groups.txt"
OLD_DIR="previous_build"
RESULTS_FILE="report.txt"

#######################################################################################################
# 1) Cleanup files as needed

# clean up any existing files, except the archive file, since that is a dependency from the last build.
# if they don't exist (common case), continue without error
rm -rf $ANIM_LIST_FILE $ANIM_TRIGGER_FILE $ANIM_GROUP_FILE $UNUSED_TRIGGERS_FILE $UNUSED_GROUPS_FILE $OLD_DIR $RESULTS_FILE || true

mkdir $OLD_DIR

# if the archive file exists, extract to old dir
if [ -f $ARCHIVE_FILE ]; then
    tar xzf $ARCHIVE_FILE -C $OLD_DIR
    # now remove any old archive file
    rm -rf $ARCHIVE_FILE
fi

#######################################################################################################
# 2) gather anything of the form "anim_*" (including quotes) in any .cpp, .h, or config .json file.
#    animationStreamer.cpp has some temporary animation blacklists that need exluding, but it also has neutral
#    eyes, so exclude the file but manually add the neutral eyes animation back in

ANIM_LIST=`find cannedAnims engine animProcess resources/config \
            -not -path "resources/config/engine/animations/*" \
            -iname "*.json" -o -iname "*.h" -o -iname "*.cpp" \
          | grep -v "animationStreamer.cpp\|animation_whitelist.json" \
          | xargs grep -Eoh \"anim_\.+\" \
          | (cat && echo "anim_neutral_eyes_01") \
          | cut -d "\"" -f 2 \
          | sort | uniq \
          | tee $ANIM_LIST_FILE`  # <--- also write to file

#######################################################################################################
# 3) gather anything of the form "ag_*" (including quotes) in any .cpp, .h, or config .json file, and
#    fail the script if so, since we don't handle that case yet

ANIM_GROUP_LIST=`find cannedAnims engine animProcess resources/config \
                  -iname "*.json" -o -iname "*.h" -o -iname "*.cpp" \
                | xargs grep -Eoh \"ag_\.+\" \
                | cut -d "\"" -f 2 \
                | sort | uniq`
if [[ $ANIM_GROUP_LIST = *[![:space:]]* ]]; then
    echo "This script doesn't know how to handle victor usage of raw animation groups ('ag_*'). Failing"
    exit 1
fi

#######################################################################################################
# 4) gather any usage of AnimationTrigger::TriggerName in engine C++ code

TRIGGER_LIST_CPP=`${GIT_PROJ_ROOT}/tools/ai/findAnimTriggers.py ${GIT_PROJ_ROOT}/engine`

#######################################################################################################
# 5) Gather any usage of animation triggers referenced as strings by
#    running a special build of a unit test

# find and verify files that need modifying
TEST_FILE="${GIT_PROJ_ROOT}/test/engine/behaviorComponent/testBehaviorDirectoryStructure.cpp"
TRIGGER_FILE_H="${GIT_PROJ_ROOT}/generated/clad/engine/clad/types/animationTrigger.h"
TRIGGER_FILE_CPP="${GIT_PROJ_ROOT}/generated/clad/engine/clad/types/animationTrigger.cpp"
if [ ! -f $TEST_FILE ]; then
    echo "File '${TEST_FILE}' not found! Failing"
    exit 1
fi
if [ ! -f $TRIGGER_FILE_H ]; then
    echo "File '${TRIGGER_FILE_H}' not found! Failing"
    exit 1
fi
if [ ! -f $TRIGGER_FILE_CPP ]; then
    echo "File '${TRIGGER_FILE_CPP}' not found! Failing"
    exit 1
fi

# enable flag in unit test 
sed -i '' -E 's/define +TEST_GENERATE_ANIMATION_TRIGGERS +0/define TEST_GENERATE_ANIMATION_TRIGGERS 1/g' $TEST_FILE

# find the line "#include <vector>" and insert additional header
TRIGGER_H_INSERTION_PT=`grep -noE "\#include <vector>" $TRIGGER_FILE_H  | sed -e 's/:.*//g'`
sed -i '' "${TRIGGER_H_INSERTION_PT} a\\
  #include <set>\\
" ${TRIGGER_FILE_H}

# find header line "namespace Vector {" and insert declarations after
TRIGGER_H_INSERTION_PT=`grep -noE "namespace Vector \{" $TRIGGER_FILE_H  | sed -e 's/:.*//g'`
sed -i '' "${TRIGGER_H_INSERTION_PT} a\\
  extern bool _gTestDumpAnimTriggers;\\
  extern std::set<std::string> _gTestAnimTriggers;\\
" ${TRIGGER_FILE_H}

# find cpp line "namespace Vector {" and insert definitions after
TRIGGER_CPP_INSERTION_PT=`grep -noE "namespace Vector \{" $TRIGGER_FILE_CPP  | sed -e 's/:.*//g'`
sed -i '' "${TRIGGER_CPP_INSERTION_PT} a\\
  bool _gTestDumpAnimTriggers = false;\\
  std::set<std::string> _gTestAnimTriggers;\\
" ${TRIGGER_FILE_CPP}

# find cpp line "EnumFromString(const std::string& str, AnimationTrigger& enumOutput)", and then find the next match
# of "enumOutput = it->second;", and insert code there that caches used triggers
FUNC_START=`grep -noE "EnumFromString\(const std\:\:string\& str, AnimationTrigger\& enumOutput\)" $TRIGGER_FILE_CPP  | sed -e 's/:.*//g'`
FUNC_INSERTION_PTS=`grep -noE "enumOutput = it->second;" $TRIGGER_FILE_CPP  | sed -e 's/:.*//g'`
TRIGGER_CPP_INSERTION_PT=""
for FUNC_INSERTION_PT in $FUNC_INSERTION_PTS; do
    if [[ $FUNC_INSERTION_PT -gt $FUNC_START ]]; then
        TRIGGER_CPP_INSERTION_PT=$FUNC_INSERTION_PT
        break;
    fi
done
if [[ -z $TRIGGER_CPP_INSERTION_PT ]]; then
    echo "Could not find EnumFromString insertion point in file '${TRIGGER_FILE_CPP}'. Failing"
    exit 1
fi
sed -i '' "${TRIGGER_CPP_INSERTION_PT} a\\
  if( _gTestDumpAnimTriggers ) { \\
    _gTestAnimTriggers.insert(str); \\
  } \\
" ${TRIGGER_FILE_CPP}


# re-build with -I to adopt the above changes
${GIT_PROJ_ROOT}/project/victor/build-victor.sh -c Debug -p mac -I

# run a single unit test and capture lines between (and including) annotations
export GTEST_FILTER="BehaviorDirectoryStructure.Run"
UNIT_TEST_OUT=`${GIT_PROJ_ROOT}/project/buildServer/steps/unittestsEngine.sh -v \
              | sed -n '/<BEGIN_ANIMATION_TRIGGERS_USED>/,/<END_ANIMATION_TRIGGERS_USED>/p' \
              | sed -e 's/.*: //g'`
unset GTEST_FILTER

if [[ -z $UNIT_TEST_OUT ]]; then
    echo "Unit test did not output a list of animation triggers used"
    exit 1
fi

# remove annotations to give a list of animation trigger strings
TRIGGER_STRINGS=`echo "$UNIT_TEST_OUT" | sed '1d;$d'`

#######################################################################################################
# 6) Combine steps (4) and (5) to make a single list of used animation triggers, and
#    then reference the trigger map to make a list of used animation groups

# make a single list of used anim triggers
ANIM_TRIGGERS=`(echo "$TRIGGER_STRINGS" \
                 && echo "$TRIGGER_LIST_CPP") \
              | sort | uniq\
              | tee $ANIM_TRIGGER_FILE` # <--- also write to file

# get anim groups associated with the used anim triggers
TRIGGER_MAP_FILE="${GIT_PROJ_ROOT}/resources/assets/cladToFileMaps/AnimationTriggerMap.json"
if [ ! -f $TRIGGER_MAP_FILE ]; then
    echo "File '${TRIGGER_MAP_FILE}' not found! Failing"
    exit 1
fi
ANIM_GROUPS=`for ANIM_TRIGGER in $ANIM_TRIGGERS; do
                grep -A 1 "\"$ANIM_TRIGGER\"" $TRIGGER_MAP_FILE | cut -d\" -f 4 | tail -n1
            done \
            | sort | uniq | tee $ANIM_GROUP_FILE` # <--- also write to file

#######################################################################################################
# 7) find what animation triggers and groups are unused in the project

EXISTING_TRIGGERS=`grep Clad $TRIGGER_MAP_FILE | tr "\"" " " | awk '{print $3}' | sort | uniq`
UNUSED_TRIGGERS=`for EXISTING_TRIGGER in $EXISTING_TRIGGERS; do
                    if [ "$(echo "$ANIM_TRIGGERS" | grep -c "^$EXISTING_TRIGGER$")" -eq 0 ]; then
                        echo $EXISTING_TRIGGER
                    fi
                done`
echo "$UNUSED_TRIGGERS" | tee $UNUSED_TRIGGERS_FILE 

EXISTING_GROUPS=`grep AnimName $TRIGGER_MAP_FILE | tr "\"" " " | awk '{print $3}' | sort | uniq`
UNUSED_GROUPS=`for EXISTING_GROUP in $EXISTING_GROUPS; do
                  if [ "$(echo "$ANIM_GROUPS" | grep -c "^$EXISTING_GROUP$")" -eq 0 ]; then
                      echo $EXISTING_GROUP
                  fi
              done`
echo "$UNUSED_GROUPS" | tee $UNUSED_GROUPS_FILE

#######################################################################################################
# 8) print results and a diff to RESULTS_FILE

{ 
    echo "NIGHTLY ANIMATION USAGE BUILD REPORT";
    echo `date "+%F %T"`;
    echo "";
    echo "Found `echo $ANIM_LIST        | wc -w` animation names";
    echo "Found `echo $TRIGGER_STRINGS  | wc -w` animation triggers referenced by name";
    echo "Found `echo $TRIGGER_LIST_CPP | wc -w` animation triggers referenced by enum value";
    echo "Found `echo $ANIM_GROUPS      | wc -w` animation groups referenced by animation trigger";
    echo "Found `echo $UNUSED_TRIGGERS  | wc -w` unused animation triggers in the trigger map";
    echo "Found `echo $UNUSED_GROUPS    | wc -w` unused animation groups in the trigger map";
    echo "";
    echo "****CHANGES****";
    echo "";
} | tee $RESULTS_FILE

# if previous versions of some of these files exist, do a diff to find modifications
ANYTHING_CHANGED=false

# animation names list
if [ -f $OLD_DIR/$ANIM_LIST_FILE ]; then
    ANIM_NAMES_REMOVED=`comm -13i $ANIM_LIST_FILE $OLD_DIR/$ANIM_LIST_FILE`
    if [[ ! -z $ANIM_NAMES_REMOVED ]]; then
        echo "Animations called by name REMOVED:"           | tee -a $RESULTS_FILE
        echo "$ANIM_NAMES_REMOVED" | sed 's/^/  /'          | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
    ANIM_NAMES_ADDED=`comm -13i $OLD_DIR/$ANIM_LIST_FILE $ANIM_LIST_FILE`
    if [[ ! -z $ANIM_NAMES_ADDED ]]; then
        echo "Animations called by name ADDED:"             | tee -a $RESULTS_FILE
        echo "$ANIM_NAMES_ADDED" | sed 's/^/  /'            | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
else
    echo "WARNING: Could not compare to the previous build's animation names"       | tee -a $RESULTS_FILE
fi

# animation triggers list
if [ -f $OLD_DIR/$ANIM_TRIGGER_FILE ]; then
    ANIM_TRIGGERS_REMOVED=`comm -13i $ANIM_TRIGGER_FILE $OLD_DIR/$ANIM_TRIGGER_FILE`
    if [[ ! -z $ANIM_TRIGGERS_REMOVED ]]; then
        echo "Animation triggers REMOVED:"                  | tee -a $RESULTS_FILE
        echo "$ANIM_TRIGGERS_REMOVED" | sed 's/^/  /'       | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
    ANIM_TRIGGERS_ADDED=`comm -13i $OLD_DIR/$ANIM_TRIGGER_FILE $ANIM_TRIGGER_FILE`
    if [[ ! -z $ANIM_TRIGGERS_ADDED ]]; then
        echo "Animation triggers ADDED:"                    | tee -a $RESULTS_FILE
        echo "$ANIM_TRIGGERS_ADDED" | sed 's/^/  /'         | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
else
    echo "WARNING: Could not compare to the previous build's animation triggers"    | tee -a $RESULTS_FILE
fi

# animation groups list
if [ -f $OLD_DIR/$ANIM_GROUP_FILE ]; then
    ANIM_GROUPS_REMOVED=`comm -13i $ANIM_GROUP_FILE $OLD_DIR/$ANIM_GROUP_FILE`
    if [[ ! -z $ANIM_GROUPS_REMOVED ]]; then
        echo "Animation groups REMOVED:"                    | tee -a $RESULTS_FILE
        echo "$ANIM_GROUPS_REMOVED" | sed 's/^/  /'         | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
    ANIM_GROUPS_ADDED=`comm -13i $OLD_DIR/$ANIM_GROUP_FILE $ANIM_GROUP_FILE`
    if [[ ! -z $ANIM_GROUPS_ADDED ]]; then
        echo "Animation groups ADDED:"                      | tee -a $RESULTS_FILE
        echo "$ANIM_GROUPS_ADDED" | sed 's/^/  /'           | tee -a $RESULTS_FILE
        ANYTHING_CHANGED=true
    fi
else
    echo "WARNING: Could not compare to the previous build's animation groups"      | tee -a $RESULTS_FILE
fi

if [ "$ANYTHING_CHANGED" = false ]; then
    echo "[ no changes detected ]"  | tee -a $RESULTS_FILE
fi

#######################################################################################################
# 9) create artifact with details for download, and so that the next build can compare to us

tar czf $ARCHIVE_FILE  \
    $ANIM_LIST_FILE $ANIM_TRIGGER_FILE $ANIM_GROUP_FILE \
    $UNUSED_TRIGGERS_FILE $UNUSED_GROUPS_FILE $RESULTS_FILE

#######################################################################################################
# 10) cleanup tracked files to avoid build server issues

git checkout ${GIT_PROJ_ROOT}
rm -rf ${GIT_PROJ_ROOT}/generated
