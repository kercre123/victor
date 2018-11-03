/**
 * File: symmetricMap.h
 *
 * Author: Andrew Stein
 * Created: 10/26/2018
 *
 * Description: Symmetric map for associating mappings between two values and looking up
 *              either one by the other.
 *
 *              NOTE: Current implementation is not particularly memory efficient and stores two
 *                    of everything inserted. For small maps and simple data types, this probably
 *                    doesn't matter, but could be improved for more general use. (VIC-11264)
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Util_Containers_SymmetricMap_H__
#define __Anki_Util_Containers_SymmetricMap_H__

#include <map>
#include <set>

namespace Anki {
namespace Util {
  
// Full template definition with "Enable" template parameter to support
// SFINAE approach to creating specailizations depending on whether
// TypeA and TypeB are the same type
template <class TypeA, class TypeB, typename Enable = void>
class SymmetricMap;

// Macros for better readability below in instantiating
// specializations for TypeA != TypeB and TypeA == TypeB
#define SameTypes std::enable_if_t<std::is_same<TypeA,TypeB>::value>
#define DiffTypes std::enable_if_t<!std::is_same<TypeA,TypeB>::value>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Specialization for General Case: TypeA != TypeB
//
template <class TypeA, class TypeB>
class SymmetricMap<TypeA, TypeB, DiffTypes>
{
public:
  
  // Example instantiation:
  //
  //  SymmetricMap<int, std::string> symMap{
  //    {42,        "Meaning of Life"},
  //    {100000000, "Big Number"},
  //    {-1,        "Negative"},
  //    {0,         "Nil"},
  //    {42,        "Dolphins"},
  //    {-2,        "Negative"},
  //  };
  //
  SymmetricMap(const std::initializer_list<std::pair<TypeA, TypeB>>& args);
  
  // Inserts the mapping (forward and reverse mapping)
  void Insert(const TypeA& a, const TypeB& b);
  
  // Remove a specific mapping
  void Erase(const TypeA& a, const TypeB& b);
  
  // Remove all entries involving the given value
  //
  // Example using instantation above:
  //  symMap.Erase(42) will remove both (42, "Meaning of Life") and (42, "Dolphins")
  //  symMap.Erase("Negative") will remove both (-1, "Negative) and (-2, "Negative")
  //
  void Erase(const TypeA& a);
  void Erase(const TypeB& b);
  
  // Find: Adds all elements associated with the given key to values. (NOTE: does not clear values first!)
  //
  // Examples using instantiation above:
  //  symMap.Find(42, strs) will populate strs with {"Dolphins", "Meaning of Life"}
  //  symMap.Find("Nil", vals) will populate vals with 0
  //  symMap.Find("foo", strs) will not change strs
  //
  void Find(const TypeA& key, std::set<TypeB>& values) const;
  void Find(const TypeB& key, std::set<TypeA>& values) const;
  
  // GetKeys: Adds all keys of the first or second type to given set. (NOTE: does not clear keys first!)
  //
  // Examples using instantiation above:
  //  std::set<int> keys;
  //  symMap.GetKeys(keys) will populate keys with {42, 100000000, -1, 0, -2}
  //
  void GetKeys(std::set<TypeA>& keys) const;
  void GetKeys(std::set<TypeB>& keys) const;
  
private:
  
  // TODO: Avoid duplicate storage (VIC-11264)
  std::multimap<TypeA,TypeB> _AtoB;
  std::multimap<TypeB,TypeA> _BtoA;
  
  void EraseHelper(const TypeA& a, const TypeB& b);
  void EraseHelper(const TypeB& b, const TypeA& a);
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Specialization for Special Case: TypeA == TypeB
//
template<class TypeA, class TypeB>
class SymmetricMap<TypeA, TypeB, SameTypes>
{
public:
  
  // (See above for API documentation)
  
  SymmetricMap(const std::initializer_list<std::pair<TypeA, TypeB>>& args);
  
  void Insert(const TypeA& a, const TypeB& b);
  
  void Erase(const TypeA& key);
  void Erase(const TypeA& a, const TypeB& b);
  
  void Find(const TypeA& key, std::set<TypeB>& values) const;
  
  void GetKeys(std::set<TypeA>& keys) const;
  
private:
  std::multimap<TypeA,TypeB> _map;
  
  void EraseHelper(const TypeA& a, const TypeB& b);
};
  
  
//
// Templated implementations
//
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
SymmetricMap<TypeA,TypeB,DiffTypes>::SymmetricMap(const std::initializer_list<std::pair<TypeA, TypeB>>& args)
{
  for(const auto& pair : args)
  {
    Insert(pair.first, pair.second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::GetKeys(std::set<TypeA>& keys) const
{
  for(const auto& entry : _AtoB)
  {
    keys.insert(entry.first);
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::GetKeys(std::set<TypeB>& keys) const
{
  for(const auto& entry : _BtoA)
  {
    keys.insert(entry.first);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Insert(const TypeA& a, const TypeB& b)
{
  _AtoB.emplace(a,b);
  _BtoA.emplace(b,a);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Erase(const TypeA& a)
{
  auto rangeA = _AtoB.equal_range(a);
 
  for(auto iterA = rangeA.first; iterA != rangeA.second; ++iterA)
  {
    // Erase reverse mappings *to* a from any b in a's list
    EraseHelper(iterA->second, a);
  }
  
  // Erase all forward
  _AtoB.erase(rangeA.first, rangeA.second);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Erase(const TypeB& b)
{
  auto rangeB = _BtoA.equal_range(b);
  
  for(auto iterB = rangeB.first; iterB != rangeB.second; ++iterB)
  {
    // Erase all reverse mappings *to* b from any a in b's list
    EraseHelper(iterB->second, b);
  }
  
  _BtoA.erase(rangeB.first, rangeB.second);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Erase(const TypeA& a, const TypeB& b)
{
  // Look for individual (a,b) and erase it if we find it
  EraseHelper(a,b);
  
  // Look for individual (b,a) and erase it if we find it
  EraseHelper(b,a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::EraseHelper(const TypeA& a, const TypeB& b)
{
  auto range = _AtoB.equal_range(a);
  auto iter = range.first;
  while(iter != range.second)
  {
    if(iter->second == b)
    {
      iter = _AtoB.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::EraseHelper(const TypeB& b, const TypeA& a)
{
  auto range = _BtoA.equal_range(b);
  auto iter = range.first;
  while(iter != range.second)
  {
    if(iter->second == a)
    {
      iter = _BtoA.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Find(const TypeA& key, std::set<TypeB>& values) const
{
  auto range = _AtoB.equal_range(key);
  for(auto iter=range.first; iter != range.second; ++iter)
  {
    values.insert(iter->second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,DiffTypes>::Find(const TypeB& key, std::set<TypeA>& values) const
{
  auto range = _BtoA.equal_range(key);
  for(auto iter=range.first; iter != range.second; ++iter)
  {
    values.insert(iter->second);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
SymmetricMap<TypeA,TypeB,SameTypes>::SymmetricMap(const std::initializer_list<std::pair<TypeA, TypeB>>& args)
{
  for(const auto& pair : args)
  {
    Insert(pair.first, pair.second);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::Insert(const TypeA& a, const TypeB& b)
{
  _map.emplace(a,b);
  _map.emplace(b,a);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::Erase(const TypeA& a)
{
  auto range = _map.equal_range(a);
 
  for(auto iter = range.first; iter != range.second; ++iter)
  {
    if(iter->second != a)
    {
      EraseHelper(iter->second, a);
    }
  }
  
  _map.erase(range.first, range.second);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::Erase(const TypeA& a, const TypeB& b)
{
  EraseHelper(a, b);
  EraseHelper(b, a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::EraseHelper(const TypeA& a, const TypeB& b)
{
  auto range = _map.equal_range(a);
  auto iter = range.first;
  while(iter != range.second)
  {
    if(iter->second == b)
    {
      iter = _map.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::Find(const TypeA& key, std::set<TypeB>& values) const
{
  auto range = _map.equal_range(key);
  for(auto iter = range.first; iter != range.second; ++iter)
  {
    values.insert(iter->second);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class TypeA, class TypeB>
void SymmetricMap<TypeA,TypeB,SameTypes>::GetKeys(std::set<TypeA>& keys) const
{
  for(const auto& entry : _map)
  {
    keys.insert(entry.first);
  }
}
  
#undef SameTypes
#undef DiffTypes
  
} // namespace Util
} // namespace Anki

#endif /* __Anki_Util_Containers_SymmetricMap_H__ */
