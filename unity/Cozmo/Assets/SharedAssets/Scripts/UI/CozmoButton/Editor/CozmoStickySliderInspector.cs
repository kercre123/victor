using UnityEngine;
using UnityEditor;

[CustomEditor(typeof(Cozmo.UI.CozmoStickySlider))]
public class CozmoStickySliderInspector : UnityEditor.UI.SliderEditor {
  private SerializedProperty restingSliderValueProp;
  private SerializedProperty sliderValueDecayRateProp;

  protected override void OnEnable() {
    base.OnEnable();
    restingSliderValueProp = serializedObject.FindProperty("_RestingSliderValue");
    sliderValueDecayRateProp = serializedObject.FindProperty("_SliderValueDecayRate");
  }

  override public void OnInspectorGUI() {
    base.OnInspectorGUI();
    serializedObject.Update();

    EditorGUILayout.PropertyField(restingSliderValueProp);
    EditorGUILayout.PropertyField(sliderValueDecayRateProp);

    if (GUI.changed) {
      serializedObject.ApplyModifiedProperties();
    }
  }
}
