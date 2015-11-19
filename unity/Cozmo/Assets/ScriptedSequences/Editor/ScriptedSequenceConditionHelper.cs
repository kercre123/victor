using System;
using UnityEngine;
using UnityEditor;
using System.Collections.Generic;

namespace ScriptedSequences.Editor {
  public abstract class ScriptedSequenceConditionHelper {

    public readonly ScriptedSequenceCondition ConditionBase;

    protected ScriptedSequenceEditor _Editor;
    public List<ScriptedSequenceCondition> _ConditionList;
    public bool _ReplaceInsteadOfInsert = false;
    public Action<ScriptedSequenceCondition> _ReplaceAction;

    protected bool _Expanded;

    protected Action _OnDestroy;

    public event Action OnDestroy { add { _OnDestroy += value; } remove { _OnDestroy -= value; } }

    public ScriptedSequenceConditionHelper(ScriptedSequenceCondition condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) {
      ConditionBase = condition;
      _Editor = editor;
      _ConditionList = list;
      _ReplaceInsteadOfInsert = false;
      _ReplaceAction = null;
    }

    public ScriptedSequenceConditionHelper(ScriptedSequenceCondition condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) {
      ConditionBase = condition;
      _Editor = editor;
      _ConditionList = null;
      _ReplaceInsteadOfInsert = true;
      _ReplaceAction = replaceAction;
    }

    public abstract void OnGUI(Vector2 mousePosition, EventType eventType);

    public int Index { get { return _ReplaceInsteadOfInsert ? 0 : _ConditionList.IndexOf(ConditionBase); } }

  }

  public class ScriptedSequenceConditionHelper<T> : ScriptedSequenceConditionHelper where T : ScriptedSequenceCondition {

    T Condition { get { return (T)ConditionBase; } }


    public ScriptedSequenceConditionHelper(T condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list)
    {
    }

    public ScriptedSequenceConditionHelper(T condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) : base(condition, editor, replaceAction)
    {
    }


    public override void OnGUI(Vector2 mousePosition, EventType eventType)
    {
      var rect = EditorGUILayout.GetControlRect();

      var lastColor = GUI.color;
      var lastBackgroundColor = GUI.backgroundColor;
      var lastContentColor = GUI.contentColor;

      GUI.color = Color.red;
      GUI.contentColor = Color.white;
      GUI.backgroundColor = Color.red;
      GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill);
      GUI.color = Color.white;

      if (rect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Copy"), false, () => {
            _Editor.CopiedCondition = Condition;                         
          });
          if (_Editor.CopiedCondition != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              var newCondition = _Editor.CopyCondition(_Editor.CopiedCondition);
              if(_ReplaceInsteadOfInsert)
              { 
                _ReplaceAction(newCondition);
              }
              else
              {
                _ConditionList.Insert(Index, newCondition);
              }
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.AddItem(new GUIContent("Delete Condition"), false, () => {
            if(_ReplaceInsteadOfInsert)
            {
              _ReplaceAction(null);
            }
            else
            { 
              _ConditionList.RemoveAt(Index);

              if(_OnDestroy != null)
              {
                _OnDestroy();
              }
            }
          });

          menu.ShowAsContext();
        }
        else if (eventType == EventType.mouseDown) {  
          _Editor._DraggingConditionHelper = this;
          _Editor._DragOffset = rect.position - mousePosition;
          _Editor._DragStart = mousePosition;
          _Editor._DragSize = rect.size;
          _Editor._DragTitle = Condition.GetType().Name;
          _Editor._DragColor = Color.red;
        }
        else if (eventType == EventType.mouseUp) {
          var otherHelper = _Editor._DraggingConditionHelper;

          if (otherHelper != null) {
            if (otherHelper != this) {
              
              if (!_ReplaceInsteadOfInsert && _ConditionList == otherHelper._ConditionList) {
                int draggingIndex = otherHelper.Index;
                int index = Index;

                if (draggingIndex < index) {
                  var tmpNode = _ConditionList[draggingIndex];

                  for (int i = draggingIndex; i < index; i++) {
                    _ConditionList[i] = _ConditionList[i + 1];
                  }
                  _ConditionList[index] = tmpNode;
                }
                else if (draggingIndex > index) {
                  var tmpNode = _ConditionList[draggingIndex];

                  for (int i = draggingIndex; i > index; i--) {
                    _ConditionList[i] = _ConditionList[i - 1];
                  }
                  _ConditionList[index] = tmpNode;
                }
              }
              else if (otherHelper._ReplaceInsteadOfInsert) {
                if (_ReplaceInsteadOfInsert) {
                  // dragged from a single point to a different single point. Swap them.
                  var tmp = Condition;
                  _ReplaceAction(otherHelper.ConditionBase);
                  otherHelper._ReplaceAction(ConditionBase);
                  // now swap the replace actions as the helpers need to point to the new thing
                  var tmpAction = _ReplaceAction;
                  _ReplaceAction = otherHelper._ReplaceAction;
                  otherHelper._ReplaceAction = tmpAction;
                }
                else {
                  // Dragged off of a single one into a list. Leave the single entry blank
                  _ConditionList.Insert(Index, otherHelper.ConditionBase);
                  otherHelper._ReplaceAction(null);
                  otherHelper._ReplaceAction = null;
                  otherHelper._ReplaceInsteadOfInsert = false;
                  otherHelper._ConditionList = _ConditionList;
                }
              }
              else {
                if (_ReplaceInsteadOfInsert) {
                  // dragged from a list onto a single point. Send this guy to the list
                  otherHelper._ConditionList.Insert(otherHelper.Index, Condition);
                  this._ReplaceAction(otherHelper.ConditionBase);

                  _ReplaceInsteadOfInsert = false;
                  _ConditionList = otherHelper._ConditionList;
                  otherHelper._ConditionList = null;
                  otherHelper._ReplaceAction = _ReplaceAction;
                  otherHelper._ReplaceInsteadOfInsert = true;
                  _ReplaceAction = null;

                }
                else {
                  // dragged from one list to another list. Just remove from that list and add to this one
                  _ConditionList.Insert(Index, otherHelper.ConditionBase);
                  otherHelper._ConditionList = _ConditionList;
                }
              }
            }

            _Editor._DraggingConditionHelper = null;
            EditorUtility.SetDirty(_Editor.target);
          }
        }
      }
      _Expanded = EditorGUI.Foldout(rect, _Expanded, typeof(T).Name, ScriptedSequenceEditor.FoldoutStyle);

      GUI.color = lastColor;
      GUI.backgroundColor = lastBackgroundColor;
      GUI.contentColor = lastContentColor;

      if (_Expanded) {
        DrawControls();
      }
    }

    protected void DrawControls() {
    }
  }
}

