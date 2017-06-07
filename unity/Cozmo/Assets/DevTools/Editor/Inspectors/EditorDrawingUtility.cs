using System;
using System.Reflection;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
using System.Linq;
using Anki.Cozmo;

public static class EditorDrawingUtility {

  public static void DrawList<T>(string label, List<T> list, Func<T, T> drawControls, Func<T> createFunc, bool includeCreateFunc = true, bool includeDeleteFunc = true) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label, SubtitleStyle);

    if (includeCreateFunc) {
      if (GUILayout.Button("+", GUILayout.Width(30))) {
        list.Add(createFunc());
      }
    }
    if (includeDeleteFunc) {
      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(0);
      }
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i]);


      EditorGUILayout.EndHorizontal();
    }
  }

  public static void DrawListWithIndex<T>(string label, List<T> list, Func<T, int, T> drawControls, Func<int, T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc(0));
    }
    if (GUILayout.Button("-", GUILayout.Width(30))) {
      list.RemoveAt(0);
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i], i);

      if (GUILayout.Button("+", GUILayout.Width(30))) {
        list.Insert(i + 1, createFunc(i + 1));
      }

      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(i);
        i--;
      }

      EditorGUILayout.EndHorizontal();
    }
  }

  /// <summary>
  /// Draws the filtered grouped list. Filters based on U.ToString() containing filterVal, default Val is always visible even with filter used
  /// </summary>
  public static void DrawFilteredGroupedList<T, U>(string label, List<T> list, Func<T, T> drawControls, Func<T> createFunc, Func<T, U> groupBy, string filterVal, U defaultVal, Func<U, string> getGroupLabel)
    where U : IComparable {
    EditorGUILayout.BeginVertical();
    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label, TitleStyle);

    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc());
    }

    EditorGUILayout.EndHorizontal();

    var splitList = list.GroupBy(groupBy).Select(g => new KeyValuePair<U, List<T>>(g.Key, g.ToList()));

    foreach (var entry in splitList) {
      if (String.IsNullOrEmpty(filterVal) || (entry.Key.Equals(defaultVal)) || entry.Key.ToString().Contains(filterVal)) {
        EditorDrawingUtility.DrawList(getGroupLabel(entry.Key), entry.Value, drawControls, createFunc, false, false);
      }
    }

    var tmp = splitList.SelectMany(kvp => kvp.Value).ToArray();
    list.Clear();
    list.AddRange(tmp);

    EditorGUILayout.EndVertical();
  }

  public static void DrawGroupedList<T, U>(string label, List<T> list, Func<T, T> drawControls, Func<T> createFunc, Func<T, U> groupBy, Func<U, string> getGroupLabel)
    where U : IComparable {
    EditorGUILayout.BeginVertical();
    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label, TitleStyle);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc());
    }
    if (GUILayout.Button("-", GUILayout.Width(30))) {
      list.RemoveAt(0);
    }
    EditorGUILayout.EndHorizontal();

    var splitList = list.GroupBy(groupBy).Select(g => new KeyValuePair<U, List<T>>(g.Key, g.ToList()));

    foreach (var entry in splitList) {
      DrawList(getGroupLabel(entry.Key), entry.Value, drawControls, createFunc, false, false);
    }

    var tmp = splitList.SelectMany(kvp => kvp.Value).ToArray();
    list.Clear();
    list.AddRange(tmp);

    EditorGUILayout.EndVertical();
  }

  public static void DrawDictionary<T>(string label, Dictionary<string, T> dict, Func<T, T> drawControls, Func<T> createFunc) {

    var list = new List<KeyValuePair<string, T>>(dict);

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

  private static string[] GetLocalizationFileOptions() {
    var ret = new string[LocalizationEditorUtility.LocalizationFiles.Length + 1];
    ret[0] = "Any File";
    LocalizationEditorUtility.LocalizationFiles.CopyTo(ret, 1);
    return ret;
  }

  private static string[] GetLocalizationKeyOptions(int selectedFileIndex, out string[] localizedKeys) {
    bool useAnyFile = (selectedFileIndex == -1);
    string localizedStringFile = null;
    if (!useAnyFile) {
      localizedStringFile = LocalizationEditorUtility.LocalizationFiles[selectedFileIndex];
    }
    List<string> options = new List<string>(100);
    options.Add(" ");//Default option

    //Get all keys or just the keys from the selected file
    if (!useAnyFile) {
      localizedKeys = LocalizationEditorUtility.GetAllKeysInFile(localizedStringFile);
      Array.Sort(localizedKeys);
    }
    else {
      localizedKeys = LocalizationEditorUtility.LocalizationKeys;
    }

    for (int i = 0; i < localizedKeys.Length; i++) {
      var optionKey = localizedKeys[i];

      //If it starts with our localization file name
      {
        //Append localization preview
        string previewString = "";
        if (!string.IsNullOrEmpty(localizedStringFile)) {
          previewString = LocalizationEditorUtility.GetTranslation(localizedStringFile, optionKey);
        }
        else {
          var tempLocFileName = LocalizationEditorUtility.GetLocalizationFileForLocalizedKey(optionKey);

          if (!string.IsNullOrEmpty(tempLocFileName)) {
            previewString = LocalizationEditorUtility.GetTranslation(tempLocFileName, optionKey);
          }

        }

        const int kmaxPreviewStringLength = 30;
        //Truncate longer strings
        if (previewString.Length > kmaxPreviewStringLength) {
          previewString = previewString.Substring(0, kmaxPreviewStringLength - 3) + "...";
        }
        //Remove slashes because / creates a sub menu.
        previewString = previewString.Replace('/', '\0');

        if (string.IsNullOrEmpty(previewString)) {
          options.Add(optionKey); //Don't preview empty strings
        }
        else {
          options.Add(string.Format("{0} ({1})", optionKey, previewString));
        }
      }
    }
    return options.ToArray();
  }

  public static void DrawLocalizationString(ref string localizationKey) {
    string fileName = "";
    string localizedString = "";

    localizedString = LocalizationEditorUtility.GetTranslation(localizationKey, out fileName);

    DrawLocalizationString(ref localizationKey, ref fileName, ref localizedString);
  }

  public static void DrawLocalizationString(ref string localizationKey, ref string localizedStringFile) {
    string localizedString = LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey);
    DrawLocalizationString(ref localizationKey, ref localizedStringFile, ref localizedString);
  }

  public static void DrawLocalizationString(ref string localizationKey, ref string localizedStringFile, ref string localizedString) {
    var rect = EditorGUILayout.GetControlRect(true, 8 * EditorGUIUtility.singleLineHeight);
    DrawLocalizationString(rect, ref localizationKey, ref localizedStringFile, ref localizedString);
  }

  public static void DrawLocalizationString(Rect position, ref string localizationKey, ref string localizedStringFile, ref string localizedString) {

    const int lineCount = 8;
    float lineHeight = position.height / lineCount;

    //Prepend the Any option if people want to scroll through the entire list.
    string[] localizationFileOptions = GetLocalizationFileOptions();
    position.height = lineHeight;

    int selectedFileIndex = EditorGUI.Popup(position, "Localization File",
                              Mathf.Max(0,
                                System.Array.IndexOf(
                                  localizationFileOptions,
                                  localizedStringFile)),
                              localizationFileOptions);
    //Handle offset for default option. -1 means Any File
    selectedFileIndex--;
    if (selectedFileIndex == -1) {
      localizedStringFile = "";
    }
    else {
      localizedStringFile = LocalizationEditorUtility.LocalizationFiles[selectedFileIndex];
    }

    position.y += lineHeight;

    var lastLocKey = localizationKey;
    localizationKey = EditorGUI.TextField(position, "Localization Key", localizationKey);

    position.y += lineHeight;


    string[] localizationKeys;
    //Get Options for current localization file
    string[] options = GetLocalizationKeyOptions(selectedFileIndex, out localizationKeys);

    int quickSelect = EditorGUI.Popup(position, "   ", 0, options);

    string keyNamespace = string.Empty;

    //Ignore default option
    if (quickSelect > 0) {
      //Offset to account for default option
      quickSelect--;
      localizationKey = localizationKeys[quickSelect];
      localizedString = string.Empty;
    }

    if (!string.IsNullOrEmpty(localizationKey)) {
      keyNamespace = localizationKey.Split('.')[0];
    }

    if (localizationKey != lastLocKey && (string.IsNullOrEmpty(localizedString) ||
        localizedString == LocalizationEditorUtility.GetTranslation(localizedStringFile, lastLocKey))) {
      InitializeLocalizationString(localizationKey, out localizedStringFile, out localizedString);
    }

    bool keyDoesNotExistInFile = !LocalizationEditorUtility.KeyExists(localizedStringFile, localizationKey);
    bool noBestFitFileForKey = false;
    string potentialLocalizedStringFile = null;
    //Check every update if there is a potential file for warning, but don't set it till input.
    if (keyDoesNotExistInFile && !string.IsNullOrEmpty(localizationKey)) {
      potentialLocalizedStringFile = LocalizationEditorUtility.FindBestFitFileForKey(localizationKey);
      noBestFitFileForKey = string.IsNullOrEmpty(potentialLocalizedStringFile);
    }

    //If new input on the string, update string file to the best fit.
    if (keyDoesNotExistInFile && localizationKey != lastLocKey && !string.IsNullOrEmpty(localizationKey)) {
      localizedStringFile = potentialLocalizedStringFile;
    }

    position.y += lineHeight;
    GUI.Label(position, "Text");

    position.y += lineHeight;
    position.height = 3 * lineHeight;
    localizedString = GUI.TextArea(position, localizedString);

    position.y += 3 * lineHeight;
    position.height = lineHeight;
    position.width *= 0.5f;

    //Let users know they may have a typo
    if (noBestFitFileForKey) {
      var c = GUI.color;
      GUI.color = Color.red;
      GUILayout.Label(string.Format("I don't know what file \"{0}\" refers to!", keyNamespace));
      GUI.color = c;
    }

    if (!string.IsNullOrEmpty(localizedStringFile)) {

      if (LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey) != localizedString) {
        if (GUI.Button(position, "Reset")) {
          localizedString = LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey);
        }
        position.x += position.width;
        if (keyDoesNotExistInFile ? GUI.Button(position, "Create New") : GUI.Button(position, "Save")) {
          if (keyDoesNotExistInFile) {
            if (string.IsNullOrEmpty(keyNamespace)) {
              keyNamespace = localizationKey.Split('.')[0];
            }

            //Double check with user if they want to make a new namespace.
            if (!LocalizationEditorUtility.GetKeyNamespacesForLocalizationFile(localizedStringFile).Contains(keyNamespace)) {
              bool continueDespiteWarning = EditorUtility.DisplayDialog("Warning", string.Format("You are creating a new namespace \"{0}\" in file {1}\n\nAre you sure?",
                                                                   keyNamespace, localizedStringFile), "Yes", "No");
              if (!continueDespiteWarning)
                return;
            }

            bool userWantsToSave = EditorUtility.DisplayDialog("Create New Localization String?",
                                        string.Format("Create new entry \"{0}\" in {1} with English string:\n \"{2}\"",
                                                      localizationKey, localizedStringFile, localizedString),
                                        "OK", "Cancel");
            if (userWantsToSave) {
              LocalizationEditorUtility.SetTranslation(localizedStringFile, localizationKey, localizedString);
              LocalizationEditorUtility.Reload();
            }
          }
          else {
            LocalizationEditorUtility.SetTranslation(localizedStringFile, localizationKey, localizedString);
          }
        }
      }
    }
  }

  public static void DrawTagDropDown(ref string chosenTag) {
    TagConfig config = AssetDatabase.LoadAssetAtPath<TagConfig>(TagConfig.kTagConfigLocation);
    TagConfig.SetInstance(config);
    List<string> tagList = new List<string>();
    tagList.AddRange(TagConfig.GetAllTags());
    string[] tagOptions = tagList.ToArray();
    int currOption = Mathf.Max(0, Array.IndexOf(tagOptions, chosenTag));
    int newOption = EditorGUILayout.Popup("Tag", currOption, tagOptions);
    if (newOption != currOption) {
      chosenTag = tagOptions[newOption];
    }
  }

  public static string GetConditionListString(List<GoalCondition> condList) {
    string newS = string.Format(" ({0})", condList.Count);
    for (int i = 0; i < condList.Count; i++) {
      newS += string.Format("-{0}", condList[i].GetType().Name.ToHumanFriendly());
    }
    return newS;
  }

  // Draws a list of Conditions
  public static void DrawConditionList<T>(List<T> conditions) where T : GoalCondition {
    for (int i = 0; i < conditions.Count; i++) {
      var cond = conditions[i];
      // clear out any null conditions
      if (cond == null) {
        conditions.RemoveAt(i);
        i--;
        continue;
      }

      var name = (cond as T).GetType().Name.ToHumanFriendly();
      (cond as T).OnGUI_DrawUniqueControls();
      if (GUI.Button(EditorGUILayout.GetControlRect(), String.Format("^^Delete {0}^^", name), EditorDrawingUtility.RemoveButtonStyle)) {
        conditions.RemoveAt(i);
      }

    }

    var nextRect = EditorGUILayout.GetControlRect();

    // show the add condition/action box at the bottom of the list
    var newObject = DrawAddConditionPopup<T>(nextRect, ref _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices, _ConditionTypes);

    if (!EqualityComparer<T>.Default.Equals(newObject, default(T))) {
      conditions.Add(newObject);
    }

  }

  // internal function for ShowAddPopup, does the layout of the buttons and actual object creation
  private static T DrawAddConditionPopup<T>(Rect rect, ref int index, string[] names, int[] indices, Type[] types) where T : GoalCondition {
    var popupRect = new Rect(rect.x, rect.y, rect.width - 150, rect.height);
    var plusRect = new Rect(rect.x + rect.width - 150, rect.y, 150, rect.height);
    index = EditorGUI.IntPopup(popupRect, index, names, indices);

    if (GUI.Button(plusRect, "New Condition", EditorDrawingUtility.AddButtonStyle)) {
      var result = (T)(Activator.CreateInstance(types[index]));
      return result;
    }
    return default(T);
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

  #region Style Settings

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

  // Unity Style for the Main DAILY GOALS title at the top
  private static GUIStyle _MainTitleStyle;

  public static GUIStyle MainTitleStyle {
    get {
      if (_MainTitleStyle == null) {
        _MainTitleStyle = new GUIStyle();
        _MainTitleStyle.fontStyle = FontStyle.Bold;
        _MainTitleStyle.normal.textColor = Color.white;
        _MainTitleStyle.active.textColor = Color.white;
        _MainTitleStyle.fontSize = 28;
      }
      return _MainTitleStyle;
    }
  }

  // Unity Style for event labels that separate each group of daily goals
  private static GUIStyle _TitleStyle;

  public static GUIStyle TitleStyle {
    get {
      if (_TitleStyle == null) {
        _TitleStyle = new GUIStyle();
        _TitleStyle.fontStyle = FontStyle.Bold;
        _TitleStyle.normal.textColor = Color.white;
        _TitleStyle.active.textColor = Color.white;
        _TitleStyle.fontSize = 20;
      }
      return _TitleStyle;
    }
  }

  // Unity style for labels within each individual daily goal
  private static GUIStyle _SubtitleStyle;

  public static GUIStyle SubtitleStyle {
    get {
      if (_SubtitleStyle == null) {
        _SubtitleStyle = new GUIStyle();
        _SubtitleStyle.fontStyle = FontStyle.Bold;
        _SubtitleStyle.normal.textColor = Color.white;
        _SubtitleStyle.active.textColor = Color.white;
        _SubtitleStyle.fontSize = 16;
      }
      return _SubtitleStyle;
    }
  }

  // custom Unity Style for a button
  private static GUIStyle _AddButtonStyle;

  public static GUIStyle AddButtonStyle {
    get {
      if (_AddButtonStyle == null) {
        _AddButtonStyle = new GUIStyle(EditorStyles.miniButton);
        _AddButtonStyle.normal.textColor = Color.green;
        _AddButtonStyle.active.textColor = Color.white;
        _AddButtonStyle.fontSize = 12;
      }
      return _AddButtonStyle;
    }
  }

  // custom Unity Style for a button
  private static GUIStyle _RemoveButtonStyle;

  public static GUIStyle RemoveButtonStyle {
    get {
      if (_RemoveButtonStyle == null) {
        _RemoveButtonStyle = new GUIStyle(EditorStyles.miniButton);
        _RemoveButtonStyle.normal.textColor = Color.red;
        _RemoveButtonStyle.active.textColor = Color.white;
        _RemoveButtonStyle.fontSize = 12;
      }
      return _RemoveButtonStyle;
    }
  }

  // TODO: Add in a proper highlight background for when Foldouts are opened, Box Texture2D with some color
  private static GUIStyle _FoldoutStyle;

  public static GUIStyle FoldoutStyle {
    get {
      if (_FoldoutStyle == null) {
        _FoldoutStyle = new GUIStyle(EditorStyles.foldout);
        Color white = Color.white;
        _FoldoutStyle.fontStyle = FontStyle.Bold;
        _FoldoutStyle.normal.textColor = white;
        _FoldoutStyle.onNormal.textColor = white;
        _FoldoutStyle.hover.textColor = white;
        _FoldoutStyle.onHover.textColor = white;
        _FoldoutStyle.focused.textColor = white;
        _FoldoutStyle.onFocused.textColor = white;
        _FoldoutStyle.active.textColor = white;
        _FoldoutStyle.onActive.textColor = white;
      }
      return _FoldoutStyle;
    }
  }

  #endregion

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
    AddTypeOption<List<T>>("List<" + name + ">", x => {
      DrawList("", x, y => (T)_TypeDrawers[typeof(T)](y), () => default(T));
      return x;
    });
  }

  private static void AddConvertTypeOption<T, U>() {
    _TypeNameDictionary[typeof(T)] = _TypeNameDictionary[typeof(U)];
    _TypeDrawers[typeof(T)] = x => Convert.ChangeType(_TypeDrawers[typeof(U)](Convert.ChangeType(x, typeof(U))), typeof(T));
  }


  private static string[] _ConditionTypeNames;
  private static Type[] _ConditionTypes;
  private static int[] _ConditionIndices;
  private static int _SelectedConditionIndex;

  static EditorDrawingUtility() {

    _TypeDictionary.Add("null", null);

    AddTypeOption<int>("int", i => EditorGUILayout.IntField(i));
    AddTypeOption<float>("float", f => EditorGUILayout.FloatField(f));
    AddTypeOption<string>("string", s => EditorGUILayout.TextArea(s ?? string.Empty));
    AddTypeOption<bool>("bool", b => EditorGUILayout.Toggle(b));
    AddConvertTypeOption<long, int>();
    AddConvertTypeOption<double, float>();
    AddTypeOption<EmotionScorer>("EmotionScorer", DrawEmotionScorer);
    AddTypeOption<AnimationCurve>("AnimationCurve", c => EditorGUILayout.CurveField(c));
    AddListTypeOption<int>("int");
    AddListTypeOption<float>("float");
    AddListTypeOption<string>("string");

    // get all conditions
    var ctypes = Assembly.GetAssembly(typeof(GoalCondition))
      .GetTypes()
      .Where(t => typeof(GoalCondition).IsAssignableFrom(t) &&
                 !t.IsAbstract);
    _ConditionTypes = ctypes.ToArray();
    _ConditionTypeNames = _ConditionTypes.Select(x => x.Name.ToHumanFriendly()).ToArray();

    _ConditionIndices = new int[_ConditionTypeNames.Length];
    for (int i = 0; i < _ConditionIndices.Length; i++) {
      _ConditionIndices[i] = i;
    }

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
      if (newType == typeof(int)) {
        int intVal;
        if (int.TryParse((string)x, out intVal)) {
          return intVal;
        }
      }
      if (newType == typeof(float)) {
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
      if (newType == typeof(int)) {
        return (bool)x == true ? 1 : 0;
      }
      if (newType == typeof(float)) {
        return (bool)x == true ? 1f : 0f;
      }
    }

    if (newType == typeof(bool)) {
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

