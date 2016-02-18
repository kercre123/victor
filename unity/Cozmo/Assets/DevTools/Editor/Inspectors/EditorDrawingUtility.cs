using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
using System.Linq;

public static class EditorDrawingUtility {

  public static void DrawList<T>(string label, List<T> list, Func<T,T> drawControls, Func<T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc());
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i]);


      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(i);
        i--;
      }

      if (GUILayout.Button("+", GUILayout.Width(30))) {        
        list.Insert(i + 1, createFunc());
      }

      EditorGUILayout.EndHorizontal();
    }
  }

  public static void DrawListWithIndex<T>(string label, List<T> list, Func<T, int, T> drawControls, Func<int, T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc(0));
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i], i);


      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(i);
        i--;
      }

      if (GUILayout.Button("+", GUILayout.Width(30))) {        
        list.Insert(i + 1, createFunc(i + 1));
      }

      EditorGUILayout.EndHorizontal();
    }
  }

  public static void DrawGroupedList<T,U>(string label, List<T> list, Func<T,T> drawControls, Func<T> createFunc, Func<T,U> groupBy, Func<U,string> getGroupLabel)
    where U : IComparable
  {
    EditorGUILayout.BeginVertical();
    GUILayout.Label(label);

    var splitList = list.GroupBy(groupBy).Select(g => new KeyValuePair<U, List<T>>(g.Key, g.ToList()));

    foreach (var entry in splitList) {
      DrawList(getGroupLabel(entry.Key), entry.Value, drawControls, createFunc);
    }

    var tmp = splitList.SelectMany(kvp => kvp.Value).ToArray();
    list.Clear();
    list.AddRange(tmp);

    EditorGUILayout.EndVertical();
  }

  public static void DrawDictionary<T>(string label, Dictionary<string, T> dict, Func<T,T> drawControls, Func<T> createFunc) {

    var list = new List<KeyValuePair<string,T>>(dict);

    DrawList(label, list, x => {
      string key = x.Key;
      T value = x.Value;

      EditorGUILayout.BeginHorizontal();

      key = EditorGUILayout.TextField(key);

      value = drawControls(value);

      EditorGUILayout.EndHorizontal();

      return new KeyValuePair<string, T>(key, value);
    },
      () => {
        string key = string.Empty;

        while (dict.ContainsKey(key)) {
          key = "New" + key;
        }

        T value = createFunc();

        return new KeyValuePair<string, T>(key, value);
      });

    dict.Clear();

    foreach (var kvp in list) {
      dict[kvp.Key] = kvp.Value;
    }
  }

  public static void InitializeLocalizationString(string localizationKey, out string localizedStringFile, out string localizedString) {
    localizedString = LocalizationEditorUtility.GetTranslation(localizationKey, out localizedStringFile);
  }

  public static void DrawLocalizationString(ref string localizationKey, ref string localizedStringFile, ref string localizedString) {
    int selectedFileIndex = EditorGUILayout.Popup("Localization File", 
                              Mathf.Max(0, 
                                System.Array.IndexOf(
                                  LocalizationEditorUtility.LocalizationFiles, 
                                  localizedStringFile)), 
                              LocalizationEditorUtility.LocalizationFiles);
    localizedStringFile = LocalizationEditorUtility.LocalizationFiles[selectedFileIndex];
       
    var lastLocKey = localizationKey;
    localizationKey = EditorGUILayout.TextField("Localization Key", localizationKey);

    int quickSelect = EditorGUILayout.Popup("   ", 0, LocalizationEditorUtility.LocalizationKeys);
    if (quickSelect > 0) {
      localizationKey = LocalizationEditorUtility.LocalizationKeys[quickSelect];
      localizedString = string.Empty;
    }
      
    if (localizationKey != lastLocKey && (string.IsNullOrEmpty(localizedString) || 
        localizedString == LocalizationEditorUtility.GetTranslation(localizedStringFile, lastLocKey))) {
      InitializeLocalizationString(localizationKey, out localizedStringFile, out localizedString);
    }

    GUILayout.Label("Text");
    localizedString = GUILayout.TextArea(localizedString, GUILayout.Height(45));

    if (LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey) != localizedString) {
      EditorGUILayout.BeginHorizontal();
      if (GUILayout.Button("Reset")) {
        localizedString = LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey);
      }
      if (GUILayout.Button("Save")) {
        LocalizationEditorUtility.SetTranslation(localizedStringFile, localizationKey, localizedString);
      }
      EditorGUILayout.EndHorizontal();
    }
  }

  public static EmotionScorer DrawEmotionScorer(EmotionScorer emotionScorer) {
    EditorGUILayout.BeginVertical();

    emotionScorer.EmotionType = (Anki.Cozmo.EmotionType)EditorGUILayout.EnumPopup("Emotion", emotionScorer.EmotionType);

    int trackDeltaMultiplier = emotionScorer.TrackDelta ? 2 : 1;
    emotionScorer.GraphEvaluator = EditorGUILayout.CurveField("Score Graph", emotionScorer.GraphEvaluator, Color.green, new Rect(-1 * trackDeltaMultiplier, 0, 2 * trackDeltaMultiplier, 1));

    emotionScorer.TrackDelta = EditorGUILayout.Toggle("Track Delta", emotionScorer.TrackDelta);

    EditorGUILayout.EndVertical();
    return emotionScorer;
  }

  // custom Unity Style for a toolbar button
  private static GUIStyle _ToolbarButtonStyle;

  public static GUIStyle ToolbarButtonStyle {
    get {
      if (_ToolbarButtonStyle == null) {
        _ToolbarButtonStyle = new GUIStyle(EditorStyles.toolbarButton);
        _ToolbarButtonStyle.normal.textColor = Color.white;
        _ToolbarButtonStyle.active.textColor = Color.white;
      }
      return _ToolbarButtonStyle;
    }
  }

  // custom Unity Style for a toolbar button
  private static GUIStyle _ToolbarStyle;

  public static GUIStyle ToolbarStyle {
    get {
      if (_ToolbarStyle == null) {
        _ToolbarStyle = new GUIStyle(EditorStyles.toolbar);
        _ToolbarStyle.normal.textColor = Color.white;
        _ToolbarStyle.active.textColor = Color.white;
      }
      return _ToolbarStyle;
    }
  }

  private static readonly string[] _TypeOptions;

  private static readonly Dictionary<string, Type> _TypeDictionary = new Dictionary<string, Type>();
  private static readonly Dictionary<Type, string> _TypeNameDictionary = new Dictionary<Type, string>();

  private static readonly Dictionary<Type, Func<object, object>> _TypeDrawers = new Dictionary<Type, Func<object, object>>();

  private static void AddTypeOption<T>(string name, Func<T, T> drawer) {
    _TypeDictionary[name] = typeof(T);
    _TypeNameDictionary[typeof(T)] = name;
    _TypeDrawers[typeof(T)] = x => drawer((T)(object)x);
  }

  private static void AddListTypeOption<T>(string name) {
    AddTypeOption<List<T>>("List<"+name+">", x => {
      DrawList("", x, y => (T)_TypeDrawers[typeof(T)](y), () => default(T));
      return x;
    });
  }

  private static void AddConvertTypeOption<T,U>() {
    _TypeNameDictionary[typeof(T)] = _TypeNameDictionary[typeof(U)];
    _TypeDrawers[typeof(T)] = x => Convert.ChangeType(_TypeDrawers[typeof(U)](Convert.ChangeType(x, typeof(U))), typeof(T));
  }

  static EditorDrawingUtility() {

    _TypeDictionary.Add("null", null);

    AddTypeOption<int>("int", i => EditorGUILayout.IntField(i));
    AddTypeOption<float>("float", f =>EditorGUILayout.FloatField(f));
    AddTypeOption<string>("string", s => EditorGUILayout.TextArea(s ?? string.Empty));
    AddTypeOption<bool>("bool", b => EditorGUILayout.Toggle(b));
    AddConvertTypeOption<long,int>();
    AddConvertTypeOption<double,float>();
    AddTypeOption<EmotionScorer>("EmotionScorer", DrawEmotionScorer);
    AddTypeOption<AnimationCurve>("AnimationCurve", c => EditorGUILayout.CurveField(c));
    AddListTypeOption<int>("int");
    AddListTypeOption<float>("float");
    AddListTypeOption<string>("string");

    _TypeOptions = _TypeDictionary.Keys.ToArray();
  }
    
  public static object DrawSelectionObjectEditor(object x) {

    EditorGUILayout.BeginHorizontal();

    string typeName;
    int index;

    if (x != null && _TypeNameDictionary.TryGetValue(x.GetType(), out typeName)) {
      index = Array.IndexOf(_TypeOptions, typeName);
    }
    else {      
      index = 0;
      x = null;
    }

    int newIndex = EditorGUILayout.Popup(index, _TypeOptions);

    if (newIndex != index) {
      var newType = _TypeDictionary[_TypeOptions[newIndex]];
      x = ConvertOrNew(x, newType);
    }

    var result = DrawObjectEditor(x);

    EditorGUILayout.EndHorizontal();
    return result;
  }

  private static object ConvertOrNew(object x, Type newType) {
    if (newType == null) {
      return null;
    }

    if (x == null) {
      if (newType == typeof(string)) {
        return string.Empty;
      }
      return Activator.CreateInstance(newType);
    }

    if (newType == typeof(string)) {
      return x.ToString();
    }

    if (x.GetType() == typeof(string)) {
      if(newType == typeof(int)) {
        int intVal;
        if (int.TryParse((string)x, out intVal)) {
          return intVal;
        }
      }
      if(newType == typeof(float)){
        float floatVal;
        if (float.TryParse((string)x, out floatVal)) {
          return floatVal;
        }
      }
      if (newType == typeof(bool)) {
        return "true".Equals((string)x, comparisonType: StringComparison.InvariantCultureIgnoreCase);
      }
    }

    if (x.GetType() == typeof(bool)) {
      if(newType == typeof(int)) {
        return (bool)x == true ? 1 : 0;
      }
      if(newType == typeof(float)){
        return (bool)x == true ? 1f : 0f;
      }
    }

    if(newType == typeof(bool)) {
      if (x.GetType() == typeof(int)) {
        return (int)x != 0;
      }
      if (x.GetType() == typeof(float)) {
        return (float)x != 0;
      }
    }

    if (x.GetType() == typeof(int) && newType == typeof(float)) {
      return (float)(int)x;
    }
    if (x.GetType() == typeof(float) && newType == typeof(int)) {
      return (int)(float)x;
    }

    return Activator.CreateInstance(newType);
  }

  public static object DrawObjectEditor(object x) {
    if (x == null) {
      return null;
    }
    Func<object, object> drawer;
    if (_TypeDrawers.TryGetValue(x.GetType(), out drawer)) {
      return drawer(x);
    }
    else {
      Debug.LogError("Don't know how to draw editor for type " + x.GetType());
      return null;
    }
  }


}

