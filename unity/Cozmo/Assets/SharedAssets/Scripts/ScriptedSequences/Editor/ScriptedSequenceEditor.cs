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

  public string CurrentSequenceFile;
  public ScriptedSequence CurrentSequence;

  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();


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


  static ScriptedSequenceEditor()
  {
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
  private Dictionary<uint, bool> _EditingNodeNames = new Dictionary<uint, bool>();

  // when dragging a node, this tells us which one
  private int _DraggingNodeIndex = -1;

  // when dragging a condition or action, this is how we identify it
  private ScriptedSequenceHelper<ScriptedSequenceCondition> _DraggingConditionHelper;
  private ScriptedSequenceHelper<ScriptedSequenceAction> _DraggingActionHelper;

  // generic way to get start dragging a condition or action
  public void SetDraggingHelper<T>(ScriptedSequenceHelper<T> val) where T : IScriptedSequenceItem
  {
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      _DraggingConditionHelper = (ScriptedSequenceHelper<ScriptedSequenceCondition>)(object)val;
    } else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      _DraggingActionHelper = (ScriptedSequenceHelper<ScriptedSequenceAction>)(object)val;
    }
  }

  // generic way to get the dragging condition or action
  public ScriptedSequenceHelper<T> GetDraggingHelper<T>() where T : IScriptedSequenceItem
  {
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      return (ScriptedSequenceHelper<T>)(object)_DraggingConditionHelper;
    } else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      return (ScriptedSequenceHelper<T>)(object)_DraggingActionHelper;
    }
    return null;
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

  // dictionary from condition/action to the helper for that condition used for drawing controls
  private Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>> _ConditionHelpers = new Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>>();
  private Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>> _ActionHelpers = new Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>>();

  // generic way to retrieve the helper for a condition/action if it exists
  private bool TryGetHelper<T>(T key, out ScriptedSequenceHelper<T> helper)  where T : IScriptedSequenceItem
  {
    Dictionary<T, ScriptedSequenceHelper<T>> helpers = null;
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ConditionHelpers;
    }
    else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ActionHelpers;
    }
    if(helpers == null) {
      helper = null;
      return false;
    } else {
      return helpers.TryGetValue(key, out helper);
    }
  }

  // generic way to set the helper for a condition/action
  private void SetHelper<T>(T key, ScriptedSequenceHelper<T> helper)  where T : IScriptedSequenceItem
  {
    Dictionary<T, ScriptedSequenceHelper<T>> helpers = null;
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ConditionHelpers;
    }
    else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      helpers = (Dictionary<T, ScriptedSequenceHelper<T>>)(object)_ActionHelpers;
    }
    if(helpers != null) {
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
  public void SetCopiedValue<T>(T val)
  {
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      _CopiedCondition = (ScriptedSequenceCondition)(object)val;
    } else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      _CopiedAction = (ScriptedSequenceAction)(object)val;
    }
  }

  // generic method to get the copied condition/action
  public T GetCopiedValue<T>()
  {
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      return (T)(object)_CopiedCondition;
    } else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      return (T)(object)_CopiedAction;
    }
    return default(T);
  }

  // copy a node by creating a new sequence and using CopySerialized
  public ScriptedSequenceNode CopyNode(ScriptedSequenceNode node) {
    return JsonConvert.DeserializeObject<ScriptedSequenceNode>(
          JsonConvert.SerializeObject(node, 
                                      Formatting.None, 
                                      ScriptedSequenceManager.JsonSettings), 
          ScriptedSequenceManager.JsonSettings);
  }

  // copy a condition
  public T Copy<T>(T value) where T : IScriptedSequenceItem {
    T copy = (T)JsonConvert.DeserializeObject(
          JsonConvert.SerializeObject(value, 
                                      Formatting.None, 
                                      ScriptedSequenceManager.JsonSettings),
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
  public static GUIStyle LabelStyle
  {
    get
    {
      if (_LabelStyle == null) {
        _LabelStyle = new GUIStyle();
        _LabelStyle.fontStyle = FontStyle.Bold;
        _LabelStyle.normal.textColor = Color.white;
        _LabelStyle.active.textColor = Color.white;
      }
      return _LabelStyle;
    }
  }

  // custom Unity Style for a box
  private static GUIStyle _BoxStyle;
  public static GUIStyle BoxStyle
  {
    get
    {
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
  public static GUIStyle TextFieldStyle
  {
    get
    {
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
  public static GUIStyle ButtonStyle
  {
    get
    {
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
  public static GUIStyle ToolbarButtonStyle
  {
    get
    {
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
  public static GUIStyle ToolbarStyle
  {
    get
    {
      if (_ToolbarStyle == null) {
        _ToolbarStyle = new GUIStyle(EditorStyles.toolbar);
        _ToolbarStyle.normal.textColor = Color.white;
        _ToolbarStyle.active.textColor = Color.white;
      }
      return _ToolbarStyle;
    }
  }

  private static GUIStyle _FoldoutStyle;
  public static GUIStyle FoldoutStyle
  {
    get
    {
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

  public void OnGUI()
  {

    DrawToolbar();

    _ScrollPosition = EditorGUILayout.BeginScrollView(_ScrollPosition);

    DrawSequenceEditor();

    EditorGUILayout.EndScrollView();
  }

  private bool CheckDiscardUnsavedSequence()
  {
    bool canOpen = true;
    if (CurrentSequence != null && (string.IsNullOrEmpty(CurrentSequenceFile) || JsonConvert.SerializeObject(CurrentSequence, Formatting.Indented, ScriptedSequenceManager.JsonSettings) != File.ReadAllText(CurrentSequenceFile)))
    {
      canOpen = EditorUtility.DisplayDialog("Warning","Opening a Sequence will Discard Unsaved Changes. Are you Sure?", "Yes");
    }
    return canOpen;
  }

  public void DrawToolbar() {

    EditorGUILayout.BeginHorizontal(ToolbarStyle);

    if (GUILayout.Button("Load", ToolbarButtonStyle)) {
      GenericMenu menu = new GenericMenu();

      foreach (var file in Directory.GetFiles(kScriptedSequenceResourcesPath, "*.json")) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            if (CheckDiscardUnsavedSequence()) {
              try {
                CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(File.ReadAllText(f), ScriptedSequenceManager.JsonSettings);
                CurrentSequenceFile = f;
                _RecentFiles.Add(f);
              }
              catch (Exception ex) {
                DAS.Error(this, ex.ToString());
              }
            }
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();
    }

    if (GUILayout.Button("Find and Load", ToolbarButtonStyle)) {
      if(CheckDiscardUnsavedSequence())
      {
        string path = EditorUtility.OpenFilePanel("Open Sequence", "Assets", "json");

        if(!string.IsNullOrEmpty(path))
        {
          try
          {
            CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(File.ReadAllText(path), ScriptedSequenceManager.JsonSettings);
            CurrentSequenceFile = path;
            _RecentFiles.Add(path);
          }
          catch(Exception ex) {
            DAS.Error(this, ex.ToString());
          }
        }
      }
    }

    if (_RecentFiles.Count > 0 && GUILayout.Button("Load Recent", ToolbarButtonStyle)) {

      GenericMenu menu = new GenericMenu();

      foreach (var file in _RecentFiles) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            if (CheckDiscardUnsavedSequence()) {
              try {
                CurrentSequence = JsonConvert.DeserializeObject<ScriptedSequence>(File.ReadAllText(f), ScriptedSequenceManager.JsonSettings);
                CurrentSequenceFile = f;
              }
              catch (Exception ex) {
                DAS.Error(this, ex.ToString());
              }
            }
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();

    }

    if (GUILayout.Button("New Sequence", ToolbarButtonStyle)) {
      if (CheckDiscardUnsavedSequence()) {
        CurrentSequence = new ScriptedSequence();
        _EditingName = true;
        CurrentSequenceFile = null;
      }
    }


    if (CurrentSequence != null && GUILayout.Button("Save", ToolbarButtonStyle)) {         
      if (string.IsNullOrEmpty(CurrentSequenceFile)) {
        CurrentSequenceFile = EditorUtility.SaveFilePanel("Save Sequence", kScriptedSequenceResourcesPath, CurrentSequence.Name, ".json");
      }

      if(!string.IsNullOrEmpty(CurrentSequenceFile))
      {
        _RecentFiles.Add(CurrentSequenceFile);
        File.WriteAllText(CurrentSequenceFile, JsonConvert.SerializeObject(CurrentSequence, Formatting.Indented, ScriptedSequenceManager.JsonSettings));

        EditorUtility.DisplayDialog("Save Successful!", "Sequence " + CurrentSequence.Name + " Has been saved to " + CurrentSequenceFile, "OK");
      }
    }

    GUILayout.FlexibleSpace();
    EditorGUILayout.EndHorizontal();
  }

  // draw the editor for the ScriptedSequence
  public void DrawSequenceEditor() {

    var sequence = CurrentSequence;

    var evt = Event.current;

    EventType mouseEvent = EventType.Used;
    Vector2 mousePosition = Vector2.zero;
    if (evt != null) {
      mouseEvent = evt.type;
      mousePosition = evt.mousePosition;
    }
    // Theres a goofy issue where dragging down causes weirdness the frame we get Mouse Up.
    // Wait unitl the next repaint and things will be good
    if (_LastMouseUp && mouseEvent == EventType.Repaint) {
      mouseEvent = EventType.mouseUp;
      mousePosition = _LastMouseUpPosition;
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
      sequence.Name = EditorGUI.TextField(leftRect, sequence.Name, TextFieldStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingName = false;
      }
    }
    else {
      GUI.Label(titleRect, "Sequence: "+sequence.Name, LabelStyle);

      // if you right click the sequence name, it goes to edit mode
      if (mouseEvent == EventType.ContextClick) {
        if (titleRect.Contains(mousePosition)) {
          _EditingName = true;
        }
      }
    }

    EditorGUIUtility.labelWidth = 200;
    sequence.Repeatable = EditorGUILayout.Toggle("Repeatable", sequence.Repeatable);
    sequence.RequiresConditionRemainsMet = EditorGUILayout.Toggle("Condition Must Stay Met", sequence.RequiresConditionRemainsMet);
    EditorGUIUtility.labelWidth = 0;

    // draw the sequence condition
    DrawConditionOrActionEntry("Sequence Condition", sequence.Condition, (x) => {
      sequence.Condition = x;
    }, mousePosition, mouseEvent);


    GUILayout.Label("Sequence Nodes", LabelStyle);

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

    if (nextRect.Contains(mousePosition) && _DraggingNodeIndex != -1) {
      if (mouseEvent == EventType.mouseUp) {
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
        Repaint();
      }
    }

    // draw a box so you can see what you are dragging.
    if ((_DraggingNodeIndex != -1 || _DraggingConditionHelper != null || _DraggingActionHelper != null) && !_LastMouseUp && Mathf.Abs(DragStart.y - mousePosition.y) > 5) {
      var lastColor = GUI.backgroundColor;
      var lastTextColor = GUI.color;
      GUI.backgroundColor = DragColor;
      GUI.contentColor = DragTextColor;
      GUI.Box(new Rect(evt.mousePosition + DragOffset, DragSize), "  "+DragTitle, BoxStyle);
      GUI.backgroundColor = lastColor;
      GUI.contentColor = lastTextColor;
      Repaint();
    }
  }

  // function to insert a new node
  private void AddNode(int index) {
    var sequence = CurrentSequence;

    if (sequence == null) {
      return;
    }
    var id = GetNextId();
    sequence.Nodes.Insert(index, new ScriptedSequenceNode(){ Id = id, Name = "Node "+id });
  }

  // get a uint 1 higher than the highest node's Id
  private uint GetNextId()
  {
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
    _EditingNodeNames.TryGetValue(node.Id, out editingName);


    var rect = EditorGUILayout.GetControlRect();

    var lastColor = GUI.color;
    var lastBackgroundColor = GUI.backgroundColor;
    var lastContentColor = GUI.contentColor;

    GUI.color = Color.yellow;
    GUI.contentColor = Color.black;
    GUI.backgroundColor = Color.yellow;
    GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill);

    if (editingName) {
      var leftRect = rect;
      var rightRect = rect;
      leftRect.width -= 35;

      rightRect.x += leftRect.width;
      rightRect.width = 35;

      GUI.contentColor = Color.white;
      node.Name = EditorGUI.TextField(leftRect, node.Name, TextFieldStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingNodeNames[node.Id] = false;
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

          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Change Name"), false, () => {
            _EditingNodeNames[node.Id] = true;
          });

          menu.AddItem(new GUIContent("Copy"), false, () => {
            _CopiedNode = node;                         
          });
          if (_CopiedNode != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              var newNode = CopyNode(_CopiedNode);
              newNode.Id = GetNextId();
              sequence.Nodes.Insert(index, newNode);
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.AddItem(new GUIContent("Delete Node"), false, () => {
            sequence.Nodes.RemoveAt(index);
          });

          menu.ShowAsContext();
        }
        // handle drag start
        else if (eventType == EventType.mouseDown) {
          _DraggingNodeIndex = index;
          DragOffset = rect.position - mousePosition;
          DragStart = mousePosition;
          DragSize = rect.size;
          DragTitle = node.Name;
          DragColor = Color.yellow;
          DragTextColor = Color.black;
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
      expanded = EditorGUI.Foldout(rect, expanded, node.Name, FoldoutStyle);
    }

    GUI.color = lastColor;
    GUI.backgroundColor = lastBackgroundColor;
    GUI.contentColor = lastContentColor;

    _ExpandedNodes[node.Id] = expanded;

    if (!expanded) {
      return;
    }

    node.Sequencial = EditorGUILayout.Toggle("Sequencial", node.Sequencial);
    node.Final = EditorGUILayout.Toggle("Final", node.Final);
    node.FailOnError = EditorGUILayout.Toggle("Fail On Error", node.FailOnError);

    DrawConditionOrActionList("Conditions", node.Conditions, mousePosition, eventType);

    DrawConditionOrActionList("Actions", node.Actions, mousePosition, eventType);
  }

  // Draws a list of Conditions or Actions
  public void DrawConditionOrActionList<T>(string label, List<T> conditions, Vector2 mousePosition, EventType eventType) where T : IScriptedSequenceItem {    
    GUI.Label(GetIndentedLabelRect(), label, LabelStyle);

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
    if (nextRect.Contains(mousePosition) && eventType == EventType.mouseUp && draggingHelper != null) {
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

  // some things ignore indent. This is a work around
  public Rect GetIndentedLabelRect()
  {
    var rect = EditorGUILayout.GetControlRect();
    rect.x += 15 * (EditorGUI.indentLevel);
    rect.width -= 15 * (EditorGUI.indentLevel);
    return rect;
  }

  // If there is a field that is a condition/action, we can't use the list drawer. This gets around it using a setAction
  public void DrawConditionOrActionEntry<T>(string label, T value, Action<T> setAction, Vector2 mousePosition, EventType eventType) where T : IScriptedSequenceItem {
    GUI.Label(GetIndentedLabelRect(), label, LabelStyle);
    EditorGUI.indentLevel++;
    ScriptedSequenceHelper<T> helper = null;
    if (value != null) {
      if (!TryGetHelper<T>(value, out helper)) {        
        helper = Activator.CreateInstance(GetHelperType<T>(value.GetType()), value, this, setAction) as ScriptedSequenceHelper<T>;
        SetHelper<T>(value, helper);
      }

      helper.OnGUI(mousePosition, eventType);

      if (GUILayout.Button("Clear", LabelStyle)) {
        setAction(default(T));
      }
    }
    else
    {
      var nextRect = EditorGUILayout.GetControlRect();

      var newObject = ShowAddPopup<T>(nextRect);
      if (newObject != null) {
        value = newObject;
        setAction(newObject);
      }

      // Dropping on a single field swaps out the field for the dragging one
      var draggingHelper = GetDraggingHelper<T>();
      if (nextRect.Contains(mousePosition) && eventType == EventType.mouseUp && draggingHelper != null) {
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
    EditorGUI.indentLevel--;
  }

  [MenuItem("Cozmo/Scripted Sequence Editor %t")]
  public static void CreateScriptedSequence()
  {
    ScriptedSequenceEditor window = (ScriptedSequenceEditor)EditorWindow.GetWindow (typeof (ScriptedSequenceEditor));
    window.titleContent = new GUIContent("Scripted Sequence Editor");
    window.Show();
  }
}
