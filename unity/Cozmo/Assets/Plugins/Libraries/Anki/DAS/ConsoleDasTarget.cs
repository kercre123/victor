using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class ConsoleDasTarget : IDASTarget {

  public void Event(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine("[Event][" + eventName + "] " + eventValue);
  }

  public void Error(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine("[Error][" + eventName + "] " + eventValue);
  }

  public void Warn(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine("[Warn][" + eventName + "] " + eventValue);
  }

  public void Info(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine("[Info][" + eventName + "] " + eventValue);
  }

  public void Debug(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine("[Debug][" + eventName + "] " + eventValue);
  }

  public void Ch_Info(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine ("[Ch_Info][" + channelName +"][" + eventName + "] " + eventValue);
  }

  public void Ch_Debug(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    Console.WriteLine ("[Ch_Debug][" + channelName + "][" + eventName + "] " + eventValue);
  }

  public void SetGlobal(string key, string value) {
    Console.WriteLine("[SetGlobal][" + key + "] " + value);
  }
}
