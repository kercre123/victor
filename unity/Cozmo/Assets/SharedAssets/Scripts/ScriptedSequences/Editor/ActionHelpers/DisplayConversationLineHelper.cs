using UnityEngine;
using System.Collections;
using ScriptedSequences.Actions;
using System.Collections.Generic;
using System;
using UnityEditor;

namespace ScriptedSequences.Editor.ActionHelpers {
  
  [ScriptedSequenceHelper(typeof(DisplayConversationLine))]
  public class DisplayConversationLineHelper : ScriptedSequenceHelper<DisplayConversationLine, ScriptedSequenceAction> {

    public DisplayConversationLineHelper(DisplayConversationLine condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) : base(condition, editor, list) {}
    public DisplayConversationLineHelper(DisplayConversationLine condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceAction> replaceAction) : base(condition, editor, replaceAction) {}

    public const string kLocalizationFile = "ConversationStrings";

    private static string[] sSideOptions = { "Left", "Right" };

    private string _Translation;

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {

      if (string.IsNullOrEmpty(Value.LineKey)) {
        string conversationName = _Editor.CurrentSequence.Name;
        var myNode = _Editor.CurrentSequence.Nodes.Find(x => x.Actions.Contains(Value));
        Value.LineKey = conversationName + "." + myNode.Name;

        int index = 0;
        while (LocalizationEditorUtility.KeyExists(kLocalizationFile, Value.LineKey)) {
          index++;
          Value.LineKey = conversationName + "." + myNode.Name + "." + index;
        }
      }


      if (_Translation == null) {
        _Translation = LocalizationEditorUtility.GetTranslation(kLocalizationFile, Value.LineKey);
      }

      Value.LineKey = EditorGUILayout.TextField("Translation Key", Value.LineKey);

      _Translation = EditorGUILayout.TextField("Translation", _Translation);

      if (_Translation != LocalizationEditorUtility.GetTranslation(kLocalizationFile, Value.LineKey)) {
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("Reload Translation")) {
          _Translation = LocalizationEditorUtility.GetTranslation(kLocalizationFile, Value.LineKey);
        }
        if (GUILayout.Button("Save Translation")) {
          LocalizationEditorUtility.SetTranslation(kLocalizationFile, Value.LineKey, _Translation);
        }
        EditorGUILayout.EndHorizontal();
      }

      Value.Speaker = (Conversations.Speaker)EditorGUILayout.EnumPopup("Speaker", Value.Speaker);

      Value.IsRight = EditorGUILayout.Popup("Side", Value.IsRight ? 1 : 0, sSideOptions) == 1;
    }
  }
}
