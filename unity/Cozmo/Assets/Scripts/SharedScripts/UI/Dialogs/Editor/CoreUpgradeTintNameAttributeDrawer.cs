using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    [CustomPropertyDrawer(typeof(CoreUpgradeTintNameAttribute))]
    public class CoreUpgradeTintNameAttributeDrawer : PropertyDrawer {
      private const string kUiColorPaletteLocation = "Assets/AssetBundles/UI/BasicUIPrefabs-Bundle/UIColorPalette.asset";

      private string[] _UpgradeTintIds = null;
      // Draw the property inside the given rect
      public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
        if (_UpgradeTintIds == null) {
          _UpgradeTintIds = GetAllUpgradeTintIds();
        }

        int currentOption = 0;
        string currentValue = property.stringValue;
        if (!string.IsNullOrEmpty(currentValue)) {
          for (int i = 0; i < _UpgradeTintIds.Length; i++) {
            if (_UpgradeTintIds[i] == currentValue) {
              currentOption = i;
              break;
            }
          }
        }

        int newOption = EditorGUI.Popup(position, label.text, currentOption, _UpgradeTintIds);
        string newValue = _UpgradeTintIds[newOption];

        if (newValue != currentValue) {
          property.stringValue = newValue;
          property.serializedObject.ApplyModifiedProperties();
        }
      }

      private string[] GetAllUpgradeTintIds() {
        UIColorPalette uiColorPalette = AssetDatabase.LoadAssetAtPath<UIColorPalette>(kUiColorPaletteLocation);
        List<string> allIds = new List<string>();
        allIds.AddRange(uiColorPalette.EditorGetItemIds());
        return allIds.ToArray();
      }
    }
  }
}