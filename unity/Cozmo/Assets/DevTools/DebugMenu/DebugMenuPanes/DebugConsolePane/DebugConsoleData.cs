using UnityEngine;
using System.Collections;

// Public singleton just to dump shared data.
// We can store data from the engine we got from CLAD
// as well as unity data if needed.
using Anki.Cozmo;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;


public class DebugConsoleData {

  private static DebugConsoleData _Instance = new DebugConsoleData();

  public static DebugConsoleData Instance { get { return _Instance; } }


  public class DebugConsoleVarData {
    public DebugConsoleVarData() {
    }

    public string _varName;
    public string _category;
    public consoleVarUnion.Tag _tagType;

    public double _minValue;
    public double _maxValue;

    // most integral types can be expressed as one of the following
    public double _valueAsDouble;
    public long _valueAsInt64;
    public ulong _valueAsUInt64;
  }

  private bool _NeedsUIUpdate;
  private List<DebugConsoleVarData> _DataListVar;
  private Dictionary<string, List<DebugConsoleVarData> > _DataByCategory;

  public GameObject _PrefabVarUIText;
  public GameObject _PrefabVarUICheckbox;
  public GameObject _PrefabVarUIButton;
  public GameObject _PrefabVarUISlider;

  public void AddConsoleVar(DebugConsoleVar singleVar) {
    _NeedsUIUpdate = true;

    // check for dupes.
    List<DebugConsoleVarData> categoryList;
    if (!_DataByCategory.TryGetValue(singleVar.category, out categoryList)) {
      categoryList = new List<DebugConsoleVarData>();
      _DataByCategory.Add(singleVar.category, categoryList);
    }

    DebugConsoleVarData varData = new DebugConsoleVarData();
    DAS.Info("AddConsoleVar", "Type: " + singleVar.varValue.GetTag().ToString());
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

    categoryList.Add(varData);
    _DataListVar.Add(varData);
  }

  private GameObject GetPrefabForType(DebugConsoleVarData data) {
    if (data._tagType == consoleVarUnion.Tag.varBool) {
      return _PrefabVarUICheckbox;
    }
    else if (data._tagType == consoleVarUnion.Tag.varFunction) {
      return _PrefabVarUIButton;
    }
    // mins and maxes are just numeric limits... so just stubbing this in
    else if (data._maxValue < 1000000 && data._minValue > -100000) {
      return _PrefabVarUISlider;
    }
    return _PrefabVarUIText;
  }

  public bool AddCategory(Transform parentTransform, string category_name) {
    // TODO: clear previous ones or at least diff they exist.

    List<DebugConsoleVarData> lines;
    if (_DataByCategory.TryGetValue(category_name, out lines)) {
      for (int i = 0; i < lines.Count; ++i) {
        GameObject statLine = UIManager.CreateUIElement(GetPrefabForType(lines[i]), parentTransform);
        ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
        uiLine.Init(lines[i]);
      }
    }


    return true;
  }

  public List<string> GetCategories() {
    return new List<string>(_DataByCategory.Keys);
  }
    
  // Get with the singleton pattern, no creating ones outside instance
  private DebugConsoleData() { 
    _NeedsUIUpdate = false;
    _DataListVar = new List<DebugConsoleVarData>();
    _DataByCategory = new Dictionary<string, List<DebugConsoleVarData> >();
  }

  public int GetCountVars() {
    return _DataListVar.Count;
  }

  public DebugConsoleVarData GetDataAtIndex(int index) {
    return _DataListVar[index];
  }

  public bool NeedsUIUpdate() {
    return _NeedsUIUpdate;
  }

  public void OnUIUpdated() {
    _NeedsUIUpdate = false;
  }


}
