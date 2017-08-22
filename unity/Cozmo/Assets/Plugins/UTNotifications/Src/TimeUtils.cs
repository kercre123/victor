using System;
using UnityEngine;

namespace UTNotifications
{
    public sealed class TimeUtils
    {
        public static int ToSecondsFromNow(DateTime dateTime)
        {
            return (int)(dateTime - DateTime.Now).TotalSeconds;
        }

        public static int MinutesToSeconds(int minutes)
        {
            return minutes * 60;
        }

        public static int HoursToSeconds(int hours)
        {
            return hours * 3600;
        }

        public static int DaysToSeconds(int days)
        {
            return days * 86400;
        }

        public static int WeeksToSeconds(int weeks)
        {
            return weeks * 604800;
        }
    }
}