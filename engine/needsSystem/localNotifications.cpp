/**
 * File: localNotifications
 *
 * Author: Paul Terry
 * Created: 08/02/2017
 *
 * Description: Local notifications for Cozmo's app
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/ankiEventUtil.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/localNotifications.h"
#include "engine/needsSystem/needsManager.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

static const std::string kLocalNotificationConfigsKey = "localNotificationConfigs";

static const float kAutoGeneratePeriod_s = 60.0f;


CONSOLE_VAR(bool, kDumpLocalNotificationsWhenGenerated, "Needs.LocalNotifications", false);

#if ANKI_DEV_CHEATS
namespace {
  LocalNotifications* g_DebugLocalNotifications = nullptr;
  void GenerateLocalNotificationsNow( ConsoleFunctionContextRef context )
  {
    if (g_DebugLocalNotifications != nullptr)
    {
      g_DebugLocalNotifications->Generate();
    }
  }
  CONSOLE_FUNC( GenerateLocalNotificationsNow, "Needs" );
};
#endif


LocalNotifications::LocalNotifications(const CozmoContext* context, NeedsManager& needsManager)
: _context(context)
, _needsManager(needsManager)
, _signalHandles()
, _timeForPeriodicGenerate_s(0.0f)
, _localNotificationConfig()
{
}

LocalNotifications::~LocalNotifications()
{
  _signalHandles.clear();

#if ANKI_DEV_CHEATS
  g_DebugLocalNotifications = nullptr;
#endif
}


void LocalNotifications::Init(const Json::Value& json, Util::RandomGenerator* rng,
                              const float currentTime_s)
{
  _rng = rng;

  _timeForPeriodicGenerate_s = currentTime_s + kAutoGeneratePeriod_s;

  _localNotificationConfig.items.clear();
  const auto& jsonLocalNotifications = json[kLocalNotificationConfigsKey];
  if (jsonLocalNotifications.isArray())
  {
    const s32 numEntries = jsonLocalNotifications.size();
    _localNotificationConfig.items.reserve(numEntries);

    for (int i = 0; i < numEntries; i++)
    {
      const Json::Value& jsonEntry = jsonLocalNotifications[i];
      LocalNotificationItem item;
      item.SetFromJSON(jsonEntry);
      _localNotificationConfig.items.emplace_back(item);
    }
  }
  
  auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);
  using namespace ExternalInterface;
  helper.SubscribeGameToEngine<MessageGameToEngineTag::NotificationsManagerReady>();

#if ANKI_DEV_CHEATS
  g_DebugLocalNotifications = this;
#endif
}

template<>
void LocalNotifications::HandleMessage(const ExternalInterface::NotificationsManagerReady& msg)
{
  // Generate all appropriate notifications on app start
  Generate();
}

void LocalNotifications::Update(const float currentTime_s)
{
  if (currentTime_s > _timeForPeriodicGenerate_s)
  {
    Generate();
  }
}


void LocalNotifications::SetPaused(const bool pausing)
{
  // Whether the needs system is being paused, or un-paused, re-generate
  // the local notifications
  Generate();
}


// Generate all local notifications appropriate for the current state, and send them
// to the Game.  The reason we can't do this on app backgrounding (the logical time)
// is because by that time the messages get sent to Game, Unity will have paused the
// entire app, preventing us from processing those messages.  So instead, we generate
// these periodically and at key times (such as when a daily star token is awarded),
// and send them to the Game, which saves the latest set.  Then when the app is back-
// grounded, it uses that last saved set for the actual local notifications that are
// registered with the OS.

void LocalNotifications::Generate()
{
  using namespace std::chrono;

  const auto& extInt = _context->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame
                    (ExternalInterface::ClearNotificationCache()));

  const auto& needsState = _needsManager.GetCurNeedsState();
  const auto& needsConfig = _needsManager.GetNeedsConfig();

  NeedsMultipliers multipliers;
  needsState.GetDecayMultipliers(needsConfig._decayUnconnected, multipliers);

  const Time now = system_clock::now();
  static const float secondsPerMinute = 60.0f;
  const float timeSinceAppOpen_m = (duration_cast<seconds>(now -
                                    needsState._timeLastAppUnBackgrounded).count()) /
                                    secondsPerMinute;

  for (const auto& config : _localNotificationConfig.items)
  {
    if (ShouldBeRegistered(config))
    {
      const float duration_m = DetermineTimeToNotify(config, multipliers,
                                                     timeSinceAppOpen_m, now);
      const int secondsInFuture = static_cast<int>(duration_m * secondsPerMinute);


      // Pick random string key among variants
      const int textKeyIndex = _rng->RandInt(static_cast<int>(config.textKeys.size()));
      const std::string textKey = config.textKeys[textKeyIndex];

      if (kDumpLocalNotificationsWhenGenerated)
      {
        const float duration_h = duration_m / secondsPerMinute;
        const float duration_d = duration_h / 24.0f;
        const Time futureDateTime = now + seconds(secondsInFuture);
        const std::time_t futureTimeT = system_clock::to_time_t(futureDateTime);
        static const size_t bufLen = 255;
        char whenStringBuf[bufLen];
        strftime(whenStringBuf, bufLen, "%a %F %T", localtime(&futureTimeT));
        PRINT_CH_INFO(NeedsManager::kLogChannelName,
                      "LocalNotifications.Generate",
                      "From now:%8.1f minutes (%6.1f hours, or%7.2f days); at %s; key: \"%s\"",
                      duration_m, duration_h, duration_d, whenStringBuf,
                      textKey.c_str());
      }

      const auto& extInt = _context->GetExternalInterface();
      extInt->Broadcast(ExternalInterface::MessageEngineToGame
                        (ExternalInterface::CacheNotificationToSchedule(secondsInFuture, textKey)));
    }
  }

  // Reset the periodic timer after any generate (periodic or not)
  _timeForPeriodicGenerate_s = _needsManager.GetCurrentTime() + kAutoGeneratePeriod_s;
}


bool LocalNotifications::ShouldBeRegistered(const LocalNotificationItem& config) const
{
  // Filter on connection
  const bool didConnect =_needsManager.GetConnectionOccurredThisAppRun();
  if (((config.connection == Connection::DidConnect)    && !didConnect) ||
      ((config.connection == Connection::DidNotConnect) &&  didConnect))
  {
    return false;
  }

  bool shouldBeRegistered = true;

  switch (config.notificationMainUnion.GetTag())
  {
    case NotificationUnionTag::notificationGeneral:
    {
      // Nothing to check
      break;
    }

    case NotificationUnionTag::notificationNeedLevel:
    {
      const auto& needLevelConfig = config.notificationMainUnion.Get_notificationNeedLevel();
      const NeedId needId = needLevelConfig.needId;
      const float threshold = needLevelConfig.level;
      const float curLevel = _needsManager.GetCurNeedsState().GetNeedLevel(needId);
      if (curLevel < threshold)
      {
        // The current need level is already below the threshold for this notification,
        // so it's too late to send
        shouldBeRegistered = false;
      }
      break;
    }

    case NotificationUnionTag::notificationNeedBracket:
    {
      const auto& needBracketConfig = config.notificationMainUnion.Get_notificationNeedBracket();
      const NeedId needId = needBracketConfig.needId;
      const NeedBracketId needBracketId = needBracketConfig.needBracketId;
      const NeedBracketId curNeedBracketId = _needsManager.GetCurNeedsStateMutable().GetNeedBracket(needId);
      if (curNeedBracketId >= needBracketId)
      {
        // The current need bracket is already at or below the bracket for this notification,
        // so it's too late to send
        shouldBeRegistered = false;
      }
      break;
    }

    case NotificationUnionTag::notificationDailyTokensToGo:
    {
      const NeedsState state = _needsManager.GetCurNeedsState();
      // Note that stars are now called tokens (externally, including in the data pipeline)
      const int starsToGo = state._numStarsForNextUnlock - state._numStarsAwarded;
      const auto& starsToGoConfig = config.notificationMainUnion.Get_notificationDailyTokensToGo();
      if (starsToGo != starsToGoConfig.numTokensToGo)
      {
        shouldBeRegistered = false;
      }
      break;
    }

    default:
      break;
  }

  return shouldBeRegistered;
}


float LocalNotifications::DetermineTimeToNotify(const LocalNotificationItem& config,
                                                const NeedsMultipliers& multipliers,
                                                const float timeSinceAppOpen_m,
                                                const Time now) const
{
  float minutesInFuture = 0.0f;

  switch (config.notificationMainUnion.GetTag())
  {
    case NotificationUnionTag::notificationGeneral:
    {
      minutesInFuture = CalculateMinutesInFuture(config, timeSinceAppOpen_m, now);
      break;
    }

    case NotificationUnionTag::notificationNeedLevel:
    {
      const auto& needLevelConfig = config.notificationMainUnion.Get_notificationNeedLevel();
      const NeedId needId = needLevelConfig.needId;
      const float threshold = needLevelConfig.level;

      // How long will it take to decay to the given level?
      minutesInFuture = CalculateMinutesToThreshold(needId, threshold, multipliers);
      break;
    }

    case NotificationUnionTag::notificationNeedBracket:
    {
      const auto& needBracketConfig = config.notificationMainUnion.Get_notificationNeedBracket();
      const NeedId needId = needBracketConfig.needId;
      const NeedBracketId bracketId = needBracketConfig.needBracketId;

      // How long will it take to decay to the TOP of the given bracket?
      // (e.g. when Cozmo's Energy level BECOMES 'critical')
      const int prevBracketIndex = static_cast<int>(bracketId) - 1;
      if (prevBracketIndex >= 0)
      {
        const NeedBracketId prevBracketId = static_cast<NeedBracketId>(prevBracketIndex);
        const auto& needsConfig = _needsManager.GetNeedsConfig();
        const float threshold = needsConfig.NeedLevelForNeedBracket(needId, prevBracketId);

        minutesInFuture = CalculateMinutesToThreshold(needId, threshold, multipliers);
      }
      else
      {
        PRINT_CH_INFO(NeedsManager::kLogChannelName,
                      "LocalNotifications.DetermineTimeToNotify",
                      "Need Bracket notification specifies 'full' which doesn't make sense");
      }
      break;
    }

    case NotificationUnionTag::notificationDailyTokensToGo:
    {
      minutesInFuture = CalculateMinutesInFuture(config, timeSinceAppOpen_m, now);
      break;
    }

    default:
      break;
  }
  
  // Allow for a random uniform distribution if desired
  minutesInFuture += static_cast<float>(_rng->RandDblInRange(-config.rangeEarly, config.rangeLate));

  // Enforce minimum duration
  if (minutesInFuture < config.minimumDuration)
  {
    minutesInFuture = config.minimumDuration;
  }

  // Enforce maximum duration
  const float maxMinutes = _needsManager.GetNeedsConfig()._localNotificationMaxFutureMinutes;
  if (minutesInFuture > maxMinutes)
  {
    PRINT_CH_INFO(NeedsManager::kLogChannelName,
                  "LocalNotifications.DetermineTimeToNotify",
                  "Clamping local notification time at the configured maximum of %.1f minutes",
                  maxMinutes);
    minutesInFuture = maxMinutes;
  }

  return minutesInFuture;
}


float LocalNotifications::CalculateMinutesInFuture(const LocalNotificationItem& config,
                                                   const float timeSinceAppOpen_m,
                                                   const Time now) const
{
  float minutesInFuture = 0.0f;

  switch (config.whenType)
  {
    case WhenType::NotApplicable:
    {
      DEV_ASSERT_MSG(config.whenType != WhenType::NotApplicable,
                     "LocalNotifications.CalculateMinutesInFuture",
                     "Notification config has 'when type' of NotApplicable but wants an applicable when type");
      break;
    }

    case WhenType::AfterAppOpen:
    {
      minutesInFuture = config.whenParam - timeSinceAppOpen_m;
      break;
    }

    case WhenType::AfterAppClose:
    {
      minutesInFuture = config.whenParam;
      break;
    }

    case WhenType::ClockTime:
    {
      // First, create a Time for today, at the config's clock time:
      using namespace std::chrono;
      const std::time_t nowTimeT = system_clock::to_time_t(now);
      std::tm targetLocalTime;
      localtime_r(&nowTimeT, &targetLocalTime);
      const int clockTimeHours = static_cast<int>(config.whenParam / 60.0f);
      const int clockTimeMinutes = static_cast<int>(fmodf(config.whenParam, 60.0f));
      targetLocalTime.tm_hour = clockTimeHours;
      targetLocalTime.tm_min = clockTimeMinutes;
      std::time_t targetTimeT = mktime(&targetLocalTime);

      // If we've already passed this time of day, make it the NEXT day
      if (targetTimeT <= nowTimeT)
      {
        targetTimeT += (24 * 60 * 60);
      }
      minutesInFuture = (targetTimeT - nowTimeT) / 60.0f;

      // Finally, enforce the 'minimum duration' by bumping ahead a day if needed
      if (minutesInFuture < config.minimumDuration)
      {
        minutesInFuture += (24 * 60);
      }
      break;
    }
  }

  return minutesInFuture;
}


float LocalNotifications::CalculateMinutesToThreshold(const NeedId needId, const float targetLevel,
                                                      const NeedsMultipliers& multipliers) const
{
  // Calculate how long it will take (in minutes) for curLevel to decay
  // (with unconnected decay rates) to the target level
  const auto& needsState = _needsManager.GetCurNeedsState();
  const auto& needsConfig = _needsManager.GetNeedsConfig();

  const float minutesInFuture = needsState.TimeForDecayToLevel(needsConfig._decayUnconnected,
                                                               static_cast<int>(needId),
                                                               targetLevel,
                                                               multipliers);

  if (minutesInFuture == std::numeric_limits<float>::max())
  {
    // If we calculated "infinite" (which could be the case when a
    // decay bracket has a rate of zero), report this
    PRINT_CH_INFO(NeedsManager::kLogChannelName,
                  "LocalNotifications.CalculateMinutesToThreshold",
                  "Notification set to infinite future");
  }

  return minutesInFuture;
}



} // namespace Cozmo
} // namespace Anki

