using System;
using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.Linq;

namespace ScriptedSequences.Editor {
  // Base class which has should have a generic argument of either
  // ScriptedSequenceCondition or ScriptedSequenceAction
  public abstract class ScriptedSequenceHelper<T> where T : ScriptableObject {

    public readonly T ValueBase;

    public Color Color { get { return typeof(T) == typeof(ScriptedSequenceCondition) ? Color.red : Color.blue; } }

    protected ScriptedSequenceEditor _Editor;
    public List<T> List;
    public bool ReplaceInsteadOfInsert = false;
    public Action<T> ReplaceAction;

    protected bool _Expanded;

    protected abstract bool _Expandable { get; }

    protected Action _OnDestroy;

    public event Action OnDestroy { add { _OnDestroy += value; } remove { _OnDestroy -= value; } }

    public ScriptedSequenceHelper(T condition, ScriptedSequenceEditor editor, List<T> list) {
      ValueBase = condition;
      _Editor = editor;
      List = list;
      ReplaceInsteadOfInsert = false;
      ReplaceAction = null;
    }

    public ScriptedSequenceHelper(T condition, ScriptedSequenceEditor editor, Action<T> replaceAction) {
      ValueBase = condition;
      _Editor = editor;
      List = null;
      ReplaceInsteadOfInsert = true;
      ReplaceAction = replaceAction;
    }

    public abstract void OnGUI(Vector2 mousePosition, EventType eventType);

    public int Index { get { return ReplaceInsteadOfInsert ? 0 : List.IndexOf(ValueBase); } }

  }

  // Generic class for a specific class of Condition or Action
  public class ScriptedSequenceHelper<T, U> : ScriptedSequenceHelper<U> where T : U  where U : ScriptableObject {

    public T Value { get { return (T)ValueBase; } }

    // By Default, we generate an editor for all fields that are int, float, string or bool
    // which lets us not write a custom editor for every single condition or action
    private System.Reflection.FieldInfo[] _Fields;
    private System.Reflection.FieldInfo[] Fields { 
      get
      {
        if(_Fields == null)
        {
          _Fields = typeof(T).GetFields(System.Reflection.BindingFlags.Public | 
                                        System.Reflection.BindingFlags.Instance)
                             .Where(x => x.FieldType == typeof(int) || 
                                         x.FieldType == typeof(float) || 
                                         x.FieldType == typeof(bool) || 
                                         x.FieldType == typeof(string))
                            .ToArray();
        }
        return _Fields;
      }
    }

    // Some conditions/actions don't have any parameters, so we don't need to draw the foldout
    protected override bool _Expandable {
      get {
        return Fields.Length > 0;
      }
    }

    // Constructors
    public ScriptedSequenceHelper(T condition, ScriptedSequenceEditor editor, List<U> list) : base(condition, editor, list) {}
    public ScriptedSequenceHelper(T condition, ScriptedSequenceEditor editor, Action<U> replaceAction) : base(condition, editor, replaceAction) {}


    // Function to draw the controls for this Condition/Action
    public override void OnGUI(Vector2 mousePosition, EventType eventType)
    {
      var rect = EditorGUILayout.GetControlRect();

      var lastColor = GUI.color;
      var lastBackgroundColor = GUI.backgroundColor;
      var lastContentColor = GUI.contentColor;

      GUI.color = Color;
      GUI.contentColor = Color.white;
      GUI.backgroundColor = Color;
      var textureRect = rect;
      textureRect.x += 15 * (EditorGUI.indentLevel - 1);
      textureRect.width -= 15 * (EditorGUI.indentLevel - 1);
      GUI.DrawTexture(textureRect, Texture2D.whiteTexture, ScaleMode.StretchToFill);
      GUI.color = Color.white;

      // Handle Right Click, Drag, or Drop
      if (rect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Copy"), false, () => {
            _Editor.SetCopiedValue<U>(Value);                         
          });
          if (_Editor.GetCopiedValue<U>() != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              var newCondition = _Editor.Copy(_Editor.GetCopiedValue<U>());
              if(ReplaceInsteadOfInsert)
              { 
                ReplaceAction(newCondition);
              }
              else
              {
                List.Insert(Index, newCondition);
              }
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.AddItem(new GUIContent("Delete"), false, () => {
            if(ReplaceInsteadOfInsert)
            {
              ReplaceAction(default(U));
            }
            else
            { 
              List.RemoveAt(Index);
            }
            if(_OnDestroy != null)
            {
              _OnDestroy();
            }

            AssetDatabase.DeleteAsset(AssetDatabase.GetAssetPath(Value));
          });

          menu.ShowAsContext();
        }
        else if (eventType == EventType.mouseDown) {  
          _Editor.SetDraggingHelper<U>(this);
          _Editor.DragOffset = rect.position - mousePosition;
          _Editor.DragStart = mousePosition;
          _Editor.DragSize = rect.size;
          _Editor.DragTitle = Value.GetType().Name;
          _Editor.DragColor = Color;
          _Editor.DragTextColor = Color.white;
        }
        else if (eventType == EventType.mouseUp) {
          var otherHelper = _Editor.GetDraggingHelper<U>();

          if (otherHelper != null) {
            if (otherHelper != this) {
              
              if (!ReplaceInsteadOfInsert && List == otherHelper.List) {
                int draggingIndex = otherHelper.Index;
                int index = Index;

                if (draggingIndex < index) {
                  var tmpNode = List[draggingIndex];

                  for (int i = draggingIndex; i < index; i++) {
                    List[i] = List[i + 1];
                  }
                  List[index] = tmpNode;
                }
                else if (draggingIndex > index) {
                  var tmpNode = List[draggingIndex];

                  for (int i = draggingIndex; i > index; i--) {
                    List[i] = List[i - 1];
                  }
                  List[index] = tmpNode;
                }
              }
              else if (otherHelper.ReplaceInsteadOfInsert) {
                if (ReplaceInsteadOfInsert) {
                  // dragged from a single point to a different single point. Swap them.
                  var tmp = Value;
                  ReplaceAction(otherHelper.ValueBase);
                  otherHelper.ReplaceAction(ValueBase);
                  // now swap the replace actions as the helpers need to point to the new thing
                  var tmpAction = ReplaceAction;
                  ReplaceAction = otherHelper.ReplaceAction;
                  otherHelper.ReplaceAction = tmpAction;
                }
                else {
                  // Dragged off of a single one into a list. Leave the single entry blank
                  List.Insert(Index, otherHelper.ValueBase);
                  otherHelper.ReplaceAction(default(U));
                  otherHelper.ReplaceAction = null;
                  otherHelper.ReplaceInsteadOfInsert = false;
                  otherHelper.List = List;
                }
              }
              else {
                if (ReplaceInsteadOfInsert) {
                  // dragged from a list onto a single point. Send this guy to the list
                  otherHelper.List.Insert(otherHelper.Index, Value);
                  otherHelper.List.Remove(otherHelper.ValueBase);
                  this.ReplaceAction(otherHelper.ValueBase);

                  ReplaceInsteadOfInsert = false;
                  List = otherHelper.List;
                  otherHelper.List = null;
                  otherHelper.ReplaceAction = ReplaceAction;
                  otherHelper.ReplaceInsteadOfInsert = true;
                  ReplaceAction = null;

                }
                else {
                  // dragged from one list to another list. Just remove from that list and add to this one
                  List.Insert(Index, otherHelper.ValueBase);
                  otherHelper.List = List;
                }
              }
            }

            _Editor.SetDraggingHelper<U>(null);
            EditorUtility.SetDirty(_Editor.target);
          }
        }
      }
      // if this condition/action is expandable, draw a foldout. Otherwise just draw a label
      if (_Expandable) {
        _Expanded = EditorGUI.Foldout(rect, _Expanded, typeof(T).Name, ScriptedSequenceEditor.FoldoutStyle);
      }
      else {
        EditorGUI.indentLevel++;
        GUI.Label(rect, typeof(T).Name, ScriptedSequenceEditor.LabelStyle);
        EditorGUI.indentLevel--;
      }

      GUI.color = lastColor;
      GUI.backgroundColor = lastBackgroundColor;
      GUI.contentColor = lastContentColor;

      // if this condition/action is expanded, Draw the Controls for it
      if (_Expanded) {
        EditorGUI.indentLevel++;
        DrawControls(mousePosition, eventType);
        EditorGUI.indentLevel--;
        EditorUtility.SetDirty(Value);
      }
    }

    // Default Drawer using Reflection
    protected virtual void DrawControls(Vector2 mousePosition, EventType eventType) {

      var fields = Fields;

      for (int i = 0; i < fields.Length; i++) {
        var field = fields[i];

        if (field.FieldType == typeof(int)) {
          field.SetValue(Value, EditorGUILayout.IntField(field.Name, (int)field.GetValue(Value)));
        }
        else if (field.FieldType == typeof(float)) {
          field.SetValue(Value, EditorGUILayout.FloatField(field.Name, (float)field.GetValue(Value)));
        }
        else if (field.FieldType == typeof(bool)) {
          field.SetValue(Value, EditorGUILayout.Toggle(field.Name, (bool)field.GetValue(Value)));
        }
        else if (field.FieldType == typeof(string)) {
          string oldVal = (string)field.GetValue(Value) ?? string.Empty;
          field.SetValue(Value, EditorGUILayout.TextField(field.Name, oldVal));
        }
      }
    }
  }

  // An Attribute used to mark the editor you want to use for an Action/Condition
  public class ScriptedSequenceHelperAttribute : Attribute {

    public readonly Type Type;
    public ScriptedSequenceHelperAttribute(System.Type type) {
      Type = type;
    }
  }
}

