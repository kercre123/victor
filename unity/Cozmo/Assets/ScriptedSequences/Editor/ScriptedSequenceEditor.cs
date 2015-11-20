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

[CustomEditor(typeof(ScriptedSequence))]
public class ScriptedSequenceEditor : Editor {
  
  private static string[] _ConditionTypeNames;

  private static Type[] _ConditionTypes;

  private static int[] _ConditionIndices;

  private int _SelectedConditionIndex = 0;

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
                                     t != typeof(ScriptedSequenceCondition));
    _ConditionTypes = ctypes.ToArray();
    _ConditionTypeNames = _ConditionTypes.Select(x => x.Name).ToArray();

    _ConditionIndices = new int[_ConditionTypeNames.Length];
    for (int i = 0; i < _ConditionIndices.Length; i++) {
      _ConditionIndices[i] = i;
    }

    // get all actions
    var atypes = Assembly.GetAssembly(typeof(ScriptedSequenceAction))
                         .GetTypes()
                         .Where(t => typeof(ScriptedSequenceAction).IsAssignableFrom(t) && 
                                     t != typeof(ScriptedSequenceAction));
    _ActionTypes = atypes.ToArray();
    _ActionTypeNames = _ActionTypes.Select(x => x.Name).ToArray();

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



  private bool _EditingName = false;

  private Dictionary<uint, bool> _ExpandedNodes = new Dictionary<uint, bool>();

  private Dictionary<uint, bool> _EditingNodeNames = new Dictionary<uint, bool>();

  private int _DraggingNodeIndex = -1;

  private ScriptedSequenceHelper<ScriptedSequenceCondition> _DraggingConditionHelper;
  private ScriptedSequenceHelper<ScriptedSequenceAction> _DraggingActionHelper;

  public void SetDraggingHelper<T>(ScriptedSequenceHelper<T> val) where T : ScriptableObject
  {
    if(typeof(T) == typeof(ScriptedSequenceCondition))
    {
      _DraggingConditionHelper = (ScriptedSequenceHelper<ScriptedSequenceCondition>)(object)val;
    } else if(typeof(T) == typeof(ScriptedSequenceAction))
    {
      _DraggingActionHelper = (ScriptedSequenceHelper<ScriptedSequenceAction>)(object)val;
    }
  }

  public ScriptedSequenceHelper<T> GetDraggingHelper<T>() where T : ScriptableObject
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


  public Vector2 DragOffset;

  public Vector2 DragSize;

  public string DragTitle;

  public Color DragColor;
  public Color DragTextColor;

  public Vector2 DragStart;

  private bool _LastMouseUp;

  private Vector2 _LastMouseUpPosition;

  private Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>> _ConditionHelpers = new Dictionary<ScriptedSequenceCondition, ScriptedSequenceHelper<ScriptedSequenceCondition>>();
  private Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>> _ActionHelpers = new Dictionary<ScriptedSequenceAction, ScriptedSequenceHelper<ScriptedSequenceAction>>();

  private bool TryGetHelper<T>(T key, out ScriptedSequenceHelper<T> helper)  where T : ScriptableObject
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

  private void SetHelper<T>(T key, ScriptedSequenceHelper<T> helper)  where T : ScriptableObject
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

  private T ShowAddPopup<T>(Rect rect) where T : ScriptableObject {
    if (typeof(T) == typeof(ScriptedSequenceCondition)) {
      return ShowAddPopupInternal<T>(rect, ref _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices, _ConditionTypes);
    }
    else if (typeof(T) == typeof(ScriptedSequenceAction)) {
      return ShowAddPopupInternal<T>(rect, ref _SelectedActionIndex, _ActionTypeNames, _ActionIndices, _ActionTypes);
    }
    return null;
  }

  private T ShowAddPopupInternal<T>(Rect rect, ref int index, string[] names, int[] indices, Type[] types) where T : ScriptableObject {
    var popupRect = new Rect(rect.x, rect.y, rect.width - 50, rect.height);
    var plusRect = new Rect(rect.x + rect.width - 50, rect.y, 50, rect.height);
    index = EditorGUI.IntPopup(popupRect, index, names, indices);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      var result = (ScriptableObject.CreateInstance(types[index]) as T);
      result.hideFlags = HideFlags.HideInInspector;
      AssetDatabase.CreateAsset(result, Path.Combine(SequenceFolder, Guid.NewGuid().ToString() + ".asset"));
      return result;
    }
    return null;
  }

  private ScriptedSequenceNode _CopiedNode;

  private ScriptedSequenceCondition _CopiedCondition;
  private ScriptedSequenceAction _CopiedAction;

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


  public ScriptedSequenceNode CopyNode(ScriptedSequenceNode node) {
    ScriptedSequence a = ScriptedSequence.CreateInstance<ScriptedSequence>(); 
    ScriptedSequence b = ScriptedSequence.CreateInstance<ScriptedSequence>();
    a.Nodes.Add(node);   
    EditorUtility.CopySerialized(a, b); 
    return b.Nodes[0];
  }

  public T Copy<T>(T condition) where T : ScriptableObject {
    T copy = (T)ScriptableObject.CreateInstance(condition.GetType());
    EditorUtility.CopySerialized(condition, copy);
    return copy;
  }

  public static System.Type GetHelperType<T>(System.Type type) {
    Type returnType;
    if (!_HelperLookup.TryGetValue(type, out returnType)) {
      returnType = typeof(ScriptedSequenceHelper<,>).MakeGenericType(type, typeof(T));
    }
    return returnType;
  }

  private static GUIStyle _LabelStyle;
  public static GUIStyle LabelStyle
  {
    get
    {
      if (_LabelStyle == null) {
        _LabelStyle = new GUIStyle();
        _LabelStyle.normal.textColor = Color.white;
        _LabelStyle.active.textColor = Color.white;
      }
      return _LabelStyle;
    }
  }

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

  public string SequenceFolder {
    get {
      var path = AssetDatabase.GetAssetPath(target);

      var dir = Path.GetDirectoryName(path);
      var fileName = Path.GetFileNameWithoutExtension(path);

      string dirPath = Path.Combine(dir, fileName);
      if(!Directory.Exists(dirPath))
      {
        AssetDatabase.CreateFolder(dir, fileName);
      }
      return dirPath;
    }
  }

  public override void OnInspectorGUI() {

    var sequence = target as ScriptedSequence;

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
      sequence.Name = EditorGUI.TextField(leftRect, sequence.Name, LabelStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingName = false;
        EditorUtility.SetDirty(target);
      }
    }
    else {
      GUI.Label(titleRect, sequence.Name, LabelStyle);

      if (mouseEvent == EventType.ContextClick) {
        if (titleRect.Contains(mousePosition)) {
          _EditingName = true;
          EditorUtility.SetDirty(target);
        }
      }
    }

    DrawConditionOrActionEntry("Sequence Condition", sequence.Condition, (x) => {
      sequence.Condition = x;
    }, mousePosition, mouseEvent);


    GUILayout.Label("Sequence Nodes", LabelStyle);

    EditorGUI.indentLevel++;

    for (int i = 0; i < sequence.Nodes.Count; i++) {
      if (sequence.Nodes[i] == null) {
        sequence.Nodes.RemoveAt(i);
        i--;
        continue;
      }

      DrawScriptedSequenceNode(i, sequence.Nodes[i], sequence, mousePosition, mouseEvent);
    }

    var nextRect = EditorGUILayout.GetControlRect();

    var plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      AddNode(sequence.Nodes.Count);
    }


    EditorGUI.indentLevel--;

    if (mouseEvent == EventType.mouseUp) {
      if (!_LastMouseUp) {
        _LastMouseUp = true;
        _LastMouseUpPosition = mousePosition;
        EditorUtility.SetDirty(target);
      }
      else {
        _DraggingNodeIndex = -1;
        _DraggingConditionHelper = null;
        _DraggingActionHelper = null;
        _LastMouseUp = false;
      }
    }

    if ((_DraggingNodeIndex != -1 || _DraggingConditionHelper != null || _DraggingActionHelper != null) && !_LastMouseUp && Mathf.Abs(DragStart.y - mousePosition.y) > 5) {
      var lastColor = GUI.backgroundColor;
      var lastTextColor = GUI.color;
      GUI.backgroundColor = DragColor;
      GUI.contentColor = DragTextColor;
      GUI.Box(new Rect(evt.mousePosition + DragOffset, DragSize), "  "+DragTitle, BoxStyle);
      GUI.backgroundColor = lastColor;
      GUI.contentColor = lastTextColor;
      EditorUtility.SetDirty(target);
    }

    serializedObject.ApplyModifiedProperties();
  }

  private void AddNode(int index) {
    var sequence = target as ScriptedSequence;

    if (sequence == null) {
      return;
    }
    var id = GetNextId();
    sequence.Nodes.Insert(index, new ScriptedSequenceNode(){ Id = id, Name = "Node "+id });
  }

  private uint GetNextId()
  {
    var sequence = target as ScriptedSequence;

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

  private void RemoveNode(int index) {
    var sequence = target as ScriptedSequence;

    if (sequence == null) {
      return;
    }

    sequence.Nodes.RemoveAt(index);
  }

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
    GUI.color = Color.black;

    if (editingName) {
      var leftRect = rect;
      var rightRect = rect;
      leftRect.width -= 35;

      rightRect.x += leftRect.width;
      rightRect.width = 35;

      node.Name = EditorGUI.TextField(leftRect, node.Name, LabelStyle);
      if (GUI.Button(rightRect, "OK", ButtonStyle)) {
        _EditingNodeNames[node.Id] = false;
        EditorUtility.SetDirty(target);
      }
    }
    else {
      if (rect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Change Name"), false, () => {
            _EditingNodeNames[node.Id] = true;
            EditorUtility.SetDirty(target);
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
        else if (eventType == EventType.mouseDown) {
          _DraggingNodeIndex = index;
          DragOffset = rect.position - mousePosition;
          DragStart = mousePosition;
          DragSize = rect.size;
          DragTitle = node.Name;
          DragColor = Color.yellow;
          DragTextColor = Color.black;
        }
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
            EditorUtility.SetDirty(target);
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
    node.CheckPoint = EditorGUILayout.Toggle("Check Point", node.CheckPoint);

    DrawConditionOrActionList("Conditions", node.Conditions, mousePosition, eventType);

    DrawConditionOrActionList("Actions", node.Actions, mousePosition, eventType);
  }

  public void DrawConditionOrActionList<T>(string label, List<T> conditions, Vector2 mousePosition, EventType eventType) where T : ScriptableObject {    
    GUI.Label(GetIndentedLabelRect(), label, LabelStyle);

    for (int i = 0; i < conditions.Count; i++) {
      var cond = conditions[i];
      if (cond == null) {
        conditions.RemoveAt(i);
        i--;
        continue;
      }

      ScriptedSequenceHelper<T> helper = null;

      if (!TryGetHelper<T>(cond, out helper)) {
        helper = Activator.CreateInstance(GetHelperType<T>(cond.GetType()), cond, this, conditions) as ScriptedSequenceHelper<T>;
        SetHelper<T>(cond, helper);
      }

      helper.OnGUI(mousePosition, eventType);
    }

    var nextRect = EditorGUILayout.GetControlRect();

    var newObject = ShowAddPopup<T>(nextRect);

    if (newObject != default(T)) {
      conditions.Add(newObject);
    }
    var draggingHelper = GetDraggingHelper<T>();

    if (nextRect.Contains(mousePosition) && eventType == EventType.mouseUp && draggingHelper != null) {
      if (conditions != draggingHelper.List) {

        conditions.Add(draggingHelper.ValueBase);
        if (draggingHelper.ReplaceInsteadOfInsert) {
          draggingHelper.ReplaceAction(null);
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

  public Rect GetIndentedLabelRect()
  {
    var rect = EditorGUILayout.GetControlRect();
    rect.x += 15 * (EditorGUI.indentLevel - 1);
    rect.width -= 15 * (EditorGUI.indentLevel - 1);
    return rect;
  }

  public void DrawConditionOrActionEntry<T>(string label, T value, Action<T> setAction, Vector2 mousePosition, EventType eventType) where T : ScriptableObject {
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
        setAction(null);
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

      var draggingHelper = GetDraggingHelper<T>();
      if (nextRect.Contains(mousePosition) && eventType == EventType.mouseUp && draggingHelper != null) {
        value = draggingHelper.ValueBase;
        setAction(value);

        if (draggingHelper.ReplaceInsteadOfInsert) {
          draggingHelper.ReplaceAction(null);
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

  [MenuItem("Assets/Create/Cozmo/Scripted Sequence")]
  public static void CreateScriptedSequence()
  {
    var selected = Selection.objects.FirstOrDefault();

    var path = selected == null ? "Assets" : AssetDatabase.GetAssetPath(selected);

    path = Directory.Exists(path) ? path : Path.GetDirectoryName(path);

    var instance = ScriptableObject.CreateInstance<ScriptedSequence>();

    string fileNameBase = "NewSequence";

    int i = 0;
    string suffix = "";

    while(File.Exists(Path.Combine(path, fileNameBase + suffix + ".asset")))
    {
      i++;
      suffix = i.ToString();
    }
    instance.name = fileNameBase + suffix;

    AssetDatabase.CreateAsset(instance, Path.Combine(path, fileNameBase + suffix + ".asset"));
  }
}
