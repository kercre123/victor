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
  virtual void Dump(const DumpType dumpType, const bool dumpAll,
                    const std::string* fileName = nullptr, std::string* resultStr = nullptr) const override final;

private:

  void DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                   FILE* fd, std::string* resultStr) const;

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
};


} // namespace Vector
} // namespace Anki


#endif // __Vector_PerfMetric_Anim_H__
