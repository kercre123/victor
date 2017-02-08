using UnityEngine;
using System.Collections;

// Public singleton just to dump shared data.
// We can store data from the engine we got from CLAD
// as well as unity data if needed.
using Anki.Cozmo;
using System;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;
using System.Reflection;

namespace Anki.Debug {
  public class DebugConsoleData : IDisposable {

    private static DebugConsoleData _Instance;

    public static DebugConsoleData Instance {
      get {
        if (_Instance == null) {
          _Instance = new DebugConsoleData();
        }
        return _Instance;
      }
    }

    public delegate void DebugConsoleVarEventHandler(string str);

    public class DebugConsoleVarData : IComparable {

      public string VarName;
      public string Category;
      public ConsoleVarUnion.Tag TagType;

      public double MinValue;
      public double MaxValue;

      // most integral types can be expressed as one of the following
      public double ValueAsDouble;
      public long ValueAsInt64;
      public ulong ValueAsUInt64;
      // C# setter function
      public DebugConsoleVarEventHandler UnityVarHandler = null;
      public System.Object UnityObject = null;
      public GameObject UIAdded = null;

      // Engine has confirmed an update or init
      public void SetValue(ConsoleVarUnion varValue) {
        TagType = varValue.GetTag();
        switch (TagType) {
        case ConsoleVarUnion.Tag.varDouble:
          ValueAsDouble = varValue.varDouble;
          break;
        case ConsoleVarUnion.Tag.varInt:
          ValueAsInt64 = varValue.varInt;
          break;
        case ConsoleVarUnion.Tag.varUint:
          ValueAsUInt64 = varValue.varUint;
          break;
        case ConsoleVarUnion.Tag.varBool:
          ValueAsUInt64 = varValue.varBool;
          break;
        }
        if (UIAdded != null) {
          ConsoleVarLine uiLine = UIAdded.GetComponent<ConsoleVarLine>();
          if (uiLine) {
            uiLine.OnValueRefreshed();
          }
        }
      }

      public int CompareTo(object obj) {
        if (obj != null) {
          DebugConsoleVarData otherEntry = obj as DebugConsoleVarData;
          if (otherEntry != null) {
            // Sort primarily on category, then on name if category matches

            int catCompare = (Category.CompareTo(otherEntry.Category));
            if (catCompare == 0) {
              return VarName.CompareTo(otherEntry.VarName);
            }
            else {
              return catCompare;
            }
          }
        }

        // sort ahead of null / invalid objects
        return 1;
      }
    }

    private bool _EngineCallbacksRegistered = false;
    private bool _NeedsUIUpdate;
    private Dictionary<string, List<DebugConsoleVarData>> _DataByCategory;
    private ConsoleInitUnityData _UnityData = null;
    public ConsoleInitUnityData UnityData { get { return _UnityData; } }

    private DebugConsolePane _ConsolePane;

    public DebugConsolePane ConsolePane { set { _ConsolePane = value; } }
    public event Action<string> DebugConsoleVarUpdated;

    private void HandleInitDebugConsoleVar(Anki.Cozmo.ExternalInterface.InitDebugConsoleVarMessage message) {
      DAS.Info("RobotEngineManager.ReceivedDebugConsoleInit", "Received Debug Console Init");
      for (int i = 0; i < message.varData.Length; ++i) {
        Anki.Debug.DebugConsoleData.Instance.AddConsoleVar(message.varData[i]);
      }
    }

    private void HandleVerifyDebugConsoleVar(Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage message) {
      DebugConsoleData.Instance.SetStatusText(message.statusMessage);
      DebugConsoleVarData varData = GetExistingLineUI(message.varName);
      if (varData != null) {
        varData.SetValue(message.varValue);
      }
    }

    // Only unity vars come through this public api, Engine vars come through above clad messages.
    public void AddConsoleVar(string varName, string categoryName, System.Object obj, float minVal = float.MinValue, float maxVal = float.MaxValue) {
      Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
      consoleVar.category = categoryName;
      consoleVar.varName = varName;
      consoleVar.minValue = minVal;
      consoleVar.maxValue = maxVal;
      BindingFlags bindFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic
                            | BindingFlags.Static;

      System.Object useObj = obj;
      System.Reflection.FieldInfo info = obj.GetType().GetField(varName, bindFlags);
      // if we are of type "Type" than its just a static var.
      if (obj is System.Type) {
        useObj = null;
        info = ((System.Type)obj).GetField(varName, bindFlags);
      }
      if (info == null) {
        DAS.Error("DebugConsole", "Attempted to add nonexistent var: " + varName);
        return;
      }

      if (info.FieldType == typeof(int) || info.FieldType == typeof(uint)) {
        int val = (int)info.GetValue(useObj);
        consoleVar.varValue.varInt = val;
      }
      else if (info.FieldType == typeof(float) || info.FieldType == typeof(double)) {
        float val = (float)info.GetValue(useObj);
        consoleVar.varValue.varDouble = val;
      }
      else if (info.FieldType == typeof(bool)) {
        Boolean val = (Boolean)info.GetValue(useObj);
        consoleVar.varValue.varBool = val ? (byte)1 : (byte)0;
      }
      AddConsoleVar(consoleVar, null, obj);
      _NeedsUIUpdate = true;
    }
    public void AddConsoleFunction(string varName, string categoryName, DebugConsoleVarEventHandler callback = null) {
      Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
      consoleVar.category = categoryName;
      consoleVar.varName = varName;
      consoleVar.varValue.varFunction = varName;
      AddConsoleVar(consoleVar, callback);
      _NeedsUIUpdate = true;
    }
    public void RemoveConsoleData(string varName, string categoryName) {
      List<DebugConsoleVarData> lines;
      if (_DataByCategory.TryGetValue(categoryName, out lines)) {
        for (int i = 0; i < lines.Count; ++i) {
          if (lines[i].VarName == varName) {
            lines.RemoveAt(i);
            break;
          }
        }
      }
    }
    public void RemoveConsoleData(string categoryName) {
      _DataByCategory.Remove(categoryName);
    }
    public void UnityConsoleDataUpdated(string varName) {
      if (DebugConsoleVarUpdated != null) {
        DebugConsoleVarUpdated(varName);
      }
    }

    private DebugConsoleVarData GetExistingLineUI(string categoryName, string varName) {
      List<DebugConsoleVarData> lines;
      if (_DataByCategory.TryGetValue(categoryName, out lines)) {
        for (int i = 0; i < lines.Count; ++i) {
          if (lines[i].VarName == varName) {
            return lines[i];
          }
        }
      }
      return null;
    }

    // Gets instance by name without category
    private DebugConsoleVarData GetExistingLineUI(string varName) {
      foreach (KeyValuePair<string, List<DebugConsoleVarData>> entry in _DataByCategory) {
        // do something with entry.Value or entry.Key
        List<DebugConsoleVarData> lines = entry.Value;
        for (int i = 0; i < lines.Count; ++i) {
          if (lines[i].VarName == varName) {
            return lines[i];
          }
        }
      }
      return null;
    }
    // CSharp can't safely store pointers, so we need a setter delegates
    private void AddConsoleVar(DebugConsoleVar singleVar, DebugConsoleVarEventHandler callback = null, System.Object unityObj = null) {
      _NeedsUIUpdate = true;

      // check for dupes.
      List<DebugConsoleVarData> categoryList;
      if (!_DataByCategory.TryGetValue(singleVar.category, out categoryList)) {
        categoryList = new List<DebugConsoleVarData>();
        _DataByCategory.Add(singleVar.category, categoryList);
      }
      // try to find an existing one if exists or get a new one.
      DebugConsoleVarData varData = GetExistingLineUI(singleVar.category, singleVar.varName);
      if (varData == null) {
        varData = new DebugConsoleVarData();
        categoryList.Add(varData);
      }
      varData.SetValue(singleVar.varValue);
      varData.VarName = singleVar.varName;
      varData.Category = singleVar.category;
      varData.MaxValue = singleVar.maxValue;
      varData.MinValue = singleVar.minValue;

      varData.UnityVarHandler = callback;
      varData.UnityObject = unityObj;
    }

    private GameObject GetPrefabForType(DebugConsoleVarData data) {
      if (data.TagType == ConsoleVarUnion.Tag.varBool) {
        return _ConsolePane.PrefabVarUICheckbox;
      }
      else if (data.TagType == ConsoleVarUnion.Tag.varFunction) {
        return _ConsolePane.PrefabVarUIButton;
      }
      // mins and maxes are just numeric limits... so just stubbing this in
      else if ((data.MaxValue < 10000 && data.MinValue > -10000) &&
               (Math.Abs(data.MaxValue - data.MinValue) > double.Epsilon)) {
        return _ConsolePane.PrefabVarUISlider;
      }
      return _ConsolePane.PrefabVarUIText;
    }

    public bool RefreshCategory(Transform parentTransform, string category_name, string filterText) {
      List<DebugConsoleVarData> lines;
      bool isAnyActive = filterText == "";
      if (_DataByCategory.TryGetValue(category_name, out lines)) {
        lines.Sort();
        for (int i = 0; i < lines.Count; ++i) {
          // check if this already exists...
          if (lines[i].UIAdded == null) {
            GameObject statLine = UIManager.CreateUIElement(GetPrefabForType(lines[i]), parentTransform);
            ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
            uiLine.Init(lines[i], statLine);
          }
          lines[i].UIAdded.SetActive(true);
          if (filterText != "") {
            if (!lines[i].VarName.ToLower().Contains(filterText)) {
              lines[i].UIAdded.SetActive(false);
            }
            else {
              isAnyActive = true;
            }
          }
        }
      }
      return isAnyActive;
    }

    public void SetStatusText(string text) {
      // If unity has forced a console var from code, this will be null.
      if (_ConsolePane) {
        _ConsolePane.PaneStatusText.text = text;
      }
    }

    public List<string> GetSortedCategories() {
      var categories = new List<string>(_DataByCategory.Keys);
      categories.Sort();
      return categories;
    }

    // Get with the singleton pattern, no creating ones outside instance
    private DebugConsoleData() {
      _NeedsUIUpdate = false;
      _DataByCategory = new Dictionary<string, List<DebugConsoleVarData>>();
    }

    public void RegisterEngineCallbacks() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.InitDebugConsoleVarMessage>(HandleInitDebugConsoleVar);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleVerifyDebugConsoleVar);
      if (_UnityData == null) {
        _UnityData = new ConsoleInitUnityData();
        _UnityData.Init();
      }
      _EngineCallbacksRegistered = true;
    }

    public void Dispose() {
      if (_EngineCallbacksRegistered) {
        RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.InitDebugConsoleVarMessage>(HandleInitDebugConsoleVar);
        RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleVerifyDebugConsoleVar);
      }
    }

    public bool NeedsUIUpdate() {
      return _NeedsUIUpdate;
    }
    public bool SetNeedsUIUpdate(bool updateNeeded) {
      return _NeedsUIUpdate = updateNeeded;
    }

    public void OnUIUpdated() {
      _NeedsUIUpdate = false;
    }


  }
}
