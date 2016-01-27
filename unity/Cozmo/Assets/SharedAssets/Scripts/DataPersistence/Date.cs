using System;
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
        var validDate = new Date(year, 1, 1).AddMonths(month).AddDays(day);
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

    public Date AddYears(int years) {
      return new Date(Year + years, Month, Day);
    }
      
    public Date AddMonths(int months) {
      int newYear = Year;
      int newMonth = Month;

      while(months > 0) {
        int overflow = months + newMonth - 12;
        if (overflow > 0) {
          months = overflow;
          newYear++;
          newMonth = 1;
        }
        else {
          newMonth += months;
          months = 0;
        }
      }
      while (months < 0) {
        int undeflow = months + newMonth - 1;
        if (undeflow < 0) {
          newYear--;
          months = undeflow;
          newMonth = 12;
        }
        else {
          newMonth -= months;
          months = 0;
        }
      }

      int newDay = Math.Min(Day, DaysInTheMonth((Month)newMonth, newYear));
      return new Date(newYear, newMonth, newDay);
    }

    public Date AddDays(int days) {
      int newDay = Day;
      int newMonth = Month;
      int newYear = Year;

      while (days > 0) {
        int daysInMonth = DaysInTheMonth((DataPersistence.Month)newMonth, newYear);
        int overflow = days + newDay - daysInMonth;
        if (overflow > 0) {
          days = overflow;
          newDay = 1;

          newMonth++;
          if (newMonth > 12) {
            newMonth = 1;
            newYear++;
          }
        }
        else {
          newDay += days;
          days = 0;
        }
      }
      while(days < 0) {
        int underflow = days + newDay - 1;
        if (underflow < 0) {
          newMonth--;
          if (newMonth < 1) {
            newMonth = 12;
            newYear--;
          }
          int daysInMonth = DaysInTheMonth((DataPersistence.Month)newMonth, newYear);
          days = underflow;
          newDay = daysInMonth;
        }
        else {
          newDay -= days;
          days = 0;
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

    public static bool operator== (Date a, Date b) {
      return a.Year == b.Year && a.Month == b.Month && a.Day == b.Day;
    }

    public static bool operator!= (Date a, Date b) {
      return a.Year != b.Year || a.Month != b.Month || a.Day != b.Day;
    }

    public static bool operator< (Date a, Date b) {
      if (a.Year == b.Year) {
        if (a.Month == b.Month) {
          return a.Day < b.Day;
        }
        return a.Month < b.Month;
      }
      return a.Year < b.Year;
    }

    public static bool operator> (Date a, Date b) {
      if (a.Year == b.Year) {
        if (a.Month == b.Month) {
          return a.Day > b.Day;
        }
        return a.Month > b.Month;
      }
      return a.Year > b.Year;
    }

    public override string ToString() {
      return Month + "/" + Day + "/" + Year;
    }
  }
}

