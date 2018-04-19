/**
 * File: testRandomIndexSampler
 *
 * Author: ross
 * Created: Apr 3 2018
 *
 * Description: Unit tests for RandomIndexSampler
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=RandomIndexSampler.*
 **/


#include "util/helpers/includeGTest.h"
#include "util/random/randomIndexSampler.h"
#include "util/random/randomGenerator.h"

#include <utility>

using namespace Anki::Util;

bool EachElementAppearsExactlyOnce( const std::vector<int>& elements, int& badElem )
{
  std::set<int> uniqElems;
  for( const auto& elem : elements ) {
    const auto it = uniqElems.insert( elem );
    if( !it.second ) {
      badElem = elem;
      return false;
    }
  }
  return !elements.empty();
}

struct PairHash
{
    template <class T1, class T2>
    std::size_t operator () (std::pair<T1, T2> const &pair) const
    {
        std::size_t h1 = std::hash<T1>()(pair.first);
        std::size_t h2 = std::hash<T2>()(pair.second);

        return h1 ^ h2;
    }
};

bool EachElementHasSameProbability( const std::vector<std::vector<int>>& elementsList, float tolOverall, float tolPositional )
{
  const bool requireInAnyPosition = tolPositional >= 0.0f;
  
  // (vector position, the element value) to count
  std::unordered_map<std::pair<int,int>, float, PairHash> countPerPosition;
  // the element value to count
  std::unordered_map<int, float> totalCount;
  
  //const size_t numSamples = elementsList.size();
  const size_t numPositions = elementsList[0].size();
  
  for( const auto& vec : elementsList ) {
    if( vec.size() != numPositions ) {
      return false; // sizes should be the same
    }
    for( int i=0; i<vec.size(); ++i ) {
      std::pair<int,int> p = std::make_pair(i, vec[i]);
      ++countPerPosition[ p ];
      ++totalCount[ vec[i] ];
    }
  }
  
  const size_t numElements = totalCount.size();
  
  float avgTotalCount = 0.0f;
  for( const auto& totalPair : totalCount ) {
    avgTotalCount += totalPair.second / numElements;
  }
  
  // for each position, the avg count
  std::vector<float> avgPositionCount( numPositions, 0.0f );
  if( requireInAnyPosition ) {
    for( const auto& perPositionPair : countPerPosition ) {
      auto& avg = avgPositionCount[perPositionPair.first.first];
      avg += perPositionPair.second / numElements;
    }
  }
  
  // now get variances
  float varianceAll = 0.0f;
  for( const auto& totalPair : totalCount ) {
    varianceAll += (totalPair.second - avgTotalCount)*(totalPair.second - avgTotalCount) / numElements;
  }
  if( sqrt(varianceAll) > tolOverall * avgTotalCount ) {
    return false;
  }
  
  if( requireInAnyPosition ) {
    std::vector<float> variancePerPosition( numPositions, 0.0f );
    for( const auto& perPositionPair : countPerPosition ) {
      auto& var = variancePerPosition[perPositionPair.first.first];
      auto& avg = avgPositionCount[perPositionPair.first.first];
      var += (perPositionPair.second - avg)*(perPositionPair.second - avg) / numElements;
    }
    for( int i=0; i<variancePerPosition.size(); ++i ) {
      const auto& var = variancePerPosition[i];
      float avg = avgPositionCount[i];
      if( sqrt(var) > tolPositional * avg  ) {
        return false;
      }
    }
  }
  
  return true;
}

struct TestInfo {
  enum GetType {
    GET_NEXT,
    GET_ALL
  };
  GetType getType;
  RandomIndexSampler::OrderType orderType;
  bool isStaticShuffler = false;
};
// separate so the actual test can EXPECT_*
std::string RunTest(RandomGenerator& rng,
                    TestInfo info,
                    int populationSize, int numSamples, int numVectors,
                    float tolOverall, float tolPositional,
                    bool shouldMatchTol=true)
{
  RandomIndexSampler sampler( populationSize, numSamples, info.orderType );
  std::vector<std::vector<int>> samples(numVectors, std::vector<int>() );
  for( auto& vec : samples ) {
    if( info.isStaticShuffler ) {
      vec = RandomIndexSampler::CreateFullShuffledVector( rng, populationSize );
    } else {
      if( info.getType == TestInfo::GET_ALL ) {
        vec = sampler.GetAll( rng );
      } else {
        vec.resize(numSamples, 0);
        for( int i=0; i<numSamples; ++i ) {
          vec[i] = sampler.GetNext( rng );
        }
      }
    }
    sampler.Reset();
    
    EXPECT_EQ( (int)vec.size(), numSamples ) << "Sampled vector has incorrect size";
    
    int badElem=0;
    const bool onlyOnce = EachElementAppearsExactlyOnce( vec, badElem );
    if( !onlyOnce ) {
      return std::string("element ") + std::to_string( badElem )  + " didnt appear exactly once";
    }
  }
  
  // dont do statistical checks unless a tolerance was specified
  if( tolOverall >= 0.0f || tolPositional>=0.0f ) {
    const bool sameP = EachElementHasSameProbability( samples, tolOverall, tolPositional );
    if( sameP != shouldMatchTol ) {
      if( shouldMatchTol ) {
        return "Deviation from the mean exceeded bounds";
      } else {
        return "Deviation from the mean didnt exceed bounds";
      }
    }
  }
  
  return std::string{};
}

TEST(RandomIndexSampler, RandomOrderGetAll)
{
  RandomGenerator rng(123);
  
  TestInfo info;
  info.getType = TestInfo::GetType::GET_ALL;
  info.orderType = RandomIndexSampler::ORDER_SHOULD_BE_RANDOM;
  
  // choosing 10 out of 10 (0 variance overall since always 10/10)
  std::string result1 = RunTest( rng, info, 10, 10, 5000, 0.0f, 0.1f );
  EXPECT_TRUE( result1.empty() && "choosing 10 out of 10" ) << result1;
  
  // only choosing 5 out of 10,
  std::string result2 = RunTest( rng, info, 10, 5, 1000, 0.05f, 0.2f );
  EXPECT_TRUE( result2.empty() && "choosing 5 out of 10" ) << result2;
  // positional deviation converges
  std::string result3 = RunTest( rng, info, 10, 5, 1000, 0.05f, 0.1f, false );
  EXPECT_TRUE( result3.empty() && "choosing 5 out of 10 should exceed variance "  ) << result3;
  std::string result4 = RunTest( rng, info, 10, 5, 5000, 0.05f, 0.1f );
  EXPECT_TRUE( result4.empty() && "choosing 5 out of 10 with more samples should show convergence "  ) << result4;
}

TEST(RandomIndexSampler, RandomOrderGetNext)
{
  RandomGenerator rng(123);
  
  TestInfo info;
  info.getType = TestInfo::GetType::GET_NEXT;
  info.orderType = RandomIndexSampler::ORDER_SHOULD_BE_RANDOM;
  
  // choosing 10 out of 10 (0 variance overall since always 10/10)
  std::string result1 = RunTest( rng, info, 10, 10, 5000, 0.0f, 0.1f );
  EXPECT_TRUE( result1.empty() && "choosing 10 out of 10" ) << result1;
  
  // only choosing 5 out of 10,
  std::string result2 = RunTest( rng, info, 10, 5, 1000, 0.05f, 0.2f );
  EXPECT_TRUE( result2.empty() && "choosing 5 out of 10" ) << result2;
  // positional deviation converges
  std::string result3 = RunTest( rng, info, 10, 5, 1000, 0.05f, 0.1f, false );
  EXPECT_TRUE( result3.empty() && "choosing 5 out of 10 should exceed variance "  ) << result3;
  std::string result4 = RunTest( rng, info, 10, 5, 5000, 0.05f, 0.1f );
  EXPECT_TRUE( result4.empty() && "choosing 5 out of 10 with more samples should show convergence "  ) << result4;
}

TEST(RandomIndexSampler, RandomOrderStaticShuffler)
{
  RandomGenerator rng(123);
  
  TestInfo info;
  info.isStaticShuffler = true;
  
  // choosing 10 out of 10 (0 variance overall since always 10/10)
  std::string result1 = RunTest( rng, info, 10, 10, 1000, 0.0f, 0.2f );
  EXPECT_TRUE( result1.empty() && "choosing 10 out of 10 static shuffler" ) << result1;
  // tighten tol and it should exceed it
  std::string result2 = RunTest( rng, info, 10, 10, 1000, 0.0f, 0.1f, false );
  EXPECT_TRUE( result2.empty() && "choosing 10 out of 10 static shuffler should exceed" ) << result2;
  // converges
  std::string result3 = RunTest( rng, info, 10, 10, 5000, 0.0f, 0.1f );
  EXPECT_TRUE( result3.empty() && "choosing 10 out of 10 static shuffler should show convergence" ) << result3;
}

TEST(RandomIndexSampler, FloydGetAll)
{
  RandomGenerator rng(123);
  
  TestInfo info;
  info.getType = TestInfo::GetType::GET_ALL;
  info.orderType = RandomIndexSampler::ORDER_DOESNT_MATTER;
  
  // don't check randomness
  const float dontCheck = -1.0f;
  
  // choosing 10 out of 10 (order will NOT be random, so just check that all elements appear once)
  std::string result1 = RunTest( rng, info, 10, 10, 5000, dontCheck, dontCheck );
  EXPECT_TRUE( result1.empty() && "choosing 10 out of 10" ) << result1;
  
  // only choosing 5 out of 10, elements should appear randomly, but their positions in the sampled vector wont be
  std::string result2 = RunTest( rng, info, 10, 9, 1000, 0.05f, dontCheck );
  EXPECT_TRUE( result2.empty() && "choosing 5 out of 10" ) << result2;
  std::string result3 = RunTest( rng, info, 10, 5, 1000, 0.01f, dontCheck, false );
  EXPECT_TRUE( result3.empty() && "choosing 5 out of 10 should exceed variance" ) << result3;
  std::string result4 = RunTest( rng, info, 10, 5, 8000, 0.01f, dontCheck );
  EXPECT_TRUE( result4.empty() && "choosing 5 out of 10 show convergence" ) << result4;
}

TEST(RandomIndexSampler, FloydGetNext)
{
  RandomGenerator rng(123);
  
  TestInfo info;
  info.getType = TestInfo::GetType::GET_NEXT;
  info.orderType = RandomIndexSampler::ORDER_DOESNT_MATTER;
  
  // don't check randomness
  const float dontCheck = -1.0f;
  
  // choosing 10 out of 10 (order will NOT be random, so just check that all elements appear once)
  std::string result1 = RunTest( rng, info, 10, 10, 5000, dontCheck, dontCheck );
  EXPECT_TRUE( result1.empty() && "choosing 10 out of 10" ) << result1;
  
  // only choosing 5 out of 10, elements should appear randomly, but their positions in the sampled vector wont be
  std::string result2 = RunTest( rng, info, 10, 5, 1000, 0.05f, dontCheck );
  EXPECT_TRUE( result2.empty() && "choosing 5 out of 10" ) << result2;
  std::string result3 = RunTest( rng, info, 10, 5, 1000, 0.01f, dontCheck, false );
  EXPECT_TRUE( result3.empty() && "choosing 5 out of 10 should exceed variance" ) << result3;
  std::string result4 = RunTest( rng, info, 10, 5, 8000, 0.01f, dontCheck );
  EXPECT_TRUE( result4.empty() && "choosing 5 out of 10 show convergence" ) << result4;
}

