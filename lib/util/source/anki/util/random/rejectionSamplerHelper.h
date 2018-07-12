/**
 * File: rejectionSamplerHelper.h
 *
 * Author: ross
 * Created: 2018 Jun 29
 *
 * Description: Helper class for rejection sampling. This is _not_ a way to sample from a distribution with
 *              known functional form using samples from another distribution. It is simply a way to generate
 *              one or more samples from a complex region with no functional form by evaluating conditions
 *              that indicate if a point sampled from a larger space lies within the region.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Util_Random_RejectionSamplerHelper_H__
#define __Util_Random_RejectionSamplerHelper_H__

#include "util/helpers/templateHelpers.h"
#include "util/random/rejectionSamplerHelper_fwd.h"

#include <functional>
#include <list>
#include <vector>

namespace Anki{
namespace Util{

class RandomGenerator;
template <typename T>
class RejectionSamplingCondition;

// Second template param is optional, defaulting to false (it is declared in the _fwd).
// If you expect to be inserting conditions somewhere other than the end or using AddScopedCondition,
// consider using ExpectConditionReordering = true.

template <typename T, bool ExpectConditionReordering>
class RejectionSamplerHelper
{
public:
  
  RejectionSamplerHelper() = default;
  
  // should return true if the sample of type T is to be accepted, false otherwise
  using ConditionFunc = std::function<bool(const T&)>;
  
  // a generic handle to a condition added for accepting/rejecting
  using ConditionHandle = std::shared_ptr<RejectionSamplingCondition<T>>;
  
  // a handle to a condition of a specific type, added for accepting/rejecting
  template <typename CondType>
  using HandleType = typename std::enable_if< std::is_base_of<RejectionSamplingCondition<T>, CondType>::value,
                                              std::shared_ptr<CondType> >::type;
  
  // Add a sample evaluation condition. The optional handle param defines the insertion point. If
  // provided, the new condition will be evaluated before that of the handle. If omitted, the condition will
  // be evaluated in the order added to this class, either using this method or AddScopedCondition.
  // If you pass in a lambda, you'll get a return value of a generic ConditionHandle.
  // If you pass in a shared_ptr to a class that derives from RejectionSamplingCondition, you'll get a handle
  // that lets you access that class.
  // Add conditions in an order that makes sense for your problem -- the first conditions should not
  // be those that whittle away at edge cases, but instead should slice out chunks of the outcome space,
  // so that fewer conditions need evaluation.
  ConditionHandle AddCondition( ConditionFunc&& cond,
                                const ConditionHandle& before = nullptr );
  template <typename CondType>
  HandleType<CondType> AddCondition( std::shared_ptr<CondType> cond,
                                     const ConditionHandle& before = nullptr );

  // Add a sample evaluation condition. See comments for AddCondition. For AddScopedCondition,
  // the condition will be removed when either Evaluate or Generate is called by the time the return value
  // has gone out of scope. I.e., if the return value goes out of scope, it won't be used, but the use_count
  // of the handle won't decrement until Evaluate or Generate is called.
  ConditionHandle AddScopedCondition( ConditionFunc&& cond,
                                      const ConditionHandle& before = nullptr ) __attribute__((warn_unused_result));
  template <typename CondType>
  HandleType<CondType> AddScopedCondition( std::shared_ptr<CondType> cond,
                                           const ConditionHandle& before = nullptr ) __attribute__((warn_unused_result));
  
  // Removes a condition by its handle
  void RemoveCondition( const ConditionHandle& handle );
  
  // Clears all added conditions
  void ClearConditions();
  
  // Returns true if sample should be accepted. Be smart about your initial guesses. Considering the conditions,
  // try to maximize the acceptance probability, or this algorithm can be quite wasteful.
  // If no conditions exist, this returns true for any sample!
  bool Evaluate( RandomGenerator& rng, const T& sample );
  
  // Generate N accepted samples by supplying a generator of initial samples. The generator will be
  // run a maximum of maxAttempts times before returning whatever samples have been accepted so far,
  // which may be less than N, so be sure to check the return size.
  // If no conditions exist, this returns true for any sample!
  using GeneratorFunc = std::function<T()>;
  std::vector<T> Generate( RandomGenerator& rng,
                           unsigned int N,
                           const GeneratorFunc& generatorFunc,
                           unsigned int maxAttempts = 1000 );
  
  // If you need to verify that there are conditions remaining after removing some, use this.
  size_t GetNumConditions() { return _conditions.size(); }
  
private:
  
  ConditionHandle AddConditionInternal( std::shared_ptr<RejectionSamplingCondition<T>> cond,
                                        const ConditionHandle& before,
                                        bool scoped );
  
  bool EvaluateInternal( RandomGenerator& rng, const T& sample ) const;
  
  void PruneOutOfScopeConditions();
  
  // helper class to wrap a lambda passed to Add(Scoped)Condition as a RejectionSamplingCondition
  class LambdaWrapperCondition : public RejectionSamplingCondition<T> {
  public:
    explicit LambdaWrapperCondition( ConditionFunc&& cond ) : _cond{std::move(cond)} {}
    virtual bool operator()(const T& sample) override {
      return _cond( sample );
    }
    ConditionFunc _cond;
  };
  struct Condition {
    ConditionHandle cond;
    bool scoped;
  };

  using ContainerType = typename Anki::Util::type_from_bool<ExpectConditionReordering,
                                                            std::list<Condition>, // when expecting condition re-ordering
                                                            std::vector<Condition>>::type; // when !expecting
  ContainerType _conditions;
  bool _hasScopedConditions = false;
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helper class that mimics a mutable lambda, but allows subclasses to add their own methods to
// modify internal params after the condition has already been passed to the RejectionSamplerHelper
template <typename T>
class RejectionSamplingCondition {
public:
  virtual ~RejectionSamplingCondition() = default;
  // should return true if the sample T is to be accepted, false otherwise
  virtual bool operator()(const T&) = 0; // non-const for flexibility, like a mutable lambda
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
  typename RejectionSamplerHelper<T,B>::ConditionHandle
RejectionSamplerHelper<T,B>::AddCondition( RejectionSamplerHelper<T,B>::ConditionFunc&& cond,
                                           const RejectionSamplerHelper<T,B>::ConditionHandle& before )
{
  return AddConditionInternal( std::make_shared<LambdaWrapperCondition>( std::forward<ConditionFunc>(cond) ), before, false );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
template <typename CondType>
  typename RejectionSamplerHelper<T,B>::template HandleType<CondType>
RejectionSamplerHelper<T,B>::AddCondition( std::shared_ptr<CondType> cond,
                                           const ConditionHandle& before )
{
  AddConditionInternal( cond, before, false );
  return cond;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
  typename RejectionSamplerHelper<T,B>::ConditionHandle
RejectionSamplerHelper<T,B>::AddScopedCondition( RejectionSamplerHelper<T,B>::ConditionFunc&& cond,
                                                 const RejectionSamplerHelper<T,B>::ConditionHandle& before )
{
  _hasScopedConditions = true;
  return AddConditionInternal( std::make_shared<LambdaWrapperCondition>( std::forward<ConditionFunc>(cond) ), before, true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
template <typename CondType>
  typename RejectionSamplerHelper<T,B>::template HandleType<CondType>
RejectionSamplerHelper<T,B>::AddScopedCondition( std::shared_ptr<CondType> cond,
                                                 const RejectionSamplerHelper<T,B>::ConditionHandle& before )
{
  _hasScopedConditions = true;
  AddConditionInternal( cond, before, true );
  return cond;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
  typename RejectionSamplerHelper<T,B>::ConditionHandle
RejectionSamplerHelper<T,B>::AddConditionInternal( std::shared_ptr<RejectionSamplingCondition<T>> cond,
                                                   const RejectionSamplerHelper<T,B>::ConditionHandle& before,
                                                   bool scoped )
{
  auto itInsert = _conditions.end();
  if( before != nullptr ) {
    for( auto itExisting = _conditions.begin(); itExisting != _conditions.end(); ++itExisting ) {
      if( itExisting->cond == before ) {
        itInsert = itExisting;
        break;
      }
    }
  }
  Condition myCondition{cond, scoped};
  _conditions.emplace( itInsert, std::move(myCondition) );
  return cond;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
void RejectionSamplerHelper<T,B>::RemoveCondition( const RejectionSamplerHelper<T,B>::ConditionHandle& handle )
{
  bool anyScoped = false;
  for( auto it = _conditions.begin(); it != _conditions.end(); ) {
    if( it->cond == handle ) {
      it = _conditions.erase( it );
      if( !_hasScopedConditions ) {
        break;
      }
    } else {
      anyScoped |= it->scoped;
      ++it;
    }
  }
  _hasScopedConditions = anyScoped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
void RejectionSamplerHelper<T,B>::ClearConditions()
{
  _hasScopedConditions = false;
  _conditions.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
bool RejectionSamplerHelper<T,B>::Evaluate( RandomGenerator& rng, const T& sample )
{
  if( _hasScopedConditions ) {
    PruneOutOfScopeConditions();
  }
  return EvaluateInternal( rng, sample );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
std::vector<T> RejectionSamplerHelper<T,B>::Generate( RandomGenerator& rng,
                                                      unsigned int N,
                                                      const RejectionSamplerHelper<T,B>::GeneratorFunc& generatorFunc,
                                                      unsigned int maxAttempts )
{
  std::vector<T> acceptedSamples;
  acceptedSamples.reserve( N );
  
  if( _hasScopedConditions ) {
    PruneOutOfScopeConditions();
  }
  
  unsigned int numAccepted = 0;
  for( unsigned int i=0; i<maxAttempts; ++i ) {
    T sample = generatorFunc();
    if( Evaluate(rng, sample) ) {
      acceptedSamples.push_back( std::move(sample) );
      ++numAccepted;
      if( numAccepted >= N ) {
        break;
      }
    }
  }
  
  return acceptedSamples;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
bool RejectionSamplerHelper<T,B>::EvaluateInternal( RandomGenerator& rng, const T& sample ) const
{
  for( const auto& condition : _conditions ) {
    if( !condition.cond->operator()( sample ) ) {
      return false;
    }
  }
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename T, bool B>
void RejectionSamplerHelper<T,B>::PruneOutOfScopeConditions()
{
  bool anyScoped = false;
  for( auto it = _conditions.begin(); it != _conditions.end(); ) {
    if( it->scoped && (it->cond.use_count() == 1) ) {
      it = _conditions.erase( it );
    } else {
      anyScoped |= it->scoped;
      ++it;
    }
  }
  _hasScopedConditions = anyScoped;
}
  
} // namespace
} // namespace

#endif
