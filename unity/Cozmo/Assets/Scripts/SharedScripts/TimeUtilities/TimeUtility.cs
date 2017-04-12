using System;

public static class TimeUtility {

  public const int SECONDS_PER_MINUTE = 60;
  public const int MINUTES_PER_HOUR = 60;

  public static int HoursFromSeconds(int seconds, out int remainder) {
    int secondsPerHour = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;
    int hours = seconds / secondsPerHour;
    remainder = seconds % secondsPerHour;
    return hours;
  }

  public static int MinutesFromSeconds(int seconds, out int remainder) {
    int minutes = seconds / SECONDS_PER_MINUTE;
    remainder = seconds % SECONDS_PER_MINUTE;
    return minutes;
  }

  public static TimeSpan TimeSpanFromSeconds(int seconds) {
    int remainder = 0;
    int secondsLeftover = 0;
    int hours = HoursFromSeconds(seconds, out remainder);
    int minutes = MinutesFromSeconds(remainder, out secondsLeftover);
    return new TimeSpan(hours, minutes, secondsLeftover);
  }

  private static readonly DateTime _Epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

  public static long ToUtcMs(this DateTime date) {
    return (long)(date - _Epoch).TotalMilliseconds;
  }

  public static DateTime FromUtcMs(this long utcMs) {
    const long ticksPerMs = 10000;
    return _Epoch + new TimeSpan(utcMs * ticksPerMs);
  }

  public static long ToUtc(this DateTime date) {
    return (long)(date - _Epoch).TotalSeconds;
  }

  public static DateTime FromUtc(this long utc) {
    const long ticksPerSecond = 10000 * 1000;
    return _Epoch + new TimeSpan(utc * ticksPerSecond);
  }
}
