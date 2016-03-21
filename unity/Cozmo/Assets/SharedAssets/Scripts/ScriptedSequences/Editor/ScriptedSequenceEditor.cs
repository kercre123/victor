using UnityEngine;
using System.Collections;
using UnityEditor;
using ScriptedSequences;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System;
using ScriptedSequences.Editor;
using System.Reflection;
using Newtonsoft.Json;

public class ScriptedSequenceEditor : EditorWindow {

  private const string kScriptedSequenceResourcesPath = "Assets/SharedAssets/Resources/ScriptedSequences";
  private const string kAutosaveFilePath = "Assets/SharedAssets/Scripts/ScriptedSequences/.WorkingFile.json~";
  private const string kColorsPath = "Assets/SharedAssets/Scripts/ScriptedSequences/ScriptedSequenceEditorColors.asset";

  public string CurrentSequenceFile;
  public ScriptedSequence CurrentSequence;

  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();

  private List<string> _UndoStack = new List<string>();

  private List<string> _RedoStack = new List<string>();

  private DateTime _LastSnapshotTime;

  private DateTime _SnapshotCountdownStart;

  private string _LastSnapshotState;

  private ScriptedSequenceEditorColors _Colors;


  // Required to display the Condition Select Popup
  private static string[] _ConditionTypeNames;
  private static Type[] _ConditionTypes;
  private static int[] _ConditionIndices;
  private int _SelectedConditionIndex = 0;

  // Required to display the Action Select Popup
  private static string[] _ActionTypeNames;
  private static Type[] _ActionTypes;
  private static int[] _ActionIndices;
  private int _SelectedActionIndex = 0;
  private static Dictionary<System.Type, System.Type> _HelperLookup;


  static ScriptedSequenceEditor() {
    // get all conditions
    var ctypes = Assembly.GetAssembly(typeof(ScriptedSequenceCondition))
                         .GetTypes()
                         .Where(t => typeof(ScriptedSequenceCondition).IsAssignableFrom(t) &&
                 !t.IsAbstract);
    _ConditionTypes = ctypes.ToArray();
    _ConditionTypeNames = _ConditionTypes.Select(x => x.Name.ToHumanFriendly()).ToArray();

    _ConditionIndices = new int[_ConditionTypeNames.Length];
    for (int i = 0; i < _ConditionIndices.Length; i++) {
      _ConditionIndices[i] = i;
    }

    // get all actions
    var atypes = Assembly.GetAssembly(typeof(ScriptedSequenceAction))
                         .GetTypes()
                         .Where(t => typeof(ScriptedSequenceAction).IsAssignableFrom(t) &&
                 !t.IsAbstract);
    _ActionTypes = atypes.ToArray();
    _ActionTypeNames = _ActionTypes.Select(x => x.Name.ToHumanFriendly()).ToArray();

    _ActionIndices = new int[_ActionTypeNames.Length];
    for (int i = 0; i < _ActionIndices.Length; i++) {
      _ActionIndices[i] = i;
    }

    // get all attributed helpers
    var htypes = Assembly.GetAssembly(typeof(ScriptedSequenceHelper<>))
                          .GetTypes()
      .Where(t => t.GetCustomAttributes(typeof(ScriptedSequenceHelperAttribute), false).Length > 0);

    // make a dictionary from the type in the attribute to the helper type
    _HelperLookup = htypes
      .ToDictionary(x => ((ScriptedSequenceHelperAttribute)x
                            .GetCustomAttributes(typeof(ScriptedSequenceHelperAttribute), false)[0]).Type,
      x => x);
  }
    
  // whether we should display the text field for the Sequence Name
  private bool _EditingName = false;

  // toggle state for node foldouts
  private Dictionary<uint, bool> _ExpandedNodes = new Dictionary<uint, bool>();

  // whether we are editing the name of each node
  private int _EditingNodeIndex = -1;

  // when dragging a node, this tells us which one
  private int _DraggingNodeIndex = -1;

  // when dragging a condition or action, this is how we identify it
  private ScriptedSequenceHelper<ScriptedSequenceCondition> _DraggingConditionHelper;
  private ScriptedSequenceHelper<ScriptedSequenceAction> _DraggingActionHelper;

  // generic way to get start dragging a condition or action
  public void SetDraggingHelper<T>(ScriptedSequenceHelper<T> val) where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      _DraggingConditionHelper = (ScriptedSequenceHelper<ScriptedSequenceCondition>)(object)val;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      _DraggingActionHelper = (ScriptedSequenceHelper<ScriptedSequenceAction>)(object)val;
    }
  }

  // generic way to get the dragging condition or action
  public ScriptedSequenceHelper<T> GetDraggingHelper<T>() where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return (ScriptedSequenceHelper<T>)(object)_DraggingConditionHelper;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return (ScriptedSequenceHelper<T>)(object)_DraggingActionHelper;
    }
    return null;
  }

  // generic way to get the color for condition or action
  public Color GetColor<T>() where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return _Colors.ConditionColor;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return _Colors.ActionColor;
    }
    return Color.black;
  }

  // generic way to get the text color for condition or action
  public Color GetTextColor<T>() where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return _Colors.ConditionTextColor;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return _Colors.ActionTextColor;
    }
    return Color.white;
  }

  // generic way to get the background color for condition or action
  public Color GetBackgroundColor<T>() where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return _Colors.ConditionBackgroundColor;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return _Colors.ActionBackgroundColor;
    }
    return Color.black;
  }

  // where on the field we started dragging
  public Vector2 DragOffset;

  // the size of the rect to drag
  public Vector2 DragSize;

  // the name to display in the drag box
  public string DragTitle;

  // the color of the drag box
  public Color DragColor;
  // the text color in the drag box
  public Color DragTextColor;

  // the position we started dragging from
  public Vector2 DragStart;

  // if we got a mouse up and need to wait for a Layout and Repaint to determine what it was over
  private bool _LastMouseUp;

  // the position of the last mouse up
  private Vector2 _LastMouseUpPosition;

  private Vector2 _LastMouseDownPosition;

  private DateTime _LastMouseDownTime;

  // if we get a mouse click when the context menu is open, we want to ignore it.
  public bool ContextMenuOpen;

  // dictionary from condition/action to the helper for that condition used for drawing controls
  private Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>> _ConditionHelpers = new Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>>();
  private Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>> _ActionHelpers = new Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>>();

  // generic way to retrieve the helper for a condition/action if it exists
  private bool TryGetHelper<T>(T key, out ScriptedSequenceHelper<T> helper)  where T : IScriptedSequenceItem {
    Dictionary<T, ScriptedSequenceHelper<T>> helpers = null;
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ConditionHelpers;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ActionHelpers;
    }
    if (helpers == null) {
      helper = null;
      return false;
    }
    else {
      return helpers.TryGetValue(key, out helper);
    }
  }

  // generic way to set the helper for a condition/action
  private void SetHelper<T>(T key, ScriptedSequenceHelper<T> helper)  where T : IScriptedSequenceItem {
    Dictionary<T, ScriptedSequenceHelper<T>> helpers = null;
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ConditionHelpers;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ActionHelpers;
    }
    if (helpers != null) {
      helpers[key] = helper;
    }
  }

  // shows a popup for a new condition or action, returns the created
  // condition/action on button press or null if button is not pressed
  private T ShowAddPopup<T>(Rect rect) where T : IScriptedSequenceItem {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return ShowAddPopupInternal<T>(rect, ref _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices, _ConditionTypes);
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return ShowAddPopupInternal<T>(rect, ref _SelectedActionIndex, _ActionTypeNames, _ActionIndices, _ActionTypes);
    }
    return default(T);
  }

  // internal function for ShowAddPopup, does the layout of the buttons and actual object creation
  private T ShowAddPopupInternal<T>(Rect rect, ref int index, string[] names, int[] indices, Type[] types) where T : IScriptedSequenceItem {
    var popupRect = new Rect(rect.x, rect.y, rect.width - 50, rect.height);
    var plusRect = new Rect(rect.x + rect.width - 50, rect.y, 50, rect.height);
    index = EditorGUI.IntPopup(popupRect, index, names, indices);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      var result = (T)(Activator.CreateInstance(types[index]));
      return result;
    }
    return default(T);
  }

  // if we copy a node, it is saved here. The actual copy is made when pasting
  private ScriptedSequenceNode _CopiedNode;

  // Copied Condition and Action
  private ScriptedSequenceCondition _CopiedCondition;
  private ScriptedSequenceAction _CopiedAction;

  // generic method to set the copied condition/action
  public void SetCopiedValue<T>(T val) {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      _CopiedCondition = (ScriptedSequenceCondition)(object)val;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      _CopiedAction = (ScriptedSequenceAction)(object)val;
    }
  }

  // generic method to get the copied condition/action
  public T GetCopiedValue<T>() {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return (T)(object)_CopiedCondition;
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return (T)(object)_CopiedAction;
    }
    return default(T);
  }

  // copy a node by creating a new sequence and using CopySerialized
  public ScriptedSequenceNode CopyNode(ScriptedSequenceNode node) {
    return JsonConvert.DeserializeObject<ScriptedSequenceNode>(
      JsonConvert.SerializeObject(node, 
        Formatting.None, 
        GlobalSerializerSettings.JsonSettings), 
      GlobalSerializerSettings.JsonSettings);
  }

  // copy a condition
  public T Copy<T>(T value) where T : IScriptedSequenceItem {
    T copy = (T)JsonConvert.DeserializeObject(
               JsonConvert.SerializeObject(value, 
                 Formatting.None, 
                 GlobalSerializerSettings.JsonSettings),
               value.GetType());
    return copy;
  }

  // Generic method to get the type we should use for a condition or action
  public static System.Type GetHelperType<T>(System.Type type) {
    Type returnType;
    if (!_HelperLookup.TryGetValue(type, out returnType)) {
      returnType = typeof(ScriptedSequenceHelper<,>).MakeGenericType(type, typeof(T));
    }
    return returnType;
  }

  // custom Unity Style for a label
  private static GUIStyle _LabelStyle;

  public static GUIStyle LabelStyle {
    get {
      if (_LabelStyle == null) {
        _LabelStyle = new GUIStyle();
        _LabelStyle.fontStyle = FontStyle.Bold;
        _LabelStyle.normal.textColor = Color.white;
        _LabelStyle.active.textColor = Color.white;
      }
      return _LabelStyle;
    }
  }

  // custom Unity Style for a label with large text
  private static GUIStyle _BigLabelStyle;

  public static GUIStyle BigLabelStyle {
    get {
      if (_BigLabelStyle == null) {
        _BigLabelStyle = new GUIStyle();
        _BigLabelStyle.fontStyle = FontStyle.Bold;
        _BigLabelStyle.normal.textColor = Color.white;
        _BigLabelStyle.active.textColor = Color.white;
        _BigLabelStyle.fontSize = 16;
      }
      return _BigLabelStyle;
    }
  }

  // custom Unity Style for a box
  private static GUIStyle _BoxStyle;

  public static GUIStyle BoxStyle {
    get {
      if (_BoxStyle == null) {
        _BoxStyle = new GUIStyle();
        _BoxStyle.normal.textColor = Color.white;
        _BoxStyle.active.textColor = Color.white;
        _BoxStyle.normal.background = Texture2D.whiteTexture;
      }
      return _BoxStyle;
    }
  }

  // custom Unity Style for a text field
  private static GUIStyle _TextFieldStyle;

  public static GUIStyle TextFieldStyle {
    get {
      if (_TextFieldStyle == null) {
        _TextFieldStyle = new GUIStyle(EditorStyles.textField);
        _TextFieldStyle.normal.textColor = Color.white;
        _TextFieldStyle.active.textColor = Color.white;

        _TextFieldStyle.normal.background = GUI.skin.textField.normal.background;
        _TextFieldStyle.active.background = GUI.skin.textField.active.background;
        _TextFieldStyle.hover.background = GUI.skin.textField.hover.background;
        _TextFieldStyle.focused.background = GUI.skin.textField.hover.background;
      }
      return _TextFieldStyle;
    }
  }

  // custom Unity Style for a button
  private static GUIStyle _ButtonStyle;

  public static GUIStyle ButtonStyle {
    get {
      if (_ButtonStyle == null) {
        _ButtonStyle = new GUIStyle(EditorStyles.miniButton);
        _ButtonStyle.normal.textColor = Color.white;
        _ButtonStyle.active.textColor = Color.white;
      }
      return _ButtonStyle;
    }
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

  private Vector2 _ScrollPosition;

  public void OnDestroy() {
    if (File.Exists(kAutosaveFilePath)) {
      File.Delete(kAutosaveFilePath);
    }
  }

  public void OnGUI() {

    DrawToolbar();

    _ScrollPosition = EditorGUILayout.BeginScrollView(_ScrollPosition);

    DrawSequenceEditor();

    EditorGUILayout.EndScrollView();

    UpdateUndoRedo();
  }

  private bool CheckDiscardUnsavedSequence() {
    bool canOpen = true;
    if (CurrentSequence != null && (string.IsNullOrEmpty(CurrentSequenceFile) || JsonConvert.SerializeObject(CurrentSequence, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(CurrentSequenceFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening a Sequence will Discard Unsaved Changes. Are you Sure?", "Yes");
    }
    return canOpen;
  }

  private void DrawToolbar() {

    if (CurrentSequence == null) {
      // Try Restoring Autosave
      if (File.Exists(kAutosaveFilePath)) {
        try {
          CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(File.ReadAllText(kAutosaveFilePath), GlobalSerializerSettings.JsonSettings);
        }
        catch {
        }
      }
    }

    if (_Colors == null) {
      if (File.Exists(kColorsPath)) {
        _Colors = AssetDatabase.LoadAssetAtPath<ScriptedSequenceEditorColors>(kColorsPath);
      }
      else {
        _Colors = ScriptableObject.CreateInstance<ScriptedSequenceEditorColors>();
        AssetDatabase.CreateAsset(_Colors, kColorsPath);
      }
    }

    EditorGUILayout.BeginHorizontal(ToolbarStyle);

    if (GUILayout.Button("Load", ToolbarButtonStyle)) {
      ContextMenuOpen = true;
      GenericMenu menu = new GenericMenu();

      foreach (var file in AssetDatabase.FindAssets(" t:TextAsset").Select(x => AssetDatabase.GUIDToAssetPath(x)).Where(x => x.Contains("ScriptedSequences") && x.EndsWith(".json"))) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            LoadFile(f);
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();
    }

    if (GUILayout.Button("Find and Load", ToolbarButtonStyle)) {
      if (CheckDiscardUnsavedSequence()) {
        string path = EditorUtility.OpenFilePanel("Open Sequence", "Assets", "json");

        if (!string.IsNullOrEmpty(path)) {
          LoadFile(path);
        }
      }
    }

    if (_RecentFiles.Count > 0 && GUILayout.Button("Load Recent", ToolbarButtonStyle)) {

      ContextMenuOpen = true;
      GenericMenu menu = new GenericMenu();

      foreach (var file in _RecentFiles) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            LoadFile(f);
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();
    }

    if (GUILayout.Button("New Sequence", ToolbarButtonStyle)) {
      if (CheckDiscardUnsavedSequence()) {
        _UndoStack.Clear();
        _RedoStack.Clear();
        _UndoStack.Add("{}");
        CurrentSequence = new ScriptedSequence();
        _EditingName = true;
        GUI.FocusControl("EditNameField");
        CurrentSequenceFile = null;
      }
    }


    if (CurrentSequence != null) {
      if (GUILayout.Button("Save", ToolbarButtonStyle)) {         
        if (string.IsNullOrEmpty(CurrentSequenceFile)) {
          CurrentSequenceFile = EditorUtility.SaveFilePanel("Save Sequence", kScriptedSequenceResourcesPath, CurrentSequence.Name, "json");
        }

        if (!string.IsNullOrEmpty(CurrentSequenceFile)) {       
          if (CurrentSequence.Nodes.Count == 0) {
            EditorUtility.DisplayDialog("Error!", "Cannot save a sequence with no Nodes!", "OK");
          }
          else {
            string msg = string.Empty;
            if (!CurrentSequence.Nodes.Any(x => x.Final)) {
              var last = CurrentSequence.Nodes.Last();
              last.Final = true;
              msg = "\u26A0 No Nodes Marked Final. Marking Node '" + last.Name + "' As Final Node.\n";
            }

            var invalidNodes = CurrentSequence.Nodes.Where(x => x.ExitConditions.Count == 0 && !x.ExitOnActionsComplete);

            if (invalidNodes.Any()) {
              foreach (var node in invalidNodes) {
                node.ExitOnActionsComplete = true;
                msg += "\u26A0 Node '" + node.Name + "' Had 'Exit On Actions Complete' unchecked, but no exit conditions. Fixing that for you.\n";
              }
            }

            _RecentFiles.Add(CurrentSequenceFile);

            File.WriteAllText(CurrentSequenceFile, JsonConvert.SerializeObject(CurrentSequence, Formatting.Indented, GlobalSerializerSettings.JsonSettings));

            AssetDatabase.ImportAsset(CurrentSequenceFile);
            EditorUtility.DisplayDialog("Save Successful!", msg + "Sequence '" + CurrentSequence.Name + "' has been saved to " + CurrentSequenceFile, "OK");
            // Clear out our temporary working file on save
            OnDestroy();
          }
        }
      }

      if (GUILayout.Button("Undo", ToolbarButtonStyle)) {
        Undo();
      }

      if (GUILayout.Button("Redo", ToolbarButtonStyle)) {
        Redo();
      }
    }

    GUILayout.FlexibleSpace();

    if (GUILayout.Button("Customize Colors", ToolbarButtonStyle)) {      
      Selection.activeObject = _Colors;
    }

    EditorGUILayout.EndHorizontal();
  }

  private void SaveTemporaryFile(string json) {    
    File.WriteAllText(kAutosaveFilePath, json);
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsavedSequence()) {
      try {
        CurrentSequence = null;
        CurrentSequenceFile = null;
        _UndoStack.Clear();
        _RedoStack.Clear();
        string json = File.ReadAllText(path);
        _UndoStack.Add(json);
        CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(json, GlobalSerializerSettings.JsonSettings);
        CurrentSequenceFile = path;
        _RecentFiles.Add(path);
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }
  }

  // draw the editor for the ScriptedSequence
  private void DrawSequenceEditor() {

    var sequence = CurrentSequence;

    var evt = Event.current;

    EventType mouseEvent = EventType.Used;
    Vector2 mousePosition = Vector2.zero;
    if (evt != null) {
      mouseEvent = evt.type;
      mousePosition = evt.mousePosition;
    }

    if (mouseEvent == EventType.MouseDown) {
      if (ContextMenuOpen) {
        mouseEvent = EventType.Used;
        ContextMenuOpen = false;
      }
    }

    // Theres a goofy issue where dragging down causes weirdness the frame we get Mouse Up.
    // Wait unitl the next repaint and things will be good
    if (_LastMouseUp && mouseEvent == EventType.Repaint) {
      mouseEvent = EventType.mouseUp;
      mousePosition = _LastMouseUpPosition;
    }

    bool mouseDown = mouseEvent == EventType.mouseDown;

    if (mouseEvent == EventType.keyUp) {
      if (Event.current.keyCode == KeyCode.Escape || Event.current.keyCode == KeyCode.Return) {
        _EditingName = false;
        _EditingNodeIndex = -1;
      }
    }

    if (sequence == null) {
      return;
    }

    // make sure Nodes is not null
    if (sequence.Nodes == null) {
      sequence.Nodes = new System.Collections.Generic.List<ScriptedSequenceNode>();
    }

    var titleRect = EditorGUILayout.GetControlRect();

    if (_EditingName) {
      var leftRect = titleRect;
      var rightRect = titleRect;
      leftRect.width -= 35;

      rightRect.x += leftRect.width;
      rightRect.width = 35;

      GUI.SetNextControlName("EditNameField");
      sequence.Name = EditorGUI.TextField(leftRect, sequence.Name, TextFieldStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingName = false;
      }
    }
    else {
      GUI.Label(titleRect, "Sequence: " + sequence.Name, BigLabelStyle);

      // if you right click the sequence name, it goes to edit mode
      if (mouseEvent == EventType.ContextClick) {
        if (titleRect.Contains(mousePosition)) {
          _EditingName = true;
          _EditingNodeIndex = -1;
          GUI.FocusControl("EditNameField");
        }
      }
      else if (mouseEvent == EventType.mouseDown) {
        if ((mousePosition - _LastMouseDownPosition).sqrMagnitude < 16 && DateTime.UtcNow < _LastMouseDownTime + new TimeSpan(250 * 10000)) {
          _EditingNodeIndex = -1;
          _EditingName = true;
        }
      }
    }

    EditorGUIUtility.labelWidth = 200;
    sequence.Repeatable = EditorGUILayout.Toggle(
      new GUIContent("Repeatable",
        "If this sequence can play multiple times"), 
      sequence.Repeatable);
    
    sequence.RequiresConditionRemainsMet = EditorGUILayout.Toggle(
      new GUIContent("Condition Must Stay Met",
        "If the condition becomes unmet while the sequence is running, the sequence will reset"), 
      sequence.RequiresConditionRemainsMet);
    
    sequence.ActivateOnConditionMet = EditorGUILayout.Toggle(
      new GUIContent("Activates On Condition Met", 
        "If this is true, the sequence will start playing as soon as the condition is met." +
        " Otherwise it must be started by name."), 
      sequence.ActivateOnConditionMet);
    EditorGUIUtility.labelWidth = 0;

    // draw the sequence condition
    DrawConditionOrActionEntry("Sequence Condition", sequence.Condition, (x) => {
      sequence.Condition = x;
    }, mousePosition, mouseEvent);


    GUILayout.Label("Sequence Nodes", BigLabelStyle);

    EditorGUI.indentLevel++;

    // Draw the nodes in the sequence
    for (int i = 0; i < sequence.Nodes.Count; i++) {
      if (sequence.Nodes[i] == null) {
        sequence.Nodes.RemoveAt(i);
        i--;
        continue;
      }

      DrawScriptedSequenceNode(i, sequence.Nodes[i], sequence, mousePosition, mouseEvent);
    }

    var nextRect = EditorGUILayout.GetControlRect();
    // plus button to add more nodes
    var plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      AddNode(sequence.Nodes.Count);
    }

    if (nextRect.Contains(mousePosition)) {
      if (mouseEvent == EventType.ContextClick) {
        if (_DraggingNodeIndex == -1) {
          ContextMenuOpen = true;
        }
        else {
          _DraggingNodeIndex = -1;
        }
        var menu = new GenericMenu();

        if (_CopiedNode != null) {
          menu.AddItem(new GUIContent("Paste"), false, () => {
            ContextMenuOpen = true;
            var newNode = CopyNode(_CopiedNode);
            newNode.Id = GetNextId();
            sequence.Nodes.Add(newNode);
          });
        }
        else {
          menu.AddDisabledItem(new GUIContent("Paste"));
        }

        menu.ShowAsContext();
      }
      if (_DraggingNodeIndex != -1 && mouseEvent == EventType.mouseUp) {
        var node = sequence.Nodes[_DraggingNodeIndex];
        sequence.Nodes.RemoveAt(_DraggingNodeIndex);
        sequence.Nodes.Add(node);
      }
    }

    EditorGUI.indentLevel--;

    // clear out anything being dragged on mouse up 
    //(if it was over something the drop would have been handled already)
    if (mouseEvent == EventType.mouseUp) {
      if (!_LastMouseUp) {
        _LastMouseUp = true;
        _LastMouseUpPosition = mousePosition;
      }
      else {
        _DraggingNodeIndex = -1;
        _DraggingConditionHelper = null;
        _DraggingActionHelper = null;
        _LastMouseUp = false;
      }
    }

    if (mouseDown) {
      _LastMouseDownTime = DateTime.UtcNow;
      _LastMouseDownPosition = mousePosition;
    }

    // draw a box so you can see what you are dragging.
    if ((_DraggingNodeIndex != -1 || _DraggingConditionHelper != null || _DraggingActionHelper != null) && !_LastMouseUp && Mathf.Abs(DragStart.y - mousePosition.y) > 5) {
      var lastColor = GUI.backgroundColor;
      var lastTextColor = GUI.color;
      GUI.backgroundColor = DragColor;
      GUI.contentColor = DragTextColor;
      GUI.Box(new Rect(evt.mousePosition + DragOffset, DragSize), "  " + DragTitle, BoxStyle);
      GUI.backgroundColor = lastColor;
      GUI.contentColor = lastTextColor;
    }

    Repaint();
  }

  private void UpdateUndoRedo() {

    if (CurrentSequence == null) {
      return;
    }

    if (Event.current.type == EventType.keyDown &&
        (Event.current.command || Event.current.control) &&
        Event.current.keyCode == KeyCode.Z) {

      if (Event.current.shift) { // redo
        Redo();
      }
      else { // undo
        Undo();
      }
      Event.current.Use();

      return;
    }

    if (DateTime.UtcNow < _LastSnapshotTime + new TimeSpan(0, 0, 1)) {
      return;
    }

    var currentState = JsonConvert.SerializeObject(CurrentSequence, Formatting.Indented, GlobalSerializerSettings.JsonSettings);

    if (_UndoStack.Count == 0) {
      _UndoStack.Add(currentState);
    }

    if (currentState == _UndoStack.Last()) {
      _SnapshotCountdownStart = default(DateTime);
      return;
    }

    if (currentState != _LastSnapshotState) {
      _SnapshotCountdownStart = DateTime.UtcNow;
      _LastSnapshotState = currentState;
      return;
    }

    if (DateTime.UtcNow < _SnapshotCountdownStart + new TimeSpan(0, 0, 1)) {
      return;
    }

    _SnapshotCountdownStart = default(DateTime);
    _LastSnapshotTime = DateTime.UtcNow;
    _LastSnapshotState = null;

    SaveTemporaryFile(currentState);

    _UndoStack.Add(currentState);
    _RedoStack.Clear();

    if (_UndoStack.Count > 100) {
      _UndoStack.RemoveAt(0);
    }
  }

  // function to insert a new node
  private void AddNode(int index) {
    var sequence = CurrentSequence;

    if (sequence == null) {
      return;
    }
    var id = GetNextId();
    sequence.Nodes.Insert(index, new ScriptedSequenceNode(){ Id = id, Name = "Node " + id });
  }

  private void Undo() {
    if (_UndoStack.Count > 1) {
      
      var current = _UndoStack[_UndoStack.Count - 2];
      _RedoStack.Add(_UndoStack.Last());
      _UndoStack.RemoveAt(_UndoStack.Count - 1);

      SaveTemporaryFile(current);

      _ActionHelpers.Clear();
      _ConditionHelpers.Clear();
      _DraggingConditionHelper = null;
      _DraggingActionHelper = null;
      _DraggingNodeIndex = -1;
      CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(current, GlobalSerializerSettings.JsonSettings);
    }
  }

  private void Redo() {
    if (_RedoStack.Count > 0) {          
      var current = _RedoStack.Last();
      _RedoStack.RemoveAt(_RedoStack.Count - 1);
      _UndoStack.Add(current);

      SaveTemporaryFile(current);

      _ActionHelpers.Clear();
      _ConditionHelpers.Clear();
      _DraggingConditionHelper = null;
      _DraggingActionHelper = null;
      _DraggingNodeIndex = -1;

      CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(current, GlobalSerializerSettings.JsonSettings);
    }
  }

  // get a uint 1 higher than the highest node's Id
  private uint GetNextId() {
    var sequence = CurrentSequence;

    if (sequence == null) {
      return 0;
    }

    uint maxId = 0;
    for (int i = 0; i < sequence.Nodes.Count; i++) {
      var node = sequence.Nodes[i];

      maxId = node.Id > maxId ? node.Id : maxId;
    }
    return maxId + 1;
  }

  // remove a node
  private void RemoveNode(int index) {
    var sequence = CurrentSequence;

    if (sequence == null) {
      return;
    }

    sequence.Nodes.RemoveAt(index);
  }

  // method to draw the controls for a node
  private void DrawScriptedSequenceNode(int index, ScriptedSequenceNode node, ScriptedSequence sequence, Vector2 mousePosition, EventType eventType) {

    bool expanded, editingName;

    _ExpandedNodes.TryGetValue(node.Id, out expanded);
    editingName = _EditingNodeIndex == index;

    var lastColor = GUI.color;
    var lastBackgroundColor = GUI.backgroundColor;
    var lastContentColor = GUI.contentColor;

    var bgColor = _Colors.NodeBackgroundColor;
    GUI.color = bgColor;
    EditorGUILayout.BeginVertical(BoxStyle);

    var rect = EditorGUILayout.GetControlRect();


    GUI.color = _Colors.NodeColor;
    GUI.contentColor = _Colors.NodeTextColor;
    GUI.backgroundColor = _Colors.NodeColor;
    GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill);

    if (editingName) {
      var leftRect = rect;
      var rightRect = rect;
      leftRect.width -= 35;

      rightRect.x += leftRect.width;
      rightRect.width = 35;

      GUI.contentColor = Color.white;
      GUI.SetNextControlName("EditNameField");
      node.Name = EditorGUI.TextField(leftRect, node.Name, TextFieldStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingNodeIndex = -1;
      }
      GUI.contentColor = Color.black;

      // sometimes it starts dragging when we change names. Cancel that.
      if (_DraggingNodeIndex == index) {
        _DraggingNodeIndex = -1;
      }
    }
    else {
      if (rect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          if (_DraggingNodeIndex == -1) {
            ContextMenuOpen = true;
          }
          else {
            _DraggingNodeIndex = -1;
          }
          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Change Name"), false, () => {
            ContextMenuOpen = true;
            _EditingNodeIndex = index;
            _EditingName = false;
            GUI.FocusControl("EditNameField");
          });

          menu.AddItem(new GUIContent("Copy"), false, () => {
            ContextMenuOpen = true;
            _CopiedNode = node;                         
          });
          if (_CopiedNode != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              ContextMenuOpen = true;
              var newNode = CopyNode(_CopiedNode);
              newNode.Id = GetNextId();
              sequence.Nodes.Insert(index, newNode);
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.AddItem(new GUIContent("Delete Node"), false, () => {
            ContextMenuOpen = true;
            sequence.Nodes.RemoveAt(index);
          });
            
          menu.ShowAsContext();
        }
        // handle drag start
        else if (eventType == EventType.mouseDown) {
          if ((mousePosition - _LastMouseDownPosition).sqrMagnitude < 16 && DateTime.UtcNow < _LastMouseDownTime + new TimeSpan(250 * 10000)) {
            _EditingNodeIndex = index;
            _EditingName = false;
          }
          else {
            _DraggingNodeIndex = index;
            DragOffset = rect.position - mousePosition;
            DragStart = mousePosition;
            DragSize = rect.size;
            DragTitle = node.Name;
            DragColor = _Colors.NodeColor;
            DragTextColor = _Colors.NodeTextColor;
          }
        }
        // handle drag end (drop)
        else if (eventType == EventType.mouseUp) {
          if (_DraggingNodeIndex != -1) {
            if (_DraggingNodeIndex < index) {
              var tmpNode = sequence.Nodes[_DraggingNodeIndex];

              for (int i = _DraggingNodeIndex; i < index; i++) {
                sequence.Nodes[i] = sequence.Nodes[i + 1];
              }
              sequence.Nodes[index] = tmpNode;
            }
            else if (_DraggingNodeIndex > index) {
              var tmpNode = sequence.Nodes[_DraggingNodeIndex];

              for (int i = _DraggingNodeIndex; i > index; i--) {
                sequence.Nodes[i] = sequence.Nodes[i - 1];
              }
              sequence.Nodes[index] = tmpNode;
            }
            _DraggingNodeIndex = -1;
          }
        }
      }
      bool expand = EditorGUI.Foldout(rect, expanded, node.Name, FoldoutStyle);
      if (expand != expanded) {
        _LastMouseDownTime = default(DateTime);
        expanded = expand;
      }
    }

    GUI.color = lastColor;
    GUI.backgroundColor = lastBackgroundColor;
    GUI.contentColor = lastContentColor;

    _ExpandedNodes[node.Id] = expanded;

    if (!expanded) {
      EditorGUILayout.EndVertical();
      return;
    }


    node.Sequencial = EditorGUILayout.Toggle(
      new GUIContent("Sequencial", 
        "If this is checked, this node will fire as soon as the node immediately above it is complete"), 
      node.Sequencial);
    node.Final = EditorGUILayout.Toggle(
      new GUIContent("Final",
        "The Sequence will end as soon as this node is complete if checked"), 
      node.Final);
    node.FailOnError = EditorGUILayout.Toggle(
      new GUIContent("Fail On Error", 
        "If this node fails, the sequence will end in a failure state. It will not be marked complete."), 
      node.FailOnError);

    DrawConditionOrActionList(
      new GUIContent("Conditions", 
        "A set of preconditions before the actions in this node will fire"), 
      node.Conditions, mousePosition, eventType);

    DrawConditionOrActionList(
      new GUIContent("Actions", 
        "The set of things this node will 'do'"), 
      node.Actions, mousePosition, eventType);

    EditorGUIUtility.labelWidth = 200;
    node.ExitOnActionsComplete = EditorGUILayout.Toggle(
      new GUIContent("Exit On Actions Complete", 
        "If checked, this node will exit as soon as the actions are complete regardless of any incomplete " +
        "exit conditions. If unchecked, exit conditions must be met to mark this node complete."),
      node.ExitOnActionsComplete);
    EditorGUIUtility.labelWidth = 0;

    DrawConditionOrActionList(
      new GUIContent("Exit Conditions", 
        "A set of post conditions that will mark this node complete. See 'Exit On Actions Complete' for more info."), 
      node.ExitConditions, mousePosition, eventType);

    EditorGUILayout.EndVertical();
    EditorGUILayout.GetControlRect();

  }

  // Draws a list of Conditions or Actions
  public void DrawConditionOrActionList<T>(GUIContent label, List<T> conditions, Vector2 mousePosition, EventType eventType) where T : IScriptedSequenceItem {    
    GUI.Label(GetIndentedLabelRect(), label, BigLabelStyle);
    for (int i = 0; i < conditions.Count; i++) {
      var cond = conditions[i];
      // clear out any null conditions
      if (cond == null) {
        conditions.RemoveAt(i);
        i--;
        continue;
      }

      ScriptedSequenceHelper<T> helper = null;

      // get the helper for this condition/action type
      if (!TryGetHelper<T>(cond, out helper)) {
        helper = Activator.CreateInstance(GetHelperType<T>(cond.GetType()), cond, this, conditions) as ScriptedSequenceHelper<T>;
        SetHelper<T>(cond, helper);
      }

      // use the helper to draw the gui for the item
      helper.OnGUI(mousePosition, eventType);
    }

    var nextRect = EditorGUILayout.GetControlRect();

    // show the add condition/action box at the bottom of the list
    var newObject = ShowAddPopup<T>(nextRect);

    if (!EqualityComparer<T>.Default.Equals(newObject, default(T))) {
      conditions.Add(newObject);
    }
    var draggingHelper = GetDraggingHelper<T>();

    // dropping on the add field inserts at the end
    if (nextRect.Contains(mousePosition)) {
      if (eventType == EventType.ContextClick) {

        if (GetDraggingHelper<T>() == null) {
          ContextMenuOpen = true;
        }
        else {
          SetDraggingHelper<T>(null);
        }
        var menu = new GenericMenu();

        if (GetCopiedValue<T>() != null) {
          menu.AddItem(new GUIContent("Paste"), false, () => {
            var newCondition = Copy(GetCopiedValue<T>());
            conditions.Add(newCondition);
          });
        }
        else {
          menu.AddDisabledItem(new GUIContent("Paste"));
        }

        menu.ShowAsContext();
      }
      else if (eventType == EventType.mouseUp && draggingHelper != null) {
        if (conditions != draggingHelper.List) {

          conditions.Add(draggingHelper.ValueBase);
          if (draggingHelper.ReplaceInsteadOfInsert) {
            draggingHelper.ReplaceAction(default(T));
            draggingHelper.ReplaceAction = null;
            draggingHelper.ReplaceInsteadOfInsert = false;
          }
          else {
            draggingHelper.List.RemoveAt(_DraggingConditionHelper.Index);
          }
          draggingHelper.List = conditions;
        }
        else {
          int oldIndex = _DraggingConditionHelper.Index;
          conditions.RemoveAt(oldIndex);
          conditions.Add(draggingHelper.ValueBase);
        }
      }
    }
  }

  // some things ignore indent. This is a work around
  public Rect GetIndentedLabelRect() {
    var rect = EditorGUILayout.GetControlRect();
    rect.x += 15 * (EditorGUI.indentLevel);
    rect.width -= 15 * (EditorGUI.indentLevel);
    return rect;
  }

  // If there is a field that is a condition/action, we can't use the list drawer. This gets around it using a setAction
  public void DrawConditionOrActionEntry<T>(string label, T value, Action<T> setAction, Vector2 mousePosition, EventType eventType) where T : IScriptedSequenceItem {
    GUI.Label(GetIndentedLabelRect(), label, BigLabelStyle);
    EditorGUI.indentLevel++;
    ScriptedSequenceHelper<T> helper = null;
    if (value != null) {
      if (!TryGetHelper<T>(value, out helper)) {        
        helper = Activator.CreateInstance(GetHelperType<T>(value.GetType()), value, this, setAction) as ScriptedSequenceHelper<T>;
        SetHelper<T>(value, helper);
      }

      helper.OnGUI(mousePosition, eventType);
    }
    else {
      var nextRect = EditorGUILayout.GetControlRect();

      var newObject = ShowAddPopup<T>(nextRect);
      if (newObject != null) {
        value = newObject;
        setAction(newObject);
      }

      // Dropping on a single field swaps out the field for the dragging one
      var draggingHelper = GetDraggingHelper<T>();
      if (nextRect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          if (GetDraggingHelper<T>() == null) {
            ContextMenuOpen = true;
          }
          else {
            SetDraggingHelper<T>(null);
          }
          var menu = new GenericMenu();

          if (GetCopiedValue<T>() != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              var newCondition = Copy(GetCopiedValue<T>());
              setAction(newCondition);
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.ShowAsContext();
        }
        else if (eventType == EventType.mouseUp && draggingHelper != null) {
          value = draggingHelper.ValueBase;
          setAction(value);

          if (draggingHelper.ReplaceInsteadOfInsert) {
            draggingHelper.ReplaceAction(default(T));
          }
          else {
            draggingHelper.List.RemoveAt(draggingHelper.Index);
            draggingHelper.List = null;
            draggingHelper.ReplaceInsteadOfInsert = true;
          }
          draggingHelper.ReplaceAction = setAction;
        }
      }
    }
    EditorGUI.indentLevel--;
  }

  [MenuItem("Cozmo/Sequences/Scripted Sequence Editor %k")]
  public static void OpenScriptedSequenceEditor() {
    ScriptedSequenceEditor window = (ScriptedSequenceEditor)EditorWindow.GetWindow(typeof(ScriptedSequenceEditor));
    window.titleContent = new GUIContent("Scripted Sequence Editor");
    window.Show();
  }

  [MenuItem("Cozmo/Sequences/Edit Sequence")]
  public static void EditSequence() {
    var selection = Selection.objects.FirstOrDefault() as TextAsset;
    if (selection != null) {
      ScriptedSequenceEditor window = (ScriptedSequenceEditor)EditorWindow.GetWindow(typeof(ScriptedSequenceEditor));
      window.titleContent = new GUIContent("Scripted Sequence Editor");
      window.Show();
      window.LoadFile(AssetDatabase.GetAssetPath(selection));
    }
  }
}
