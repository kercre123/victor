using UnityEngine;
using UnityEditor;
using System.Collections;
using Cozmo.UI;

[CustomEditor(typeof(TimeTextLabel))]
public class TimeTextLabelInspectory : Editor {
  public override void OnInspectorGUI() {
    base.OnInspectorGUI();
  }
}
