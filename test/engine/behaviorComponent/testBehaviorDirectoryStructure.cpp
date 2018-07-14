/**
* File: testBehaviorDirectoryStructure
*
* Author: ross
* Created: Mar 23 2018
*
* Description: Two purposes:
*               * A test for a consistent dir structure (possibly useful when reorganizing json files)
*               * A utility to help enumerate used animation triggers.
*
* Copyright: Anki, Inc. 2018
*
**/

#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/cozmoContext.h"
#include "engine/unitTestKey.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/fileUtils/fileUtils.h"
#include "gtest/gtest.h"

#include <unordered_map>

extern Anki::Cozmo::CozmoContext* cozmoContext;

namespace Anki {
namespace Cozmo {

using BehaviorIDJsonMap = std::unordered_map<BehaviorID,  const Json::Value>;

// copied more or less from robot DataLoader
bool LoadBehaviors(const std::string& path, BehaviorIDJsonMap& behaviors)
{
  const auto* platform = cozmoContext->GetDataPlatform();
  const std::string behaviorFolder = platform->pathToResource(Anki::Util::Data::Scope::Resources, path);
  auto behaviorJsonFiles = Anki::Util::FileUtils::FilesInDirectory(behaviorFolder, true, ".json", true);
  for (const auto& filename : behaviorJsonFiles) {
    Json::Value behaviorJson;
    const bool success = platform->readAsJson(filename, behaviorJson);
    if (success && !behaviorJson.empty()) {
      BehaviorID behaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorJson, filename);
      auto result = behaviors.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(behaviorID),
                                      std::forward_as_tuple(std::move(behaviorJson)));

      if( !result.second ) { // not unique
        return false;
      }
    }
    else if (!success) {
      return false;
    }
  }
  return true;
}

TEST(BehaviorDirectoryStructure, Run)
{
  Anki::Util::_errG = false;
  const std::vector<std::string> directories = {
    "config/engine/behaviorComponent/behaviors/victorBehaviorTree",
    "config/engine/behaviorComponent/behaviors/inProgress",
    "config/engine/behaviorComponent/behaviors/devBehaviors"
  };
  // this test serves three functions, related to ensuring all classes are contained within a single
  // directory or directories and enumerating AnimationTriggers that are used. It:
  // 1) tests that the behaviors in the above directory are self-referencing, and don't reference
  //    any behaviors in a parent or sibling directory. This should be useful for manually pruning
  //    unused behavior json, and just for ensuring that people put behaviors in the correct place.
  // 2) dumps to stdout a list of classes for all json instances in this directory, including
  //    anonymous behavior classes. The python script findAnimTriggers.py can then use this list of
  //    classes to scour cpp files and find hardcoded animation triggers like AnimationTrigger::Name.
  //    NOTE: this will skip base classes!
  // 3) (optional) if you follow the instructions below, you can additionally get a list of
  //    animation triggers loaded via config string for any class in the above directory.
  //
  // ** Instructions for getting animation triggers via config. These are xcode-specific steps, but could
  // ** probably be adapted to any built process that can temporarily suspend clad generation.
  //    (1) build once so that clad is generated
  //    (2) Change this to 1:
  #         define TEST_GENERATE_ANIMATION_TRIGGERS 0
  //    (3) Start editing animationTrigger.h to unlock the file. Once unlocked, add the two lines:
  //          extern bool _gTestDumpAnimTriggers;
  //          extern std::set<std::string> _gTestAnimTriggers;
  //    (4) Unlock animationTrigger.cpp and add this somewhere:
  //          bool _gTestDumpAnimTriggers = false;
  //          std::set<std::string> _gTestAnimTriggers;
  //    (5) Still in animationTrigger.cpp, in the EnumToString() for anim triggers, add this before return true:
  //        if( _gTestDumpAnimTriggers ) {
  //          _gTestAnimTriggers.insert(str);
  //        }
  //    (6) Rerun this test. (In Xcode, this won't cause the clad to regenerate, but if you're instead
  //        using build scripts, you'll have to disable clad generation.) You should now see an additional
  //        stdout output for all animation triggers used during a behavior's construction and Init()
  //    (7) Revert the generated clad files (by deleting the generated folder, for example)
  
# if TEST_GENERATE_ANIMATION_TRIGGERS
  {
    _gTestDumpAnimTriggers = false; // don't dump anim triggers yet
  }
# endif
  
  // (Part 1) this will fail when any behavior instance in directory references a behavior that is
  // not within this path. It might fail due to dev asserts, so run in dev.
  
  BehaviorIDJsonMap behaviorData;
  for( const auto& directory : directories ) {
    const bool loaded = LoadBehaviors( directory, behaviorData );
    EXPECT_TRUE( loaded );
  }
  
  // start empty
  BehaviorIDJsonMap empty;
  BehaviorContainer* bc = new BehaviorContainer{ empty }; // deleted by the tbf
  
  TestBehaviorFramework tbf;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig( BEHAVIOR_CLASS(Wait), BEHAVIOR_ID(Wait) );
  TestBehavior emptyBase(emptyConfig);
  tbf.InitializeStandardBehaviorComponent( &emptyBase, nullptr, true, bc );
  
# if TEST_GENERATE_ANIMATION_TRIGGERS
  {
    _gTestDumpAnimTriggers = true; // now save them
  }
# endif
  
  // manually add them so we can do checks
  for( const auto& behaviorIDJsonPair : behaviorData ) {
    const auto& behaviorID = behaviorIDJsonPair.first;
    const auto& behaviorJson = behaviorIDJsonPair.second;
    EXPECT_TRUE( !behaviorJson.empty() );
    const bool createdOK = tbf.GetBehaviorContainer().CreateAndStoreBehavior(behaviorJson);
    EXPECT_TRUE( createdOK ) << BehaviorTypesWrapper::BehaviorIDToString(behaviorID) << " might reference a missing behavior";
    EXPECT_FALSE( Anki::Util::_errG );
  }
  
  // make sure they can all be init'd (this is where anonymous behaviors get loaded)
  tbf.GetBehaviorContainer().Init( tbf.GetBehaviorExternalInterface() );
  
  EXPECT_FALSE( Anki::Util::_errG );
  
  // (Part 2) now get a list of behavior classes associated with this directory. most behaviors can be
  // obtained from the BehaviorContainer, but we need to walk the anonymous behavior map pointers
  // to get those class names.
  
  using Type = std::underlying_type<BehaviorID>::type;
  std::queue<ICozmoBehaviorPtr> queue;
  std::set<ICozmoBehaviorPtr> visited;
  std::set<std::string> classNames;
  for( Type i = 0; i<BehaviorTypesWrapper::GetBehaviorIDNumEntries(); ++i ) {
    BehaviorID id = static_cast<BehaviorID>(i);
    ICozmoBehaviorPtr behavior = tbf.GetBehaviorContainer().FindBehaviorByID( id );
    if( behavior == nullptr ) {
      continue;
    }
    queue.push( behavior );
    
    const auto className = behavior->GetClass();
    classNames.insert( BehaviorTypesWrapper::BehaviorClassToString( className ) );
    
    visited.insert( behavior );
  }

  while( !queue.empty() ) {
    ICozmoBehaviorPtr behavior = queue.front();
    queue.pop();

    const auto className = behavior->GetClass();
    classNames.insert( BehaviorTypesWrapper::BehaviorClassToString( className ) );

    std::map<std::string,ICozmoBehaviorPtr> anonBehaviors = behavior->TESTONLY_GetAnonBehaviors({});
    for( const auto& elem : anonBehaviors ) {
      const auto& anon = elem.second;
      if( visited.count( anon ) == 0 ) {
        queue.push( anon );

        visited.insert( anon );
        const auto className = anon->GetClass();
        classNames.insert( BehaviorTypesWrapper::BehaviorClassToString( className ) );
      }
    }
  }

  std::string ss;
  for( const auto& elem : classNames ) {
    ss += elem;
    ss += "\n";
  }
  std::cout << "BEHAVIOR CLASSES USED:" << std::endl << ss;
  
  // (Part 3) dump animation trigger loaded by config
# if TEST_GENERATE_ANIMATION_TRIGGERS
  {
    if( !_gTestAnimTriggers.empty() ) {
      ss.clear();
      for( const auto& trigger : _gTestAnimTriggers ) {
        ss += trigger;
        ss += "\n";
      }
      std::cout << "ANIMATIONTRIGGERS USED:" << std::endl << ss;
    }
  }
# endif

}

} // end namespace
} // end namespace
