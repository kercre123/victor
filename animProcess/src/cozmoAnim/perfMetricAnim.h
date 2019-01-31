/**
 * File: perfMetricAnim
 *
 * Author: Paul Terry
 * Created: 11/28/2018
 *
 * Description: Lightweight performance metric recording: for vic-anim
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Vector_PerfMetric_Anim_H__
#define __Vector_PerfMetric_Anim_H__

#include "util/perfMetric/iPerfMetric.h"
#include "util/stats/statsAccumulator.h"

#include <string>


namespace Anki {
namespace Vector {


class PerfMetricAnim : public PerfMetric
{
public:

  explicit PerfMetricAnim(const AnimContext* context);
  virtual ~PerfMetricAnim();

  virtual void Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService) override final;
  virtual void Update(const float tickDuration_ms,
                      const float tickFrequency_ms,
                      const float sleepDurationIntended_ms,
                      const float sleepDurationActual_ms) override final;

private:

  virtual void InitDumpAccumulators() override final;
  virtual const FrameMetric& UpdateDumpAccumulators(const int frameBufferIndex) override final;
  virtual int AppendFrameData(const DumpType dumpType,
                              const int frameBufferIndex,
                              const int dumpBufferOffset) override final;
  virtual int AppendSummaryData(const DumpType dumpType,
                                const int dumpBufferOffset,
                                const int lineIndex) override final;

  // Frame size:  Base struct is 16 bytes; here is 4 words * 4 (16 bytes) = 32 bytes
  // x 4000 frames is 125 KB
  struct FrameMetricAnim : public FrameMetric
  {
    uint32_t _messageCountAnimToRobot;
    uint32_t _messageCountAnimToEngine;
    uint32_t _messageCountRobotToAnim;
    uint32_t _messageCountEngineToAnim;
  };

  FrameMetricAnim*  _frameBuffer = nullptr;
//  const AnimContext* _context;
  Util::Stats::StatsAccumulator _accMessageCountRtA;
  Util::Stats::StatsAccumulator _accMessageCountAtR;
  Util::Stats::StatsAccumulator _accMessageCountEtA;
  Util::Stats::StatsAccumulator _accMessageCountAtE;
};

} // namespace Vector
} // namespace Anki


#endif // __Vector_PerfMetric_Anim_H__
