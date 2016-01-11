using UnityEngine;
using System.Collections;
using UnityEditor;
using System;
using System.Collections.Generic;
using System.Linq;
using AnimationGroups;

[CustomEditor(typeof(AnimationGroup))]
public class AnimationGroupEditor : Editor {

  private static readonly string[] _Options;

  private static Dictionary<object, bool> _ExpandedFoldouts = new Dictionary<object, bool>();

  static AnimationGroupEditor()
  {
    _Options = typeof(AnimationName).GetFields(System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.Public).Where(f => f.FieldType == typeof(string)).Select(f => (string)f.GetValue(null)).ToArray();
  }

  public override void OnInspectorGUI() {
    var animGroup = (AnimationGroup)target;
    EditorGUI.BeginChangeCheck();

    DrawList("Animations", animGroup.Animations, DrawAnimationGroupEntry, () => new AnimationGroup.AnimationGroupEntry());

    if (EditorGUI.EndChangeCheck()) {
      EditorUtility.SetDirty(target);
    }
  }

  public void DrawAnimationGroupEntry(AnimationGroup.AnimationGroupEntry entry) {
    EditorGUILayout.BeginVertical();
    entry.Name = _Options[EditorGUILayout.Popup("Animation Name", Mathf.Max(0, Array.IndexOf(_Options, entry.Name)), _Options)];

    bool unfolded = false;
    _ExpandedFoldouts.TryGetValue(entry, out unfolded);
    unfolded = EditorGUILayout.Foldout(unfolded, "Mood Curves ("+entry.MoodCurves.Count+")");
    if (unfolded) {
      EditorGUI.indentLevel++;
      DrawList("", entry.MoodCurves, DrawMoodCurve, () => new AnimationGroup.AnimationGroupEntry.MoodCurve());
      EditorGUI.indentLevel--;
    }
    _ExpandedFoldouts[entry] = unfolded;

    EditorGUILayout.EndVertical();
  }

  public void DrawMoodCurve(AnimationGroup.AnimationGroupEntry.MoodCurve moodCurve) {
    EditorGUILayout.BeginVertical();

    moodCurve.Emotion = (Anki.Cozmo.EmotionType)EditorGUILayout.EnumPopup("Emotion", moodCurve.Emotion);

    moodCurve.Curve = EditorGUILayout.CurveField("Curve", moodCurve.Curve, Color.green, new Rect(-1, -1, 2, 2));

    EditorGUILayout.EndVertical();
  }

  public void DrawList<T>(string label, List<T> list, Action<T> drawControls, Func<T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc());
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      drawControls(list[i]);


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

}
