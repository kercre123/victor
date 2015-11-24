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
    }

    private bool _NeedsUIUpdate;
    private Dictionary<string, List<DebugConsoleVarData> > _DataByCategory;

    private DebugConsolePane _ConsolePane;

    public DebugConsolePane ConsolePane { set { _ConsolePane = value; } }

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

      categoryList.Add(varData);
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
        for (int i = 0; i < lines.Count; ++i) {
          // check if this already exists...
          if (!lines[i].UIAdded) {
            GameObject statLine = UIManager.CreateUIElement(GetPrefabForType(lines[i]), parentTransform);
            ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
            uiLine.Init(lines[i]);
            lines[i].UIAdded = true;
          }
        }
      }
      return true;
    }

    public void SetStatusText(string text) {
      _ConsolePane.PaneStatusText.text = text;
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
