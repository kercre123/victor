/**
* File: testBehaviorDleegationTree
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Set of test functions that walk through cozmo's data defined
* behavior tree that ensure all possible behavior states are valid and
* within design's constraints
*
* Copyright: Anki, Inc. 2017
*
* --gtest_filter=DelegationTree*
**/

#define private public
#define protected public

#include "gtest/gtest.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/boundedWhile.h"



using namespace Anki::Vector;

TEST(DelegationTree, FullTreeWalkthrough)
{
  // Read in the current behavior system configuration
  // and then walk through the full tree appropirately activating
  // and deactivating all delegates to ensure the tree is valid
  TestBehaviorFramework testFramework(1, nullptr);
  testFramework.InitializeStandardBehaviorComponent();
  testFramework.SetDefaultBaseBehavior();

  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  IBehavior* bottomOfStack = bsm._behaviorStack->_behaviorStack.front();
  auto& delegationComponent = testFramework.GetBehaviorExternalInterface().GetDelegationComponent();
  delegationComponent.CancelDelegates(bottomOfStack);
  
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  bottomOfStack->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(bottomOfStack, tmpDelegates));
  testFramework.FullTreeWalk(delegateMap);
}



TEST(DelegationTree, DesignedControlTest)
{
  // TODO: Read in the current behavior system configuration
  // then walk through the tree and ensure that all design requirements
  // are met. E.G. every behavior must be able to transition to V.C. when
  // necessary.
  
}

TEST(DelegationTree, DumpBehaviorTransitionsToFile)
{
  // the accompanying python script will be looking for this file
  std::string outFilename;
  char* szFilename = getenv("ANKI_TEST_BEHAVIOR_FILE");
  if( szFilename != nullptr ) {
    outFilename = szFilename;
  } else {
    return;
  }
  
  TestBehaviorFramework testFramework(1, nullptr);
  testFramework.InitializeStandardBehaviorComponent();
  
  const auto* dataLoader = testFramework.GetRobot().GetContext()->GetDataLoader();
  ASSERT_NE( dataLoader, nullptr ) << "Cannot test behaviors if no data loader exists";
  
  const auto& bc = testFramework.GetBehaviorContainer();
  const auto& behaviorMap = bc.GetBehaviorMap();
  
  std::stringstream ss;
  
  for( const auto& behPair : behaviorMap ) {
    
    std::string id = Anki::Vector::BehaviorTypesWrapper::BehaviorIDToString( behPair.first );
    const ICozmoBehaviorPtr behavior = behPair.second;
    
    std::set<IBehavior*> delegates;
    behavior->GetAllDelegates( delegates );
    for( const auto* delegate : delegates ) {
      
      std::string outId = delegate->GetDebugLabel();
      // skip trailing digits that were added to make labels unique
      for( auto it = outId.rbegin(); it != outId.rend(); ) {
        if( std::isdigit(*it) ) {
          ++it;
          outId.pop_back();
        } else {
          break;
        }
      }
      ss << id << " " << outId << std::endl;
      
    }
    
  }
  
  auto res = Anki::Util::FileUtils::WriteFile( outFilename, ss.str() );
  EXPECT_EQ(res, true) << "Error writing file " << outFilename;
}

TEST(DelegationTree, DumpBehaviorTreeBranchesToFile)	
{	
  // Creates a file that lists all possible behavior stacks
  std::string outFilename;	
  char* szFilename = getenv("ANKI_TEST_BEHAVIOR_BRANCHES");	
  if( szFilename != nullptr ) {	
    outFilename = szFilename;	
  } else {	
    return;	
  }

  std::stringstream ss;	

  // Get the base behavior for default stack
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  tbf.SetDefaultBaseBehavior();
  auto currentStack = tbf.GetCurrentBehaviorStack();  
  DEV_ASSERT(1 == currentStack.size(), "CanStackOccurDuringFreeplay.SizeMismatch");
  IBehavior* base = currentStack.front();

  // Get ready for a full tree walk to compare stacks 
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  base->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(base, tmpDelegates));

  // tree walk callback
  auto evaluateTree = [&ss, &tbf](){
    auto currentStack = tbf.GetCurrentBehaviorStack();
    ss << BehaviorStack::StackToBehaviorString(currentStack);
    ss << ",\n";
  };

  tbf.FullTreeWalk(delegateMap, evaluateTree);
  	
  auto res = Anki::Util::FileUtils::WriteFile( outFilename, ss.str() );	
  EXPECT_EQ(res, true) << "Error writing file " << outFilename;	
}

namespace {
  struct DelegationTreeNode {
    IBehavior* behavior;
    std::vector< std::shared_ptr<DelegationTreeNode> > children;
    DelegationTreeNode* parent;

    std::string indentString;
    std::string entryString;
    std::string continueString;

    DelegationTreeNode(IBehavior* behavior,
        std::vector< std::shared_ptr<DelegationTreeNode> > children,
        DelegationTreeNode* parent = nullptr)
    : indentString("  "),
      entryString("* "),
      continueString("| ")
    {
      this->behavior = behavior;
      this->children = children;
      this->parent = parent;
    }

    void FormatAsList(std::stringstream &ss, std::string prefix = "", bool isLastOfDepth = true)
    {
      // depth-first through the tree, maintaining a tab depth, output behavior names
      // note that this will be called recursively!

      std::string outID = behavior->GetDebugLabel(); // TODO: can trim unique-making digits as above if desired
      // let's see if we can output the behavior's class name.
      // IBehavior doesn't let us do that, but ICozmoBehavior does...
      // this is dangerous, of course, but in practice we can be reasonably sure we have ICozmoBehaviors
      ICozmoBehavior* cozmoBehavior = static_cast<ICozmoBehavior*>(behavior);
      BehaviorClass behaviorClass = cozmoBehavior->GetClass();
      std::string behaviorClassStr = BehaviorTypesWrapper::BehaviorClassToString(behaviorClass);
      ss << prefix << entryString << outID << " : " << behaviorClassStr << "\n";
      // compute ongoing prefix
      // if no children, can stop now
      if (children.size() < 1) {
        return;
      }
      std::string newPrefix;
      // if is last of depth, we don't use continuation
      // TODO: is there a clever way to do a "min rows for connector" like Brad does is the Python version?
      // (the brute force version is to do a pass through the tree to determine the size of each node)
      // number of children as cheap proxy for size
      if (isLastOfDepth || (children.size() < 4) ) {
        newPrefix = prefix + indentString;
      } else {
        newPrefix = prefix + continueString;
      }
      for (int i = 0; i < children.size()-1; i++) {
        children[i]->FormatAsList(ss, newPrefix, false);
      }
      // for the last one we tell it it's last of depth, so it doesn't do a continuation marker
      children[children.size() - 1]->FormatAsList(ss, newPrefix, true);
    }

    void FormatAsHTML(std::stringstream &ss)
    {
      // depth-first through the tree, called recursively
      std::string outID = behavior->GetDebugLabel(); // TODO: can trim unique-making digits as above if desired
      // let's see if we can output the behavior's class name.
      // IBehavior doesn't let us do that, but ICozmoBehavior does...
      // this is dangerous, of course, but in practice we can be reasonably sure we have ICozmoBehaviors
      ICozmoBehavior* cozmoBehavior = static_cast<ICozmoBehavior*>(behavior);
      BehaviorClass behaviorClass = cozmoBehavior->GetClass();
      std::string behaviorClassStr = BehaviorTypesWrapper::BehaviorClassToString(behaviorClass);
      std::string line = outID + " : " + behaviorClassStr;
      if (children.size() < 1) {
        // just a line if this is a leaf
        ss << line << "\n";
      } else {
        // otherwise a details structure, with line as summary and list of children in details
        ss << "<details>\n";
        ss << "<summary>" << line << "</summary>\n";
        ss << "<ul>\n";
        // recurse
        for (const auto c : children) {
          ss << "<li>\n";
          c->FormatAsHTML(ss);
          ss << "</li>\n";
        }
        ss << "</ul>\n";
        ss << "</details>\n";
      }
    }
  };
}

std::shared_ptr<DelegationTreeNode> DelegationTreeFromMap(
    std::map<IBehavior*,std::set<IBehavior*>> delegateMap,
    IBehavior* base,
    DelegationTreeNode* parent = nullptr)
{
  std::shared_ptr<DelegationTreeNode> root(new DelegationTreeNode(base,
                                            std::vector< std::shared_ptr<DelegationTreeNode> >(),
                                            parent) );

  // figure out if this is a dispatcher that needs its delegates in the correct order
  // behaviorContainter's FindBehaviorByIDAndDownCast only works for the ultimate class, I believe--can't check in the
  // middle of the class hierarchy. We'll try dynamic_cast
  IBehaviorDispatcher* base_dispatcher = dynamic_cast<IBehaviorDispatcher*>(base);
  // check if that worked
  if (base_dispatcher != nullptr) {
    // this is a dispatcher

    // NOTE this method is protected; the "#define protected public" hack at the top of this file is necessary for this to work
    std::vector<ICozmoBehaviorPtr> delegates_vcp = base_dispatcher->GetAllPossibleDispatches();

    for (ICozmoBehaviorPtr b : delegates_vcp) {
      root->children.push_back( DelegationTreeFromMap(delegateMap, b.get(), root.get()) );
    }
  } else {
    // this is not a dispatcher
    std::set< IBehavior* > delegates_sbs;
    base->GetAllDelegates(delegates_sbs);
    for (IBehavior* b : delegates_sbs){
      root->children.push_back( DelegationTreeFromMap(delegateMap, b, root.get()) );
    }
  }


  return root;
}

TEST(DelegationTree, CreateBehaviorTreeHTML)
{
  // This test outputs an HTML representation of the Behavior Tree, with type annotations and correct
  // ordering for priority dispatchers and the like.

  std::string outFilename;
  char* szFilename = getenv("ANKI_TEST_DELEGATION_TREE_HTML");
  if( szFilename != nullptr ) {
    outFilename = szFilename;
  } else {
    return;
  }

  // Get the base behavior for default stack
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  tbf.SetDefaultBaseBehavior();
  auto currentStack = tbf.GetCurrentBehaviorStack();
  DEV_ASSERT(1 == currentStack.size(), "CanStackOccurDuringFreeplay.SizeMismatch");
  IBehavior* base = currentStack.front();

  // Get ready for a full tree walk to compare stacks
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  base->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(base, tmpDelegates));

  tbf.FullTreeWalk(delegateMap);

  // after the full tree walk, delegate map will be populated. just process that.
  std::shared_ptr<DelegationTreeNode> delegationTreePtr = DelegationTreeFromMap(delegateMap, base, nullptr);

  // output
  std::stringstream ss;
  // front matter for HTML
  ss << "<!DOCTYPE html>\n<html>\n<body>\n<ul>\n<li>\n";
  delegationTreePtr->FormatAsHTML(ss);
  // back matter for HTML
  ss << "</li>\n</ul>\n</body>\n</html>\n";

  auto res = Anki::Util::FileUtils::WriteFile( outFilename, ss.str() );
  EXPECT_EQ(res, true) << "Error writing file " << outFilename;
}


TEST(DelegationTree, CheckActiveFeatures)
{
  // this test checks that active features are correctly defined, and also dumps the active feature per
  // behavior branch to a file, if specified in the environment


  ////////////////////////////////////////////////////////////////////////////////

  // Add any active feature definitions here which exist but aren't yet used in the main behavior tree
  // (e.g. because they are still under development). This must be removed once they are used
  std::set< ActiveFeature > unusedActiveFeatures = {
    ActiveFeature::Frustrated, // not used
    ActiveFeature::Onboarding, // exists but in a different stack. todo: tests for onboarding.
    ActiveFeature::RequestCharger, // exists but in a different stack.
  };

  ////////////////////////////////////////////////////////////////////////////////
  
  // Creates a file that lists all possible behavior stacks
  std::string outFilename;	
  char* szFilename = getenv("ANKI_TEST_BEHAVIOR_FEATURES");
  if( szFilename != nullptr ) {	
    outFilename = szFilename;	
  } else {	
    return;	
  }

  std::stringstream ss;	

  // Get the base behavior for default stack
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  tbf.SetDefaultBaseBehavior();
  auto currentStack = tbf.GetCurrentBehaviorStack();  
  DEV_ASSERT(1 == currentStack.size(), "CanStackOccurDuringFreeplay.SizeMismatch");
  IBehavior* base = currentStack.front();

  // Get ready for a full tree walk to compare stacks
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  base->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(base, tmpDelegates));

  // verify that all active features are used
  ASSERT_EQ( (int) ActiveFeature::NoFeature, 0 ) << "unit test is broken without this";

  std::set<ActiveFeature> usedFeatures;

  // don't require explicitly using NoFeature
  usedFeatures.insert(ActiveFeature::NoFeature);
  
  auto& bei = tbf.GetBehaviorExternalInterface();
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  
  // tree walk callback
  auto evaluateTree = [&ss, &tbf, &usedFeatures, &uic](bool isLeaf){
    auto currentStack = tbf.GetCurrentBehaviorStack();
    ss << BehaviorStack::StackToBehaviorString(currentStack);
    ss << ", ";
    
    if( uic.IsAnyUserIntentPending() ) {
      uic.DropAnyUserIntent();
    }
    
    auto& afc = tbf.GetBehaviorComponent().GetComponent<ActiveFeatureComponent>();

    // fake an update of the afc (since ticks aren't running)
    afc.UpdateDependent(*tbf.GetBehaviorComponent()._comps);
    
    const ActiveFeature afcFeature = afc.GetActiveFeature();
    usedFeatures.insert(afcFeature);

    ss << ActiveFeatureToString(afcFeature);
    ss << ",\n";


    // all leaf behaviors must have some feature specified, or explicitly specify NoFeature. Additionally, if
    // a behavior explicitly specifies NoFeature, then it shouldn't delegate to any behavior with a feature
    // lower in the stack.
    // Allowed: nothing / FeatureA / FeatureB  / nothing / NoFeature
    // Allowed: nothing / FeatureA / NoFeature / nothing / NoFeature
    // Illegal: nothing / FeatureA / NoFeature / nothing / FeatureB

    if( isLeaf ) {

      const auto& behaviorIterator = tbf.GetBehaviorComponent().GetComponent<ActiveBehaviorIterator>();
      bool hasExplicitNoFeature = false;
      bool hasAnyFeature = false;
      
      auto checkFeatureCallback = [&hasExplicitNoFeature, &hasAnyFeature](const ICozmoBehavior& behavior) {
        ActiveFeature feature = ActiveFeature::NoFeature;
        if( behavior.GetAssociatedActiveFeature(feature) ) {
          if( feature == ActiveFeature::NoFeature ) {
            hasExplicitNoFeature = true;
          }
          else {
            // got a feature, so we shouldn't already have "no feature"
            EXPECT_FALSE(hasExplicitNoFeature) << "Behavior stack specified no feature, but at behavior "
                                               << behavior.GetDebugLabel() << " has active feature " << ActiveFeatureToString(feature);
            hasAnyFeature = true;
          }
        }
        return true; // Iterate the whole stack
      };

      behaviorIterator.IterateActiveCozmoBehaviorsForward(checkFeatureCallback);

      EXPECT_TRUE(hasExplicitNoFeature || hasAnyFeature)
        << "must specify some feature in each stack, or manually specify NoFeature" << std::endl
        << "behavior stack: " << BehaviorStack::StackToBehaviorString(currentStack) << std::endl;

      if( hasExplicitNoFeature && !hasAnyFeature ) {
        EXPECT_EQ(afcFeature, ActiveFeature::NoFeature)
          << "stack specifies no feature, but component has feature " << ActiveFeatureToString(afcFeature) << std::endl
          << "behavior stack: " << BehaviorStack::StackToBehaviorString(currentStack) << std::endl;
      }
      else {
        EXPECT_NE(afcFeature, ActiveFeature::NoFeature)
          << "stack specifies a feature, but component has feature " << ActiveFeatureToString(afcFeature) << std::endl
          << "behavior stack: " << BehaviorStack::StackToBehaviorString(currentStack) << std::endl;
      }
    }
  };

  tbf.FullTreeWalk(delegateMap, evaluateTree);

  auto res = Anki::Util::FileUtils::WriteFile( outFilename, ss.str() );	
  EXPECT_EQ(res, true) << "Error writing file " << outFilename;

  // verify that each active feature was used
  for( uint32_t featureInt = 1; featureInt < ActiveFeatureNumEntries; ++featureInt ) {
    const ActiveFeature af = (ActiveFeature)featureInt;

    if( unusedActiveFeatures.find(af) == unusedActiveFeatures.end() ) {
      EXPECT_TRUE( usedFeatures.find(af) != usedFeatures.end() ) << "Tree did not expose feature "
                                                                 << ActiveFeatureToString(af);
    }
  }

  // make sure a lazy developer didn't forget to remove an unused feature once it's actually used
  for( const auto& af : unusedActiveFeatures ) {
    EXPECT_TRUE( usedFeatures.find(af) == usedFeatures.end() )
      << "Please remove '" << ActiveFeatureToString(af) << "' from the unused features list (since it's used)";
  }
}

TEST(DelegationTree, PrepareToBeForceActivated)
{
  // doesn't actually test anything. makes a list of those behaviors whose PrepareTobeForceActivated
  // arent sufficient, and they still don't want to be activated
  using ListType = std::map<std::string, std::set<std::string>>;
  ListType failingBehaviors;
  ListType workingBehaviors;
  
  TestBehaviorFramework testFramework(1, nullptr);
  testFramework.InitializeStandardBehaviorComponent();
  testFramework.SetDefaultBaseBehavior();
  
  auto addToList = [](IBehavior* delegate, ListType& list) {
    const auto* castPtr = dynamic_cast<const ICozmoBehavior*>(delegate);
    ASSERT_TRUE( castPtr != nullptr );
    auto& classes = list[ BehaviorTypesWrapper::BehaviorClassToString(castPtr->GetClass()) ];
    classes.insert( BehaviorIDToString( castPtr->GetID() ) );
  };
  
  auto evalPriorToDelegation = [&](IBehavior* delegate) {
    if( !delegate->WantsToBeActivated() ) {
      addToList( delegate, failingBehaviors );
    } else {
      addToList( delegate, workingBehaviors );
    }
  };
  
  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  IBehavior* bottomOfStack = bsm._behaviorStack->_behaviorStack.front();
  auto& delegationComponent = testFramework.GetBehaviorExternalInterface().GetDelegationComponent();
  delegationComponent.CancelDelegates(bottomOfStack);
  
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  bottomOfStack->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(bottomOfStack, tmpDelegates));
  auto normalCallback = [](){};
  testFramework.FullTreeWalk(delegateMap, normalCallback, evalPriorToDelegation);
  
  auto printList = [](const ListType& list){
    for( const auto& behClassList : list ) {
      std::cout << behClassList.first << ": ";
      for( auto& beh : behClassList.second ) {
        std::cout << beh << ", ";
      }
      std::cout << std::endl;
    }
  };
  
  std::cout << "The following behavior classes (and instances) can be force started:" << std::endl;
  printList(workingBehaviors);
  std::cout << std::endl;
  
  std::cout << "The following behavior classes (and instances) can NOT be force started:" << std::endl;
  printList(failingBehaviors);
  std::cout << std::endl;
  
}
