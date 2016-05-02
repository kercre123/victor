using UnityEngine;
using System.Collections;

// Public singleton just to dump shared data.
// We can store data from the engine we got from CLAD
// as well as unity data if needed.
using Anki.Cozmo;
using System;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Anki.Debug {
  public class DebugConsoleData {

    private static DebugConsoleData _Instance = new DebugConsoleData();

    public static DebugConsoleData Instance { get { return _Instance; } }

    public delegate void DebugConsoleVarEventHandler(System.Object setvar);

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
      public bool UIAdded = false;

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

    private bool _NeedsUIUpdate;
    private Dictionary<string, List<DebugConsoleVarData> > _DataByCategory;

    private DebugConsolePane _ConsolePane;

    public DebugConsolePane ConsolePane { set { _ConsolePane = value; } }

    public void AddConsoleFunctionUnity(string varName, string categoryName, DebugConsoleVarEventHandler callback = null) {
      Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
      consoleVar.category = categoryName;
      consoleVar.varName = varName;
      consoleVar.varValue.varFunction = varName;
      AddConsoleVar(consoleVar, callback);
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
    // CSharp can't safely store pointers, so we need a setter delegates
    public void AddConsoleVar(DebugConsoleVar singleVar, DebugConsoleVarEventHandler callback = null) {
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
      varData.TagType = singleVar.varValue.GetTag();
      switch (varData.TagType) {
      case ConsoleVarUnion.Tag.varDouble:
        varData.ValueAsDouble = singleVar.varValue.varDouble;
        break;
      case ConsoleVarUnion.Tag.varInt:
        varData.ValueAsInt64 = singleVar.varValue.varInt;
        break;
      case ConsoleVarUnion.Tag.varUint:
        varData.ValueAsUInt64 = singleVar.varValue.varUint;
        break;
      case ConsoleVarUnion.Tag.varBool:
        varData.ValueAsUInt64 = singleVar.varValue.varBool;
        break;
      }
      varData.VarName = singleVar.varName;
      varData.Category = singleVar.category;
      varData.MaxValue = singleVar.maxValue;
      varData.MinValue = singleVar.minValue;

      varData.UnityVarHandler = callback;
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
             (data.MaxValue != data.MinValue)) {
        return _ConsolePane.PrefabVarUISlider;
      }
      return _ConsolePane.PrefabVarUIText;
    }

    public bool RefreshCategory(Transform parentTransform, string category_name) {
      List<DebugConsoleVarData> lines;
      if (_DataByCategory.TryGetValue(category_name, out lines)) {
        lines.Sort();
        for (int i = 0; i < lines.Count; ++i) {
          // check if this already exists...
          if (!lines[i].UIAdded) {
            GameObject statLine = UIManager.CreateUIElement(GetPrefabForType(lines[i]), parentTransform);
            ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
            uiLine.Init(lines[i]);
          }
        }
      }
      return true;
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
      _DataByCategory = new Dictionary<string, List<DebugConsoleVarData> >();
    }

    public bool NeedsUIUpdate() {
      return _NeedsUIUpdate;
    }

    public void OnUIUpdated() {
      _NeedsUIUpdate = false;
    }


  }
}
