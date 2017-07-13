/**
 * File: ptreeTools
 *
 * Author: damjan
 * Created: 5/22/13
 * 
 * Description: Helper functions for managing ptrees
 * 
 *
 * Copyright: Anki, Inc. 2013
 *
 **/
#include "ptreeTools.h"

#include "json/json.h"

#include "util/ptree/ptreeKey.h"
#include "util/ptree/ptreeTraverser.h"
#include "util/logging/logging.h"
#include "util/helpers/boundedWhile.h"
#include "util/ptree/includePtree.h"
#include "util/global/globalDefinitions.h"

#include <boost/optional.hpp>
#include "util/ptree/includeJSONParser.h"
#include <exception>
#include <map>
#include <set>
#include <unordered_map>
#include <queue>

using namespace std;
using namespace boost::property_tree;

namespace Anki{ namespace Util {

static const char* const kP_ID = "id";
static const char* const kP_EXTENDS = "extends";
static const char* const kP_PREPROCESSED = "preprocessed";

namespace PtreeTools {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace { // internal

// This class tracks extensions in a tree that is being processed. It holds an Id that is being extended
// somewhere, along with information about the places, the data defined in the id, dependencies, etc. for a
// faster preprocess algorithm.
class ExtensionId
{
public:

  // constructor
  ExtensionId() : _id("ERROR"), _ptr(nullptr), _orderValue(-1) {}
  
  // check validity
  bool IsDefined() const { return nullptr != _ptr; }

  // set self data
  void SetTree(ptree* tree, const std::string& id) {
    _id  = id;
    _ptr = tree;
  }
  
  // add a place that extends from us
  void AddExtension(ptree* place) {
    _extensions.push_back( place );
  }
  
  // add another Id that needs to be resolved before we are
  void AddDependency(ExtensionId* dep) {
    _dependencies.insert( dep );
  }

  // returns true if this id depends on the given one
  bool DependsOn(const std::string& id) const;

  // true if this Id is used anythere in the tree, false if not used
  bool IsUsed() const { return !_extensions.empty(); }
  
  // calculates sorting value if not calculated yet, otherwise it returns the current value
  int CalculateOrderValue();
  
  // applies this Id to the places where the tree should go
  void Apply();
  
  // comparator
  bool operator<(const ExtensionId& other) const {
    const bool ret = _orderValue < other._orderValue;
    return ret;
  }
  
  // debug print
  void Print() const {
    printf("+ (%d) %s [%zd uses][%zd dependencies]\n", _orderValue, _id.c_str(), _extensions.size(), _dependencies.size() );
    for( const auto& depIt : _dependencies )
    {
      printf("| - %s\n", depIt->_id.c_str() );
    }
  }
  
private:
  // id
  std::string _id;
  // this is the tree that belongs to the Id
  ptree* _ptr;
  // value used for comparisons, calculated before extensions are resolved so that we can do it in one pass
  int _orderValue;
  // these are other ids this tree has inside
  std::set<ExtensionId*> _dependencies;
  // these are the places where that id is to be extended
  std::vector<ptree*> _extensions;
};

bool ExtensionId::DependsOn(const std::string& id) const
{
  for( const auto& depIt : _dependencies )
  {
    if ( depIt->_id == id ) {
      return true;
    }
  }
  
  return false;
}

int ExtensionId::CalculateOrderValue()
{
  // if has to calculate value, do now by adding 1 to my biggest dependency
  if ( _orderValue < 0 )
  {
    if ( _dependencies.empty() )
    {
      _orderValue = 0;
    }
    else
    {
      for( auto& depIt : _dependencies )
      {
        const int depValue = depIt->CalculateOrderValue() + 1;
        _orderValue = ( depValue > _orderValue ) ? depValue : _orderValue;
      }
    }
  }
  
  assert( _orderValue >= 0 );
  return _orderValue;
}

void ExtensionId::Apply()
{

  if(_ptr == nullptr)
    return;

  // for every place that extends from us
  for(auto& extIt : _extensions)
  {
    // apply their overrides on top of our tree(_ptr), setting the result on their tree and ignoring our ids
    OrderedDeepOverride(*extIt, *_ptr, Anki::Util::kP_ID );

    // remove the extension we just resolved
    extIt->erase( Anki::Util::kP_EXTENDS );
  }
}

using ExtensionMap = std::unordered_map<std::string, ExtensionId>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This method finds all extensions/ids and builds the ExtensionId table required for fast processing
void FindExtensions(ptree& tree, ExtensionMap& idMap, std::vector<ExtensionId*>& openIds )
{
  ExtensionId* idEntry = nullptr;
  boost::optional<string> idMatch = tree.get_optional<string>(Anki::Util::kP_ID);
  if ( idMatch )
  {
    // this tree is an id itself, create entry for it
    idEntry = &idMap[*idMatch];
    
    // no dups allowed
    if ( idEntry->IsDefined() )
    {
      PRINT_NAMED_ERROR("PTreeTools.Preprocess", "The id '%s' is present multiple times", idMatch->c_str());
      throw std::runtime_error("PTreeTools.Preprocess");
    }
    
    // set data
    idEntry->SetTree( &tree, *idMatch );
//    printf("TREE ID %s\n", idMatch->data() );
//    PrintJson(tree);
  }

  // push id (important: before looking for extensions in the same tree)
  if ( nullptr != idEntry ) {
    openIds.push_back( idEntry );
  }
  
  ExtensionId* extensionEntry = nullptr;
  boost::optional<string> extendMatch = tree.get_optional<string>(Anki::Util::kP_EXTENDS);
  if ( extendMatch )
  {
    // this tree extends an id
    extensionEntry = &idMap[*extendMatch];

    // tell the other tree that we are extending from it
    extensionEntry->AddExtension( &tree );
//    printf("TREE EXTENDS %s\n", extendMatch->data() );
//    PrintJson(tree);
    
    // flag all openIds as depending on this one
    for(auto& idIt : openIds ) {
      idIt->AddDependency( extensionEntry );
    }
    
  }

  // push extension
  if ( nullptr != extensionEntry )
  {
    openIds.push_back( extensionEntry );
  }

  // check children recursively
  for( auto& treeIt : tree )
  {
    FindExtensions(treeIt.second, idMap, openIds);
  }
  
  // pop extension
  if ( nullptr != extensionEntry ) {
    assert(openIds.back() == extensionEntry);
    openIds.pop_back();
  }
  
  // pop id
  if ( nullptr != idEntry ) {
    assert(openIds.back() == idEntry);
    openIds.pop_back();
  }
}

// comparator for ExtensionId*
inline bool ExtIdPtrComp(const ExtensionId* a, const ExtensionId* b) {
  const bool lesser = (*a)<(*b);
  return lesser;
}

#if ANKI_DEVELOPER_CODE
namespace
{
  // This function recurses through the entire tree, printing anything that contains a __debug__ key,
  // value is printed as a string
  void PrintDebugTrees(ptree& tree)
  {
    boost::optional<std::string> debugFlag = tree.get_optional<std::string>("__debug__");
    if(debugFlag) {
      printf("DEBUG: printing ptree: %s\n", debugFlag->c_str());
      PrintJson(tree);
    }

    // recurse over children
    for( auto& treeIt : tree )
    {
      PrintDebugTrees(treeIt.second);
    }

  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // This method returns false if the given tree would fail to process for any reason
  bool IsValidTreeForProcess(const ptree& ptree)
  {
    const size_t extendsLen = strlen( Anki::Util::kP_EXTENDS );

    // iterate the whole tree and check that extends is not used within paths with format A.B.*
    for( const auto& entry : ptree )
    {
      const std::string& key = entry.first;
      
      // check extends is not present or it's the only thing
      size_t extendsIdx = key.find(Anki::Util::kP_EXTENDS);
      if ( extendsIdx != std::string::npos && (extendsIdx != 0 || key.size() != extendsLen) )
      {
        PRINT_NAMED_ERROR("PTreeTools", "Extends can't be defined within dot format paths '%s'", key.c_str());
        return false;
      }

      // depth check
      const bool isChildValid = IsValidTreeForProcess(entry.second);
      if ( !isChildValid ) {
        return false;
      }
    }

    return true;
  }

} // namespace
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Valid: currently unused
//// returns true if the given ptree is considered a list in JSON
//inline bool IsList(const ptree& tree) {
//  for ( auto& child : tree )
//  {
//    if ( !child.first.empty() ) {
//      return false;
//    }
//  }
//  
//  const bool hasChildren = !tree.empty();
//  return hasChildren;
//}
  
}; // annon namespace


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FastDeepOverride(ptree& where, const ptree& what)
{
  // for every key in what
  for( auto& whatEntry : what )
  {
    const std::string& key = whatEntry.first;
    
    // if what::key is empty, we consider the entry part of a list, so just put it in where
    if ( key.empty() )
    {
      where.push_back(std::make_pair("", whatEntry.second));
    }
    // if what::value has no children, then we set the value in where
    else if ( whatEntry.second.empty() )
    {
      where.put(key, whatEntry.second.data());
    }
    // entry is a ptree
    else
    {
      // if 'where' has the same key at this level
      boost::optional<ptree&> whereValue = where.get_child_optional(key);
      if( whereValue )
      {
        const bool bothTrees = !whereValue->empty() && !whatEntry.second.empty();
        if ( bothTrees )
        {
          // Some lists rely currently on merging, whereas we want to support some that override. If so, we need
          // to check here if this list (or ptree) is supposed to override instead of merge.
          //            const bool isListWhere = IsList(*whereValue);
          //            const bool isListWhat  = IsList(whatEntry.second);
          //            assert( isListWhere == isListWhat );
          const bool thisKeyListShouldOverride = false;
          if ( thisKeyListShouldOverride )
          {
            // if what::value and where::value are lists, override completely instead of merging
            where.put_child(key, whatEntry.second);
          }
          else
          {
            // if what::value and where::value are ptree, merge both trees
            FastDeepOverride(*whereValue, whatEntry.second);
          }
        }
        else if ( !whatEntry.second.empty() )
        {
          // if what::value is a ptree, put the whole tree as a child of the current key
          where.put_child(key, whatEntry.second);
        }
        else
        {
          // if what::value is not a ptree, put the data as in the current key
          where.put(key, whatEntry.second.data());
        }
      }
      else
      {
        // if 'where' does not have this key at this level, put what::value under where::key
        where.put_child(key, whatEntry.second);
      }
    }
  }
}

void OrderedDeepOverride(ptree& where, const ptree& what, const char* skipKey)
{
  // make a copy of the destination because it's expected to be smaller size
  ptree overrides = where;

  // set destination to the thing we are extending
  where = what;
  
  // remove all ids defined in the thing we are extending
  if ( skipKey ) {
    where.erase(std::string(skipKey));
  }
  
  // finally merge the overrides on top of the extension
  FastDeepOverride(where, overrides);
}

// performs a shallow merge, plus another shallow merge of one nested node
ptree DeepMergeLimited(const ptree & first, const ptree & second, const string & nestedNodeKey)
{
  // join ptrees and store
  // ------ >>>> Here be dragons
  ptree newData(first);
  boost::optional<ptree&> nestedNode = newData.get_child_optional(nestedNodeKey);
  for (ptree::const_iterator it = second.begin(); it != second.end(); ++it)
  {
    if(it->first.compare(nestedNodeKey) == 0 && nestedNode)
    {
      // join targeter definitions
      for (ptree::const_iterator nestedIt = it->second.begin(); nestedIt != it->second.end(); ++nestedIt)
        nestedNode->put_child(nestedIt->first, nestedIt->second);
    }
    else
      newData.put_child(it->first, it->second);
  }

  return newData;
}

// merges only first level nodes
ptree ShallowMergeWithCopy(const ptree & first, const ptree & second)
{
  ptree result(first);

  ptree::const_iterator end = second.end();
  for (ptree::const_iterator it = second.begin(); it != end; ++it) {
    result.put_child(it->first, it->second);
  }

  return result;
}

// merges only first level nodes
void ShallowMerge(ptree & first, const ptree & second)
{
  ptree::const_iterator end = second.end();
  for (ptree::const_iterator it = second.begin(); it != end; ++it) {
    first.put_child(it->first, it->second);
  }
}

void ResetPreprocessFlag(ptree& tree)
{
  if(tree.get<bool>(Anki::Util::kP_PREPROCESSED, false)) {
    tree.erase(Anki::Util::kP_PREPROCESSED);
  }
}

void FastPreprocess(ptree& tree)
{
  // hack for replay / basestation-main
  if(tree.get<bool>(Anki::Util::kP_PREPROCESSED, false)) {
    return;
  }

  tree.put(Anki::Util::kP_PREPROCESSED, true);

  ExtensionMap extensions;
  std::vector<ExtensionId*> openIds;

  #if DEV_ASSERT_ENABLED
  {
    DEV_ASSERT(IsValidTreeForProcess(tree), "Can't process given tree. Check previous log for info");
  }
  #endif

  // Find all ids in tree
  FindExtensions(tree, extensions, openIds);
  
  #if ANKI_DEVELOPER_CODE
  {
    // guarantee no open ids
    if ( !openIds.empty() )
    {
      PRINT_NAMED_ERROR("PtreeTools.FastPreprocess", "Not all open ids were resolved, this is a programmer error.");
      throw std::runtime_error("PtreeTools.FastPreprocess");
    }
  
    for( const auto& extensionIt : extensions )
    {
      // guarantee all ids are defined
      if ( !extensionIt.second.IsDefined() )
      {
        PRINT_NAMED_ERROR("PtreeTools.FastPreprocess", "Id '%s' is not defined, but we want to extend from it",
                              extensionIt.first.c_str() );
        throw std::runtime_error("PtreeTools.FastPreprocess");
      }
      
      // guarantee no-loops
      const bool dependsOnSelf = extensionIt.second.DependsOn( extensionIt.first );
      if ( dependsOnSelf ) {
        PRINT_NAMED_ERROR("PtreeTools.FastPreprocess", "Id '%s' depends on itself", extensionIt.first.c_str());
        throw std::runtime_error("PtreeTools.FastPreprocess");
      }
    }
  }
  #endif
  
  // 1) store in vector the Ids that are actually extended (the rest may be used for something else)
  std::vector<ExtensionId*> sortedExtensions;
  for(auto& extIdIt : extensions )
  {
    ExtensionId& extId = extIdIt.second;
    if ( extId.IsUsed() )
    {
      // calculate sorting value now
      extId.CalculateOrderValue();
      sortedExtensions.push_back( &extId );
    }
  }

  // 2) sort entries by dependencies (this guarantees we can replace in order without conflicts)
  std::sort( sortedExtensions.begin(), sortedExtensions.end(), &ExtIdPtrComp );
//    printf("-------------------------------------------\n");
//    for(const auto& sortedExtIt : sortedExtensions ) {
//      sortedExtIt->Print();
//    }
//    printf("-------------------------------------------\n");
  
  // 3) apply every extension
  for( auto& extIdPtrIt : sortedExtensions )
  {
    extIdPtrIt->Apply();
  }

  #if ANKI_DEVELOPER_CODE
  {
    PrintDebugTrees(tree);
  }
  #endif
}

void PrintJson(const ptree& config)
{
  try{
    boost::property_tree::json_parser::write_json(cout, config, true);
  }
  catch(...) {
    cout<<"<ptree cannot be represented as json>\n";
  }
}

void PrintJson(const Json::Value& jsonTree)
{
  printf("%s", jsonTree.toStyledString().c_str());
}

void PrintJson(const ptree& config, const string& key)
{
  boost::optional<const ptree& > child = config.get_child_optional(key);
  if(child) {
//    // construct a little ptree of just the key and value. There's
//    // probably a better way to do this....
//    ptree printTree;
//    printTree.put_child(key, *child);

    Anki::Util::PtreeTools::PrintJson(*child);
  }
  else {
    cout<<"< no key '"<<key<<"' found in ptree >\n";
  }
}

// Returns given json as a string
std::string StringJson(const ptree& config)
{
  try{
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, config, true);
    return ss.str();
  }
  catch(...) {
    return "<ptree cannot be represented as json>\n";
  }
}

namespace {
  static bool is_not_alnum(char c)
  {
    return !(isalnum(c) || c=='_');
  }
}

bool IsAlphaNum(const std::string &str)
{
  return find_if(str.begin(), str.end(), is_not_alnum) == str.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deprecated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Deprecated
{
namespace internal  {
  typedef map< string, pair< ptree, bool > > IDMap;

  const pair<string, pair< ptree, bool > > newMapEntry(const string& s, const ptree& p, bool b) {
    return pair<string, pair< ptree, bool > >(s, pair<ptree,bool>(p,b));
  }
}

ptree DeepMerge_Deprecated(const ptree & first, const ptree & second)
{
  // Take over first property tree
  ptree ptMerged = first;

  // Keep track of keys and values (subtrees) in second property tree
  queue<string> qKeys;
  queue<ptree> qValues;
  qValues.push( second );

  // Iterate over second property tree
  while( !qValues.empty() )
  {
    // Setup keys and corresponding values
    ptree current = qValues.front();
    qValues.pop();
    string keychain = "";
    if( !qKeys.empty() )
    {
      keychain = qKeys.front();
      qKeys.pop();
    }

    // Iterate over keys level-wise
    for( const ptree::value_type& child : current )
    {
      // Leaf
      if( child.second.size() == 0 || child.first.empty())
      {

        // node is part of a list
        if (child.first.empty())
        {
          // No "." for first level entries
          string s;
          if( keychain != "" )
            s = keychain;

          // get other node
          boost::optional<ptree &> mergeListOptional = ptMerged.get_child_optional(s);
          if (mergeListOptional)
          {
            //cout << " extending list at " << s << " : " << child.second.data() << "\n";
            mergeListOptional->push_back(make_pair("",child.second));
          } else {
            //cout << " creating a list at " << s << " : " << child.second.data() << "\n";
            ptree mergeList;
            mergeList.push_back(make_pair("",child.second));
            ptMerged.put_child(s, mergeList);
          }
        }
        else
        {
          // No "." for first level entries
          string s;
          if( keychain != "" )
            s = keychain + "." + child.first;
          else
            s = child.first;
          
          //cout << "putting data - " << s << " : " << child.second.data() << "\n";
          // Put into combined property tree
          ptMerged.put( s, child.second.data() );

        }
      }
        // Subtree
      else
      {
        // Put keys (identifiers of subtrees) and all of its parents (where present)
        // aside for later iteration. Keys on first level have no parents
        if( keychain != "" )
          qKeys.push( keychain + "." + child.first );
        else
          qKeys.push( child.first );
        
        // Put values (the subtrees) aside, too
        qValues.push( child.second );
      }
    }  // -- End of BOOST_FOREACH
  }  // --- End of while

  return ptMerged;
}

void Preprocess_Deprecated(ptree& tree)
{
  // hack for replay / basestation-main
  if(tree.get<bool>(Anki::Util::kP_PREPROCESSED, false))
    return;

  // This function merges every ptree defined by "id" into the place
  // it is referenced with "extends"

  // TODO:(bn) There is a more efficient way to do this. We need to
  // build a graph out of id's and extends where the roots are
  // "complete" ptrees that have no dependencies. Then we traverse
  // this graph, merging ptrees as we go

  // The first step is to iterate the map once to store all of the
  // ids. The value has a ptree and a bool which if true means that
  // the ptree is "complete" and contains no remaining "extends"
  // references
  internal::IDMap idMap;

  // Also keep track of each "extends" reference
  multimap<string, PtreeKey> extendsKeys;

  // We will also keep track of the number of "extends"
  // references. When that reaches zero, we have a fully defined tree
  unsigned int numRefs = 0;

  PtreeTraverser firstPass(tree);

  tree.put(Anki::Util::kP_PREPROCESSED, true);

  BOUNDED_WHILE(50000, !firstPass.done()) {
    boost::optional<string> id = firstPass.topTree().get_optional<string>(Anki::Util::kP_ID);
    if(id) {
      ptree t = firstPass.topTree();
      t.erase(Anki::Util::kP_ID);
      if(idMap.insert(internal::newMapEntry(*id, t, false)).second == false) {
        PRINT_NAMED_WARNING("PTreeTools.Preprocess.DuplicateID", "The id '%s' is present multiple times",
                              id->c_str());
        throw std::runtime_error("PTreeTools.Preprocess.DuplicateID");
        // if we don't throw here it'll just cause a confusing error later
      }
      idMap[*id].first.erase(Anki::Util::kP_ID);
    }

    boost::optional<string> extends = firstPass.topTree().get_optional<string>(Anki::Util::kP_EXTENDS);
    if(extends) {
      numRefs++;
      extendsKeys.emplace(*extends, firstPass.topKey());
    }

    firstPass.next();
  }

  PRINT_NAMED_DEBUG("Preprocess_Deprecated", "There are %d references and %zd ids",
                  numRefs, idMap.size());

  if(idMap.empty())
    return;

  // now we'll go through the id map and make sure it is fully resolved
  unsigned int numIDRefs = numRefs;
  internal::IDMap::iterator end = idMap.end();

  BOUNDED_WHILE(100, numIDRefs > 0) {

    PRINT_NAMED_INFO("PTreeTools.PreprocessIteration", "phase 2 numIDRefs: %d", numIDRefs);

    numIDRefs = 0;

    // cout<<"\nmap:\n";
    // for(internal::IDMap::iterator it = idMap.begin(); it != end; ++it) {
    //   cout<<it->first<<": complete = "<<it->second.second<<endl;
    //   Anki::Util::PtreeTools::PrintJson((it->second.first);
    // }

    for(internal::IDMap::iterator it = idMap.begin(); it != end; ++it) {
      if(it->second.second)
        continue;

      it->second.second = true;

      PtreeTraverser trav(it->second.first);

      BOUNDED_WHILE(1000, !trav.done()) {
        boost::optional<string> extends = trav.topTree().get_optional<string>(Anki::Util::kP_EXTENDS);
        if(extends) {
          numIDRefs++;

          internal::IDMap::iterator targetIt = idMap.find(*extends);
          if(targetIt != end) {
            pair<ptree, bool>& target = targetIt->second;
            if(target.second) {
              ptree child = trav.topTree();
              child.erase(Anki::Util::kP_EXTENDS);

              // usually child will over-write things in target.first,
              // so print warnings for things that aren't there. Just do
              // one level deep
              for(const ptree::value_type& v : child) {
                boost::optional<ptree&> other = target.first.get_child_optional(v.first);
                // TODO: (bn) figure out a nicer way to do this
                //              if(!other) {
                //                PRINT_NAMED_WARNING("PtreeTools.PossibleTypo",
                //                                        "When extending key '%s' you overwrote key '%s' which was not present in id",
                //                                        extends->c_str(),
                //                                        v.first.c_str());
                //              }
              }

              ptree newChild = DeepMerge_Deprecated(target.first, child);

              GetChild_Deprecated(it->second.first, trav.topKey()) = newChild;
            }

            it->second.second = false;
          }
          else {
            PRINT_NAMED_ERROR("PtreeTools.InvalidConfig",
                                  "id '%s' not found in the map",
                                  extends->c_str());
          }
        }
        trav.next();
      }
    }
  }

  // and finally go back through the ptree and merge in the now fully resolved ids
  multimap<string, PtreeKey>::iterator endKey = extendsKeys.end();
  for(multimap<string, PtreeKey>::iterator it = extendsKeys.begin(); it != endKey; ++it) {

    internal::IDMap::iterator idIter = idMap.find(it->first);
    if(idIter == idMap.end()) {
      PRINT_NAMED_ERROR("PtreeTools.InternalError",
                            "Key '%s' not found in id map",
                            it->first.c_str());
      continue;
    }

    if(!idIter->second.second) {
      PRINT_NAMED_ERROR("Ptree.CouldNotResolveKey", "could not fully resolve id '%s'", it->first.c_str());
      throw std::runtime_error("PTreeTools.CouldNotResolveKey");
    }

    ptree child = GetChild_Deprecated(tree, it->second);
    child.erase(Anki::Util::kP_EXTENDS);

    ptree newChild = DeepMerge_Deprecated(idMap[it->first].first, child);

    GetChild_Deprecated(tree, it->second) = newChild;
  }

  #define DEBUG_PTREE 0
  #if DEBUG_PTREE
  {
    printf("\n\nprocessed ptree config:\n");
    Anki::Util::PtreeTools::PrintJson(tree);
  }
  #endif

}

ptree& GetChild_Deprecated(ptree& tree, const PtreeKey& key)
{
  ptree *curr = &tree;

  for(unsigned int i=0; i<key.size(); ++i) {
    curr = &curr->get_child(key[i].first);

    if(key[i].second >= 0) {
      std::pair< ptree::assoc_iterator, ptree::assoc_iterator >
        bounds = curr->equal_range("");

      unsigned long int dist = distance(bounds.first, bounds.second);
      if(dist > 0) {
        // we have a list
        if(key[i].second >= dist) {
          PRINT_NAMED_ERROR("PTreeTools.GetChild.IndexOutOfBounds", "Index of %d exceeds distance of %ld",
                                key[i].second, dist);
          throw std::runtime_error("PTreeTools.GetChild.IndexOutOfBounds");
        }

        advance(bounds.first, key[i].second);

        curr = &bounds.first->second;
      }
    }
    // otherwise, we want the whole list
  }

  return *curr;
}


}

} // namespace
} // namespace
} // namespace
