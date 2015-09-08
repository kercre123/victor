using System;
using System.Windows.Forms;

namespace UnityEngine
{
    public class Object : System.Object
    {

    }

    public static class Debug
    {
        public static void Log(string message)
        {
            Console.WriteLine(message);
        }

        public static void LogWarning(string message)
        {
            Console.WriteLine("[Warning] " + message);
        }

        public static void LogError(string message)
        {
            MessageBox.Show(message);
        }

        public static void LogException(Exception exception)
        {
            MessageBox.Show(exception.ToString());
        }
    }

    public static class Time
    {
        public static float realtimeSinceStartup
        {
            get
            {
                return Environment.TickCount / 1000.0f;
            }
        }
    }
}
