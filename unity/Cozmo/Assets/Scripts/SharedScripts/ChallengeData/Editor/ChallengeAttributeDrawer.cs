using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [CustomPropertyDrawer(typeof(ChallengeIdAttribute))]
  public class ChallengeAttributeDrawer : PropertyDrawer {
    private const string kChallengeDataConfigLocation = "Assets/AssetBundles/GameMetadata-Bundle/ChallengeData/ChallengeList.asset";

    private string[] _ChallengeIDs = null;
    // Draw the property inside the given rect
    public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
      EditorGUI.BeginProperty(position, label, property);

      position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

      if (_ChallengeIDs == null) {
        _ChallengeIDs = GetAllChallengeIds();
      }
      int currentOption = 0;
      string currentValue = property.stringValue;
      if (!string.IsNullOrEmpty(currentValue)) {
        for (int i = 0; i < _ChallengeIDs.Length; i++) {
          if (_ChallengeIDs[i] == currentValue) {
            currentOption = i;
            break;
          }
        }
      }

      int newOption = EditorGUI.Popup(position, label.text, currentOption, _ChallengeIDs);
      string newValue = _ChallengeIDs[newOption];

      if (newValue != currentValue) {
        property.stringValue = newValue;
        property.serializedObject.ApplyModifiedProperties();
      }

      EditorGUI.EndProperty();
    }

    private string[] GetAllChallengeIds() {
      ChallengeDataList challengeDataConfig = AssetDatabase.LoadAssetAtPath<ChallengeDataList>(kChallengeDataConfigLocation);
      List<string> allIds = new List<string>();
      allIds.AddRange(challengeDataConfig.EditorGetIds());
      return allIds.ToArray();
    }
  }
}