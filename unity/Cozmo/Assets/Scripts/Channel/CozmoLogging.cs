using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;

public static class CozmoLogging
{

  private static object sync = new object();
  private static FileStream logFile = null;
  private static TextWriter logWriter = null;
  private static bool flushLogs = false;

  public static void OpenLogFile()
  {
    lock(sync)
    {
      AppDomain.CurrentDomain.DomainUnload -= OnExit;
      AppDomain.CurrentDomain.DomainUnload += OnExit;

      OptionsScreen.RefreshSettings -= UpdateFlushLogsValue;
      OptionsScreen.RefreshSettings += UpdateFlushLogsValue;
      
      UpdateFlushLogsValue();
      CloseLogFile();
      
      try
      {
        string logFilename = "CozmoLog_" + DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss") + ".txt";
        logFile = File.Open(Path.Combine(Application.persistentDataPath, logFilename), FileMode.Append, FileAccess.Write, FileShare.Read);
        logWriter = new StreamWriter(logFile, Encoding.UTF8);
        
        Application.logMessageReceivedThreaded -= LogFromUnity;
        Application.logMessageReceivedThreaded += LogFromUnity;

        CozmoBinding.SetLogCallback(LogFromCpp, 0);
      } catch(Exception e)
      {
        CloseLogFile();
        Debug.LogException(e); 
      }
    }
  }
  
  // there doesn't seem to be a good time to call this,
  // so we'll just rely on destructors to close the files
  public static void CloseLogFile()
  {
    lock(sync)
    {
      // Don't do this--might be on another thread.
      // Instead, just make sure they do nothing if the stream is closed.
      //AppDomain.CurrentDomain.DomainUnload -= OnExit;
      //OptionsScreen.RefreshSettings -= UpdateFlushLogsValue;
      //Application.logMessageReceivedThreaded -= LogToFile;

      ResetLogCallback();

      if(logWriter != null)
      {
        TextWriter writer = logWriter;
        logWriter = null;
        // StreamWriter owns the stream, so null it out here.
        logFile = null;
        try
        {
          writer.Flush();
          writer.Close();
        } catch(Exception e)
        {
          Debug.LogException(e);
        }
      }
      
      if(logFile != null)
      {
        FileStream stream = logFile;
        logFile = null;
        try
        {
          stream.Flush();
          stream.Close();
        } catch(Exception e)
        {
          Debug.LogException(e);
        }
      }
    }
  }

  private static void ResetLogCallback()
  {
    try
    {
      CozmoBinding.SetLogCallback(null, 0);
    } catch(Exception e)
    {
      Debug.LogException(e);
    }
  }

  private static void LogFromUnity(string condition, string stackTrace, LogType logType)
  {
    lock(sync)
    {
      if(logWriter != null)
      {
        try
        {
          logWriter.WriteLine(condition);
          if(!string.IsNullOrEmpty(stackTrace))
          {
            logWriter.WriteLine(stackTrace);
          }
          
          // NOTE: Will take time to flush to file. Can be disabled for less-rigorous logging.
          if(flushLogs)
          {
            logWriter.Flush();
          }
        } catch(Exception e)
        {
          CloseLogFile();
          Debug.LogException(e);
        }
      }
    }
  }

  [MonoPInvokeCallback(typeof(CozmoBinding.LogCallback))]
  private static void LogFromCpp(int logLevel, string message)
  {
    lock(sync)
    {
      if(logWriter != null)
      {
        try
        {
          logWriter.WriteLine(message);
          
          // NOTE: Will take time to flush to file. Can be disabled for less-rigorous logging.
          if(flushLogs)
          {
            logWriter.Flush();
          }
        } catch(Exception e)
        {
          CloseLogFile();
          Debug.LogException(e);
        }
      } else
      {
        if(logLevel < 4)
        {
          Debug.Log(message);
        } else if(logLevel < 5)
        {
          Debug.LogWarning(message);
        } else
        {
          Debug.LogError(message);
        }
      }
    }
  }

  private static void UpdateFlushLogsValue()
  {
    lock(sync)
    {
      flushLogs = PlayerPrefs.GetInt("FlushLogs", 1) != 0;
    }
  }

  /// <summary>
  /// Attempts to clean up unmanaged memory if at all possible.
  /// </summary>
  private static void OnExit(object unused1, EventArgs unused2)
  {
    lock(sync)
    {
      // Rest will be picked up by finalizers.
      ResetLogCallback();
    }
  }
}

