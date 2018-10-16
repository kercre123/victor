/**
 * File: testing.h
 *
 * Author: bneuman (brad)
 * Created: 9/25/2012
 *
 * Description: This file contains macros and an interface for writing
 *              C++ test cases that will be wrapped by OCunit in
 *              iOS. All C++ unit tests should inheret this.
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifndef BASESTATION_TESTING_H_
#define BASESTATION_TESTING_H_

#include "util/helpers/includeSstream.h"
#include <exception>

#include <boost/property_tree/ptree_fwd.hpp>


using namespace boost::property_tree;

// Assert macros (bt stands for basestation testing)
#define BT_ASSERT(cond, s) {if(!(cond)){ std::stringstream __note; __note<<s; throw(BasestationTestingAssert( __FILE__, __LINE__, __note.str()));}}
#define BT_ASSERT_EQ(lh, rh, s) {if((lh)!=(rh)){ std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") != ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_NE(lh, rh, s) {if((lh)==(rh)){ std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") == ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_FLT_NEAR(lh, rh, s) {if(!FLT_NEAR((lh),(rh))){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") != ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_FLT_NE(lh, rh, s) {if(FLT_NEAR((lh),(rh))){ std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") == ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_FLT_WITHIN(lh, rh, eps, s) {if(!NEAR((lh),(rh),(eps))){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") != ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_FLT_LE(lh, rh, s) {if((lh)>(rh)+FLOATING_POINT_COMPARISON_TOLERANCE){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !<= ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_FLT_GE(lh, rh, s) {if((lh)+FLOATING_POINT_COMPARISON_TOLERANCE<(rh)){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !>= ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_LT(lh, rh, s) {if((lh)>=(rh)){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !< ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_GT(lh, rh, s) {if((lh)<=(rh)){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !> ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_LE(lh, rh, s) {if((lh)>(rh)){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !<= ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}
#define BT_ASSERT_GE(lh, rh, s) {if((lh)<(rh)){std::stringstream __ss; __ss<<s<<". ("<<(#lh)<<": "<<(lh)<<") !>= ("<<(#rh)<<": "<<(rh)<<")"; throw(BasestationTestingAssert( __FILE__, __LINE__, __ss.str()));}}

// Stupid hack to make it easier to run all tests.
#define BT_LIST_TESTS(c) bool c::RunAllTests() { std::stringstream res; bool ret = false; res<<"\nTest package: "<<#c<<"\n";
#define BT_END_LIST std::cout.flush(); std::cout<<res.str(); std::cout.flush(); return ret; }
#define BT_RUN_TEST(t) {                                                \
    SetUp();                                                            \
    sUnSetErrG()                                                        \
    try {                                                               \
      (t)();                                                            \
      if(sGetErrG())                                                    \
        throw(BasestationTestingAssert( __FILE__, -1, "Internal error")); \
      res<<"| "<<#t<<": PASS\n";                                        \
    }                                                                   \
    catch(BasestationTestingAssert e){                                  \
      ret = true;                                                       \
      res<<"| "<<#t<<": "<<e.what()<<std::endl;                         \
        res<<"error: "<<#t<<": "<<e.what()<<std::endl;                  \
    };                                                                  \
    TearDown();                                                         \
  }

// This class should only be created using the BT_ASSERT macros above
class BasestationTestingAssert //: public std::exception
{
public:
  BasestationTestingAssert(const std::string& filename, const int linenum, const std::string& note) {
    std::stringstream ss;
    //ss<<filename<<':'<<linenum<<": "<<note;
    ss<<"FAIL: "<<note<<" (line "<<linenum<<')';
    //<<" (from "<<filename<<':'<<linenum<<')';
    s_ = ss.str();
  };
  BasestationTestingAssert(const BasestationTestingAssert& other) : s_(other.s_) {};

  const char* what() { return s_.c_str(); };
private:
  std::string s_;
};

typedef void (*voidFn)(void);

// This is the class that should be inhereted by C++ test cases
class BasestationTest
{
public:
  BasestationTest() : config_(NULL) {};

  // Runs all tests. This is crappy, but will work for now. The
  // function shouldn't be directly implemented. Instead,
  // implementation should Look like (in .cpp file):
  //
  // BT_LIST_TESTS(TestPackage)
  // BT_RUN_TEST(TestCase1)
  // BT_RUN_TEST(TestCase2)
  // ...
  // BT_END_LIST
  //
  // See btTestPlanner.cpp for an example.
  // returns true if any test failed, false otherwise
  virtual bool RunAllTests() = 0;

  // Optional function to set the ptree config for these tests
  void SetConfig(ptree* config) {config_ = config;};

  // Set-up function is called before each test
  virtual void SetUp() {};

  // Tear-down function is called after each test
  virtual void TearDown() {};

protected:
  ptree* config_;
};


#endif
