using UnityEngine;
using System.Collections;

// Public singleton just to dump shared data.
// We can store data from the engine we got from CLAD
// as well as unity data if needed.
using Anki.Cozmo;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Anki.Debug {
  public class DebugConsoleData {

    private static DebugConsoleData _Instance = new DebugConsoleData();

    public static DebugConsoleData Instance { get { return _Instance; } }

    public delegate void DebugConsoleVarEventHandler(System.Object setvar);

    public class DebugConsoleVarData {

      public string _varName;
      public string _category;
      public consoleVarUnion.Tag _tagType;

      public double _minValue;
      public double _maxValue;

      // most integral types can be expressed as one of the following
      public double _valueAsDouble;
      public long _valueAsInt64;
      public ulong _valueAsUInt64;
      // C# setter function
      public DebugConsoleVarEventHandler _unityVarHandler = null;
      public bool _UIAdded = false;
    }

    private bool _NeedsUIUpdate;
    private Dictionary<string, List<DebugConsoleVarData> > _DataByCategory;

    public DebugConsolePane _ConsolePane;

    // CSharp can't safely store pointers, so we need a setter delegates
    public void AddConsoleVar(DebugConsoleVar singleVar, DebugConsoleVarEventHandler callback = null) {
      _NeedsUIUpdate = true;

      // check for dupes.
      List<DebugConsoleVarData> categoryList;
      if (!_DataByCategory.TryGetValue(singleVar.category, out categoryList)) {
        categoryList = new List<DebugConsoleVarData>();
        _DataByCategory.Add(singleVar.category, categoryList);
      }

      DebugConsoleVarData varData = new DebugConsoleVarData();
      varData._tagType = singleVar.varValue.GetTag();
      switch (varData._tagType) {
      case consoleVarUnion.Tag.varDouble:
        varData._valueAsDouble = singleVar.varValue.varDouble;
        break;
      case consoleVarUnion.Tag.varInt:
        varData._valueAsInt64 = singleVar.varValue.varInt;
        break;
      case consoleVarUnion.Tag.varUint:
        varData._valueAsUInt64 = singleVar.varValue.varUint;
        break;
      case consoleVarUnion.Tag.varBool:
        varData._valueAsUInt64 = singleVar.varValue.varBool;
        break;
      }
      varData._varName = singleVar.varName;
      varData._category = singleVar.category;
      varData._maxValue = singleVar.maxValue;
      varData._minValue = singleVar.minValue;

      varData._unityVarHandler = callback;

      categoryList.Add(varData);
    }

    private GameObject GetPrefabForType(DebugConsoleVarData data) {
      if (data._tagType == consoleVarUnion.Tag.varBool) {
        return _ConsolePane._PrefabVarUICheckbox;
      }
      else if (data._tagType == consoleVarUnion.Tag.varFunction) {
        return _ConsolePane._PrefabVarUIButton;
      }
    // mins and maxes are just numeric limits... so just stubbing this in
    else if ((data._maxValue < 10000 && data._minValue > -10000) &&
             (data._maxValue != data._minValue)) {
        return _ConsolePane._PrefabVarUISlider;
      }
      return _ConsolePane._PrefabVarUIText;
    }

    public bool RefreshCategory(Transform parentTransform, string category_name) {
      List<DebugConsoleVarData> lines;
      if (_DataByCategory.TryGetValue(category_name, out lines)) {
        for (int i = 0; i < lines.Count; ++i) {
          // check if this already exists...
          if (!lines[i]._UIAdded) {
            GameObject statLine = UIManager.CreateUIElement(GetPrefabForType(lines[i]), parentTransform);
            ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
            uiLine.Init(lines[i]);
            lines[i]._UIAdded = true;
          }
        }
      }
      return true;
    }

    public void SetStatusText(string text) {
      _ConsolePane._PaneStatusText.text = text;
    }

    public List<string> GetCategories() {
      return new List<string>(_DataByCategory.Keys);
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
