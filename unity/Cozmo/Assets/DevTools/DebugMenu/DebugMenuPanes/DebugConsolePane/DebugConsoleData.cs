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
  //private Dictionary<string, List<DebugConsoleVar> > _DataByCategory;

  public void AddConsoleVar(DebugConsoleVar singleVar) {
    _NeedsUIUpdate = true;


    // TODO: check for dupes.
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
    }
    varData._varName = singleVar.varName;
    varData._category = singleVar.category;

    _DataListVar.Add(varData);
  }

  public bool InitUI(GameObject uiPrefab, Transform parentTransform) {
    
    return true;
  }
    
  // Get with the singleton pattern, no creating ones outside instance
  private DebugConsoleData() { 
    _NeedsUIUpdate = false;
    _DataListVar = new List<DebugConsoleVarData>();
    //_DataByCategory = new Dictionary<string, List<DebugConsoleVar> >();
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
