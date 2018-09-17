/**
 * File: engine/components/robotHealthReporter.h
 *
 * Description: Report robot & OS health events to DAS
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __engine_components_robotHealthReporter_h
#define __engine_components_robotHealthReporter_h

#include "engine/robotComponents_fwd.h"
#include "osState/osState.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/dispatchQueue/taskExecutor.h"

#include <chrono>
#include <map>
#include <string>

namespace Anki {
namespace Vector {

class RobotHealthReporter : public IDependencyManagedComponent<RobotComponentID>
{
  public:
    using DependencyManager = DependencyManagedEntity<RobotComponentID>;

    RobotHealthReporter();

    // IDependencyManagedComponent
    virtual void InitDependent(Robot* robot, const DependencyManager& dependencyManager) override;
    virtual void UpdateDependent(const DependencyManager& dependencyManager) override;

    // Event notification
    void OnSetLocale(const std::string & locale);
    void OnSetTimeZone(const std::string & timezone);

  private:

    // Reporting state
    bool _once_per_boot = true;
    bool _once_per_startup = true;
    std::chrono::steady_clock::time_point _once_per_minute_time;
    std::chrono::steady_clock::time_point _once_per_second_time;

    // Stuff we keep track of
    OSState::Alert _memoryAlert = OSState::Alert::None;
    OSState::WifiInfo _wifiInfo;
    std::map<std::string, OSState::DiskInfo> _diskInfo;
    std::string _locale;
    std::string _timezone;
    uint32_t _cpuFreq_kHz = 0;

    // Reporting tasks
    void BootInfoCheck(const OSState* osState);
    void WifiInfoCheck(const OSState * osState);
    void DiskInfoCheck(const OSState * osState);
    void MemoryInfoCheck(const OSState * osState);
    void CPUInfoCheck(const OSState * osState);

    // Interval tasks
    void OncePerBootCheck();
    void OncePerStartupCheck();
    void OncePerMinuteCheck();
    void OncePerSecondCheck();

};

}
}
#endif
