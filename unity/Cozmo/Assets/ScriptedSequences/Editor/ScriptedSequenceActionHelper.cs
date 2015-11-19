using System;
using UnityEngine;
using UnityEditor;
using System.Collections.Generic;

namespace ScriptedSequences.Editor {
  public abstract class ScriptedSequenceActionHelper {

    public readonly ScriptedSequenceAction ActionBase;

    protected ScriptedSequenceEditor _Editor;
    public List<ScriptedSequenceAction> _ActionList;

    protected bool _Expanded;

    protected Action _OnDestroy;

    public event Action OnDestroy { add { _OnDestroy += value; } remove { _OnDestroy -= value; } }

    public ScriptedSequenceActionHelper(ScriptedSequenceAction condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) {
      ActionBase = condition;
      _Editor = editor;
      _ActionList = list;
    }

    public abstract void OnGUI(Vector2 mousePosition, EventType eventType);

    public int Index { get { return _ActionList.IndexOf(ActionBase); } }

  }

  public class ScriptedSequenceActionHelper<T> : ScriptedSequenceActionHelper where T : ScriptedSequenceAction {

    T Action { get { return (T)ActionBase; } }


    public ScriptedSequenceActionHelper(T condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) : base(condition, editor, list)
    {
    }

    public override void OnGUI(Vector2 mousePosition, EventType eventType)
    {
      var rect = EditorGUILayout.GetControlRect();

      var lastColor = GUI.color;
      var lastBackgroundColor = GUI.backgroundColor;
      var lastContentColor = GUI.contentColor;

      GUI.color = Color.blue;
      GUI.contentColor = Color.white;
      GUI.backgroundColor = Color.blue;
      GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill);
      GUI.color = Color.white;

      if (rect.Contains(mousePosition)) {
        if (eventType == EventType.ContextClick) {

          var menu = new GenericMenu();

          menu.AddItem(new GUIContent("Copy"), false, () => {
            _Editor.CopiedAction = Action;                         
          });
          if (_Editor.CopiedAction != null) {
            menu.AddItem(new GUIContent("Paste"), false, () => {
              var newAction = _Editor.CopyAction(_Editor.CopiedAction);
              _ActionList.Insert(Index, newAction);
            });
          }
          else {
            menu.AddDisabledItem(new GUIContent("Paste"));
          }

          menu.AddItem(new GUIContent("Delete Action"), false, () => {
          
            _ActionList.RemoveAt(Index);

            if(_OnDestroy != null)
            {
              _OnDestroy();
            }

          });

          menu.ShowAsContext();
        }
        else if (eventType == EventType.mouseDown) {  
          _Editor._DraggingActionHelper = this;
          _Editor._DragOffset = rect.position - mousePosition;
          _Editor._DragStart = mousePosition;
          _Editor._DragSize = rect.size;
          _Editor._DragTitle = Action.GetType().Name;
          _Editor._DragColor = Color.blue;
        }
        else if (eventType == EventType.mouseUp) {
          var otherHelper = _Editor._DraggingActionHelper;

          if (otherHelper != null) {
            if (otherHelper != this) {

              if (_ActionList == otherHelper._ActionList) {
                int draggingIndex = otherHelper.Index;
                int index = Index;

                if (draggingIndex < index) {
                  var tmpNode = _ActionList[draggingIndex];

                  for (int i = draggingIndex; i < index; i++) {
                    _ActionList[i] = _ActionList[i + 1];
                  }
                  _ActionList[index] = tmpNode;
                }
                else if (draggingIndex > index) {
                  var tmpNode = _ActionList[draggingIndex];

                  for (int i = draggingIndex; i > index; i--) {
                    _ActionList[i] = _ActionList[i - 1];
                  }
                  _ActionList[index] = tmpNode;
                }
              }

              else {              
                // dragged from one list to another list. Just remove from that list and add to this one
                _ActionList.Insert(Index, otherHelper.ActionBase);
                otherHelper._ActionList = _ActionList;
              }
            }
            _Editor._DraggingActionHelper = null;
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

