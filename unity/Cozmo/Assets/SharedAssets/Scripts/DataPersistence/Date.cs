using System;
using UnityEngine;
using Newtonsoft.Json;

namespace DataPersistence {

  public enum Month {
    January = 1,
    February = 2,
    March = 3,
    April = 4,
    May = 5,
    June = 6,
    July = 7,
    August = 8,
    September = 9,
    October = 10,
    November = 11,
    December = 12
  }

  [JsonConverter(typeof(DateConverter))]
  public struct Date {
    public readonly int Year;
    public readonly int Month;
    public readonly int Day;

    public Month GetMonth() {
      return (Month)Month;
    }

    public Date(int year, int month, int day) {
      if (month < 1 || month > 12 || day < 1 || day > DaysInTheMonth((Month)month, year)) {
        var validDate = new Date(year, 1, 1).OffsetMonth(month - 1).OffsetDays(day - 1);
        Year = validDate.Year;
        Month = validDate.Month;
        Day = validDate.Day;
      }
      else {
        Year = year;
        Month = month;
        Day = day;
      }
    }

    public Date OffsetYear(int years) {
      return new Date(Year + years, Month, Day);
    }

    /// <summary>
    /// Returns a date X months from the current date. If the current day is the end of the month
    /// and you move to a month with fewer days, day is set to the final day of the new month.
    /// </summary>
    /// <returns>New Date.</returns>
    /// <param name="monthsToOffset">Months to offset.</param>
    public Date OffsetMonth(int monthsToOffset) {
      int newYear = Year;
      int newMonth = Month;

      if (monthsToOffset > 0) {
        return AddMonthsInternal(monthsToOffset);
      }
      else if (monthsToOffset < 0) {
        return SubtractMonthsInternal(monthsToOffset);
      }
      return new Date(newYear, newMonth, Day);
    }

    // Returns a date X months from the future
    private Date AddMonthsInternal(int monthsToAdd) {
      int newYear = Year;
      int newMonth = Month;
      if (monthsToAdd <= 0) { Debug.LogWarning("Cannot add a negative number of months"); }

      while (monthsToAdd > 0) {
        // Overflow represents the number of months into the next year the new date is
        int overflow = monthsToAdd + newMonth - 12;
        if (overflow > 0) {
          monthsToAdd = overflow;
          newYear++;
          newMonth = 1;
        }
        else {
          newMonth += monthsToAdd;
          monthsToAdd = 0;
        }
      }

      int newDay = Math.Min(Day, DaysInTheMonth((Month)newMonth, newYear));
      return new Date(newYear, newMonth, newDay);
    }

    // Returns a date X months into the past
    private Date SubtractMonthsInternal(int monthsToSubtract) {
      int newYear = Year;
      int newMonth = Month;
      if (monthsToSubtract >= 0) { Debug.LogWarning("Cannot subtract a positive number of months"); }

      while (monthsToSubtract < 0) {
        // Underflow represents the number of months into the previous year the new date is
        int undeflow = monthsToSubtract + newMonth - 1;
        if (undeflow < 0) {
          newYear--;
          monthsToSubtract = undeflow;
          newMonth = 12;
        }
        else {
          newMonth += monthsToSubtract;
          monthsToSubtract = 0;
        }
      }

      int newDay = Math.Min(Day, DaysInTheMonth((Month)newMonth, newYear));
      return new Date(newYear, newMonth, newDay);
    }

    /// <summary>
    /// Returns a date X days from the current date.
    /// </summary>
    /// <returns>New Date.</returns>
    /// <param name="daysToOffset">Days to offset.</param>
    public Date OffsetDays(int daysToOffset) {
      int newDay = Day;
      int newMonth = Month;
      int newYear = Year;
      if (daysToOffset > 0) {
        return AddDaysInternal(daysToOffset);
      }
      else if (daysToOffset < 0) {
        return SubtractDaysInternal(daysToOffset);
      }
      return new Date(newYear, newMonth, newDay);
    }

    // Returns date X days into the future
    private Date AddDaysInternal(int daysToAdd) {
      int newDay = Day;
      int newMonth = Month;
      int newYear = Year;
      if (daysToAdd <= 0) { Debug.LogWarning("Cannot add a negative number of days"); }

      while (daysToAdd > 0) {
        // Overflow represents the number of days into the next month the new date is
        int daysInMonth = DaysInTheMonth((DataPersistence.Month)newMonth, newYear);
        int overflow = daysToAdd + newDay - daysInMonth;
        if (overflow > 0) {
          daysToAdd = overflow;
          newDay = 0;

          newMonth++;
          if (newMonth > 12) {
            newMonth = 1;
            newYear++;
          }
        }
        else {
          newDay += daysToAdd;
          daysToAdd = 0;
        }
      }

      return new Date(newYear, newMonth, newDay);
    }

    // Returns date X days into the past
    private Date SubtractDaysInternal(int daysToSubtract) {
      int newDay = Day;
      int newMonth = Month;
      int newYear = Year;
      if (daysToSubtract >= 0) { Debug.LogWarning("Cannot subtract a positive number of days"); }

      while (daysToSubtract < 0) {
        // Underflow represents the number of days into the previous month the new date is
        int underflow = daysToSubtract + newDay - 1;
        if (underflow < 0) {
          newMonth--;
          if (newMonth < 1) {
            newMonth = 12;
            newYear--;
          }
          int daysInMonth = DaysInTheMonth((DataPersistence.Month)newMonth, newYear) + 1;
          daysToSubtract = underflow;
          newDay = daysInMonth;
        }
        else {
          newDay += daysToSubtract;
          daysToSubtract = 0;
        }
      }

      return new Date(newYear, newMonth, newDay);
    }

    public static int DaysInTheMonth(Month month, int year) {
      switch (month) {
      case DataPersistence.Month.September:
      case DataPersistence.Month.April:
      case DataPersistence.Month.June:
      case DataPersistence.Month.November:
        return 30;
      case DataPersistence.Month.February:
        return IsLeapYear(year) ? 29 : 28;
      default:
        return 31;
      }
    }


    public static bool IsLeapYear(int year) {
      if (year % 4 == 0) {
        if (year % 100 == 0) {
          if (year % 400 == 0) {
            return true;
          }
          return false;
        }
        return true;
      }
      return false;
    }

    public static implicit operator Date(DateTime dateTime) {
      return new Date(dateTime.Year, dateTime.Month, dateTime.Day);
    }

    public DateTime ToLocalDateTime() {
      return new DateTime(Year, Month, Day, 0, 0, 0, DateTimeKind.Local);
    }

    public DateTime ToUtcDateTime() {
      return new DateTime(Year, Month, Day, 0, 0, 0, DateTimeKind.Utc);
    }

    public override bool Equals(object obj) {
      if (obj is Date) {
        return this == (Date)obj;
      }
      return false;
    }

    public override int GetHashCode() {
      // assuming date is valid, this will be good for most of time.
      return Year << 9 | Month << 5 | Day;
    }

    public static System.TimeSpan operator -(Date a, Date b) {
      return new System.DateTime(a.Year, a.Month, a.Day) - new System.DateTime(b.Year, b.Month, b.Day);
    }

    public static bool operator ==(Date a, Date b) {
      return a.Year == b.Year && a.Month == b.Month && a.Day == b.Day;
    }

    public static bool operator !=(Date a, Date b) {
      return a.Year != b.Year || a.Month != b.Month || a.Day != b.Day;
    }

    public static bool operator <(Date a, Date b) {
      if (a.Year == b.Year) {
        if (a.Month == b.Month) {
          return a.Day < b.Day;
        }
        return a.Month < b.Month;
      }
      return a.Year < b.Year;
    }

    public static bool operator >(Date a, Date b) {
      if (a.Year == b.Year) {
        if (a.Month == b.Month) {
          return a.Day > b.Day;
        }
        return a.Month > b.Month;
      }
      return a.Year > b.Year;
    }

    public override string ToString() {
      return string.Format("{0:D4}-{1:D2}-{2:D2}", Year, Month, Day);
    }
  }
}

