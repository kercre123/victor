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
    public debugConsoleType _nativeType;

    public double _minValue;
    public double _maxValue;

    // most integral types can be expressed as one of the following
    public double _valueAsDouble;
    public long _valueAsInt64;
    public ulong _valueAsUInt64;
  }

  private bool _NeedsUIUpdate;
  private List<DebugConsoleVarData> _DataListVar;

  public void AddConsoleVar(DebugConsoleVar singleVar) {
    _NeedsUIUpdate = true;

    // TODO: check for dupes.
    DebugConsoleVarData varData = new DebugConsoleVarData();

    varData._nativeType = singleVar.varType;
    switch (varData._nativeType) {
    case debugConsoleType.Double:
      varData._valueAsDouble = singleVar.varValue.varDouble;
      break;
    case debugConsoleType.Int64:
      varData._valueAsInt64 = singleVar.varValue.varInt;
      break;
    case debugConsoleType.Uint64:
      varData._valueAsUInt64 = singleVar.varValue.varUint;
      break;
    }
    varData._varName = singleVar.varName;
    varData._category = singleVar.category;

    _DataListVar.Add(varData);
  }
    
  // Get with the singleton pattern, no creating ones outside instance
  private DebugConsoleData() { 
    _NeedsUIUpdate = false;
    _DataListVar = new List<DebugConsoleVarData>();
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
