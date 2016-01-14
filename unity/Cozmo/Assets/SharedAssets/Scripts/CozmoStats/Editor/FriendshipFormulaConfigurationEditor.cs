using UnityEngine;
using System.Collections;
using UnityEditor;
using Anki.Cozmo;

[CustomEditor(typeof(FriendshipFormulaConfiguration))]
public class FriendshipFormulaConfigurationEditor : Editor {

  public override void OnInspectorGUI() {
    base.OnInspectorGUI();

    var config = (FriendshipFormulaConfiguration)target;

    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      config.Multipliers[i] = EditorGUILayout.Slider(((ProgressionStatType)i).ToString(), config.Multipliers[i], 0, 5);
    }
  }

}
