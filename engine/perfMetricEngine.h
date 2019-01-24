/**
 * File: perfMetricEngine
 *
 * Author: Paul Terry
 * Created: 11/16/2018
 *
 * Description: Lightweight performance metric recording: for vic-engine
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Vector_Engine_PerfMetric_Engine_H__
#define __Vector_Engine_PerfMetric_Engine_H__

#include "util/perfMetric/iPerfMetric.h"
#include "clad/types/behaviorComponent/activeFeatures.h"

#include <string>


namespace Anki {
namespace Vector {


class PerfMetricEngine : public PerfMetric
{
public:
  explicit PerfMetricEngine(const CozmoContext* context);
  virtual ~PerfMetricEngine();

  virtual void Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService) override final;
  virtual void Update(const float tickDuration_ms,
                      const float tickFrequency_ms,
                      const float sleepDurationIntended_ms,
                      const float sleepDurationActual_ms) override final;
  virtual void Dump(const DumpType dumpType, const bool dumpAll,
                    const std::string* fileName = nullptr, std::string* resultStr = nullptr) const override final;

private:

  void DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                   FILE* fd, std::string* resultStr) const;

  // Frame size:  Base struct is 16 bytes; here is 10 words * 4 (40 bytes), plus 32 bytes = 88 bytes
  // x 4000 frames is ~344 KB
  struct FrameMetricEngine : public FrameMetric
  {
    uint32_t _messageCountRobotToEngine;
    uint32_t _messageCountEngineToRobot;
    uint32_t _messageCountGameToEngine;
    uint32_t _messageCountEngineToGame;
    uint32_t _messageCountViz;
    uint32_t _messageCountGatewayToEngine;
    uint32_t _messageCountEngineToGateway;

    float _batteryVoltage;
    uint32_t _cpuFreq_kHz;

    ActiveFeature    _activeFeature;
    static const int kBehaviorStringMaxSize = 32;
    char _behavior[kBehaviorStringMaxSize]; // Some description of what Victor is doing
  };

  FrameMetricEngine*  _frameBuffer = nullptr;
#if ANKI_PERF_METRIC_ENABLED
  const CozmoContext* _context;
#endif
};


} // namespace Vector
} // namespace Anki


#endif // __Vector_Engine_PerfMetric_Engine_H__
