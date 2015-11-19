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


  static ScriptedSequenceEditor()
  {
    var ctypes = Assembly.GetAssembly(typeof(ScriptedSequenceCondition)).GetTypes().Where(t => typeof(ScriptedSequenceCondition).IsAssignableFrom(t) && t != typeof(ScriptedSequenceCondition));
    _ConditionTypes = ctypes.ToArray();
    _ConditionTypeNames = _ConditionTypes.Select(x => x.Name).ToArray();

    _ConditionIndices = new int[_ConditionTypeNames.Length];
    for (int i = 0; i < _ConditionIndices.Length; i++) {
      _ConditionIndices[i] = i;
    }

    var atypes = Assembly.GetAssembly(typeof(ScriptedSequenceAction)).GetTypes().Where(t => typeof(ScriptedSequenceAction).IsAssignableFrom(t) && t != typeof(ScriptedSequenceAction));
    _ActionTypes = atypes.ToArray();
    _ActionTypeNames = _ActionTypes.Select(x => x.Name).ToArray();

    _ActionIndices = new int[_ActionTypeNames.Length];
    for (int i = 0; i < _ActionIndices.Length; i++) {
      _ActionIndices[i] = i;
    }
  }


  private bool _EditingName = false;

  private Dictionary<uint, bool> _ExpandedNodes = new Dictionary<uint, bool>();

  private Dictionary<uint, bool> _EditingNodeNames = new Dictionary<uint, bool>();

  private int _DraggingNodeIndex = -1;

  public ScriptedSequenceConditionHelper _DraggingConditionHelper;

  public ScriptedSequenceActionHelper _DraggingActionHelper;

  public Vector2 _DragOffset;

  public Vector2 _DragSize;

  public string _DragTitle;

  public Color _DragColor;

  public Vector2 _DragStart;

  private bool _LastMouseUp;

  private Vector2 _LastMouseUpPosition;

  private Dictionary<ScriptedSequenceCondition, ScriptedSequenceConditionHelper> _ConditionHelpers = new Dictionary<ScriptedSequenceCondition, ScriptedSequenceConditionHelper>();

  private Dictionary<ScriptedSequenceAction, ScriptedSequenceActionHelper> _ActionHelpers = new Dictionary<ScriptedSequenceAction, ScriptedSequenceActionHelper>();

  private ScriptedSequenceNode _CopiedNode;

  public ScriptedSequenceCondition CopiedCondition;

  public ScriptedSequenceAction CopiedAction;

  public ScriptedSequenceNode CopyNode(ScriptedSequenceNode node) {
    ScriptedSequence a = ScriptedSequence.CreateInstance<ScriptedSequence>(); 
    ScriptedSequence b = ScriptedSequence.CreateInstance<ScriptedSequence>();
    a.Nodes.Add(node);   
    EditorUtility.CopySerialized(a, b); 
    return b.Nodes[0];
  }

  public ScriptedSequenceCondition CopyCondition(ScriptedSequenceCondition condition) {
    ScriptedSequenceCondition copy = ScriptableObject.CreateInstance(condition.GetType()) as ScriptedSequenceCondition;
    EditorUtility.CopySerialized(condition, copy);
    return copy;
  }

  public ScriptedSequenceAction CopyAction(ScriptedSequenceAction action) {
    ScriptedSequenceAction copy = ScriptableObject.CreateInstance(action.GetType()) as ScriptedSequenceAction;
    EditorUtility.CopySerialized(action, copy);
    return copy;
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
        _FoldoutStyle.normal.textColor = Color.white;
        _FoldoutStyle.active.textColor = Color.white;
      }
      return _FoldoutStyle;
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

    GUILayout.Label("Sequence Condition", LabelStyle);
    EditorGUI.indentLevel++;
    ScriptedSequenceConditionHelper helper = null;
    Rect nextRect, plusRect;
    if (sequence.Condition != null) {

      if (!_ConditionHelpers.TryGetValue(sequence.Condition, out helper)) {
        Action<ScriptedSequenceCondition> setAction = (ScriptedSequenceCondition x) => {
          sequence.Condition = x;
        };
        helper = Activator.CreateInstance(typeof(ScriptedSequenceConditionHelper<>).MakeGenericType(sequence.Condition.GetType()), sequence.Condition, this, setAction) as ScriptedSequenceConditionHelper;
        _ConditionHelpers[sequence.Condition] = helper;
      }

      helper.OnGUI(mousePosition, mouseEvent);

      if (GUILayout.Button("Clear Condition", LabelStyle)) {
        sequence.Condition = null;
      }
    }
    else
    {
      nextRect = EditorGUILayout.GetControlRect();

      var popupRect = new Rect(nextRect.x, nextRect.y, nextRect.width - 50, nextRect.height);
      plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

      _SelectedConditionIndex = EditorGUI.IntPopup(popupRect, _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices);

      if (GUI.Button(plusRect, "+", ButtonStyle)) {
        sequence.Condition = (ScriptableObject.CreateInstance(_ConditionTypes[_SelectedConditionIndex]) as ScriptedSequenceCondition);
      }

      if (nextRect.Contains(mousePosition) && mouseEvent == EventType.mouseUp && _DraggingConditionHelper != null) {
        sequence.Condition = _DraggingConditionHelper.ConditionBase;
        if (_DraggingConditionHelper._ReplaceInsteadOfInsert) {
          _DraggingConditionHelper._ReplaceAction(null);
        }
        else {
          _DraggingConditionHelper._ConditionList.RemoveAt(_DraggingConditionHelper.Index);
          _DraggingConditionHelper._ConditionList = null;
          _DraggingConditionHelper._ReplaceInsteadOfInsert = true;
        }
        _DraggingConditionHelper._ReplaceAction = x => {
          sequence.Condition = x;
        };
      }
    }
    EditorGUI.indentLevel--;


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

    nextRect = EditorGUILayout.GetControlRect();

    plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

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

    if ((_DraggingNodeIndex != -1 || _DraggingConditionHelper != null || _DraggingActionHelper != null) && !_LastMouseUp && (_DragStart - mousePosition).sqrMagnitude > 25) {
      var lastColor = GUI.backgroundColor;
      GUI.backgroundColor = _DragColor;
      GUI.Box(new Rect(evt.mousePosition + _DragOffset, _DragSize), _DragTitle, LabelStyle);
      GUI.backgroundColor = lastColor;
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
    GUI.contentColor = Color.white;
    GUI.backgroundColor = Color.yellow;
    GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill);
    GUI.color = Color.white;

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
          _DragOffset = rect.position - mousePosition;
          _DragStart = mousePosition;
          _DragSize = rect.size;
          _DragTitle = node.Name;
          _DragColor = Color.yellow;
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

    GUILayout.Label("Conditions", LabelStyle);

    for (int i = 0; i < node.Conditions.Count; i++) {
      var cond = node.Conditions[i];
      if (cond == null) {
        node.Conditions.RemoveAt(i);
        i--;
        continue;
      }

      ScriptedSequenceConditionHelper helper = null;

      if (!_ConditionHelpers.TryGetValue(cond, out helper)) {
        helper = Activator.CreateInstance(typeof(ScriptedSequenceConditionHelper<>).MakeGenericType(cond.GetType()), cond, this, node.Conditions) as ScriptedSequenceConditionHelper;
        _ConditionHelpers[cond] = helper;
      }

      helper.OnGUI(mousePosition, eventType);
    }

    var nextRect = EditorGUILayout.GetControlRect();

    var popupRect = new Rect(nextRect.x, nextRect.y, nextRect.width - 50, nextRect.height);
    var plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

    _SelectedConditionIndex = EditorGUI.IntPopup(popupRect, _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      node.Conditions.Add(ScriptableObject.CreateInstance(_ConditionTypes[_SelectedConditionIndex]) as ScriptedSequenceCondition);
    }

    GUILayout.Label("Actions", LabelStyle);

    for (int i = 0; i < node.Actions.Count; i++) {
      var cond = node.Actions[i];
      if (cond == null) {
        node.Actions.RemoveAt(i);
        i--;
        continue;
      }

      ScriptedSequenceActionHelper helper = null;

      if (!_ActionHelpers.TryGetValue(cond, out helper)) {
        helper = Activator.CreateInstance(typeof(ScriptedSequenceActionHelper<>).MakeGenericType(cond.GetType()), cond, this, node.Actions) as ScriptedSequenceActionHelper;
        _ActionHelpers[cond] = helper;
      }

      helper.OnGUI(mousePosition, eventType);
    }

    nextRect = EditorGUILayout.GetControlRect();

    popupRect = new Rect(nextRect.x, nextRect.y, nextRect.width - 50, nextRect.height);
    plusRect = new Rect(nextRect.x + nextRect.width - 50, nextRect.y, 50, nextRect.height);

    _SelectedActionIndex = EditorGUI.IntPopup(popupRect, _SelectedActionIndex, _ActionTypeNames, _ActionIndices);

    if (GUI.Button(plusRect, "+", ButtonStyle)) {
      node.Actions.Add(ScriptableObject.CreateInstance(_ActionTypes[_SelectedActionIndex]) as ScriptedSequenceAction);
    }
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
