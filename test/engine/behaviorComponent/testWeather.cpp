/**
 * File: testWeather.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-09-27
 *
 * Description: unit tests for weather utility
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"

using namespace Anki;
using namespace Anki::Vector;

TEST(Weather, ConvertCToF)
{
  EXPECT_EQ(-40.0f, WeatherIntentParser::ConvertTempCToF(-40));
  EXPECT_EQ(-4.0f, WeatherIntentParser::ConvertTempCToF(-20));
  EXPECT_EQ(32.0f, WeatherIntentParser::ConvertTempCToF(0));
  EXPECT_EQ(212.0f, WeatherIntentParser::ConvertTempCToF(100));
}

