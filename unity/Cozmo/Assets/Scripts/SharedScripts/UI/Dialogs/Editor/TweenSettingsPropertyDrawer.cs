using UnityEngine;
using UnityEditor;
using System.Collections;

[CustomPropertyDrawer(typeof(Cozmo.UI.MoveTweenSettings))]
public class MoveTweenSettingsDrawer : PropertyDrawer {
  private const string kStaggerTypeName = "staggerType";
  private const string kUniformStaggerName = "uniformStagger_sec";
  private const string kTargetsName = "targets";

  private const string kTargetName = "target";
  private const string kOriginOffsetName = "originOffset";
  private const string kOpenSetting = "openSetting";
  private const string kCloseSetting = "closeSetting";

  private const string kStartTimeName = "startTime_sec";
  private const string kUseCustomName = "useCustom";
  private const string kDurationName = "duration_sec";
  private const string kEaseName = "easing";

  // Draw the property inside the given rect
  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    // Using BeginProperty / EndProperty on the parent property means that
    // prefab override logic works on the entire property.
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    float lineHeight = EditorGUIUtility.singleLineHeight;
    Rect controlRect = new Rect(position.x, position.y, position.width, lineHeight);
    property.isExpanded = EditorGUI.Foldout(controlRect, property.isExpanded, "Move Tween Settings:" + property.name);
    controlRect.y += GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return;
    }

    int indent = EditorGUI.indentLevel;
    EditorGUI.indentLevel = indent + 1;

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    EditorGUI.PropertyField(controlRect, staggerTypeProperty, new GUIContent("Anim Stagger Type"));
    controlRect.y += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      EditorGUI.PropertyField(controlRect, property.FindPropertyRelative(kUniformStaggerName), new GUIContent("Stagger (sec)"));
      controlRect.y += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    Rect targetsRect = new Rect(controlRect.x, controlRect.y, EditorGUIUtility.labelWidth, controlRect.height);
    EditorGUI.PropertyField(targetsRect, targetsArray, new GUIContent("Targets"));
    Rect sizeRect = new Rect(controlRect.x + EditorGUIUtility.labelWidth, controlRect.y,
               controlRect.width - EditorGUIUtility.labelWidth, controlRect.height);
    targetsArray.arraySize = EditorGUI.IntField(sizeRect, targetsArray.arraySize);
    controlRect.y += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      EditorGUI.indentLevel += 1;

      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        controlRect = DrawMoveTweenTarget(controlRect, targetsElement, currentStaggerType);
      }

      EditorGUI.indentLevel -= 1;
    }

    // Set indent back to what it was
    EditorGUI.indentLevel = indent;

    EditorGUI.EndProperty();
  }

  private Rect DrawMoveTweenTarget(Rect position, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    SerializedProperty transformTargetProp = elementProperty.FindPropertyRelative(kTargetName);
    Transform transformTarget = (Transform)transformTargetProp.objectReferenceValue;
    string transformName = (transformTarget != null) ? transformTarget.gameObject.name : "No Transform";
    EditorGUI.PropertyField(position, elementProperty, new GUIContent(transformName));
    position.y += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      EditorGUI.PropertyField(position, transformTargetProp, new GUIContent("Target"));
      position.y += GetLineHeightWithSpacing();

      EditorGUI.PropertyField(position, elementProperty.FindPropertyRelative(kOriginOffsetName), new GUIContent("Origin Offset (px)"));
      position.y += GetLineHeightWithSpacing();

      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kOpenSetting), "Open Settings", staggerType);
      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kCloseSetting), "Close Settings", staggerType);
    }

    return position;
  }

  private Rect DrawTweenSetting(Rect position, SerializedProperty settingProperty, string label, Cozmo.UI.StaggerType staggerType) {
    EditorGUI.LabelField(position, label);

    Rect toggleRect = new Rect(position.x + EditorGUIUtility.labelWidth, position.y,
                   position.width - EditorGUIUtility.labelWidth, position.height);
    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    toggleValue = EditorGUI.Toggle(toggleRect, "Use Custom", toggleValue);
    position.y += GetLineHeightWithSpacing();

    EditorGUI.indentLevel += 1;

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kStartTimeName), new GUIContent("Start Time (sec)"));
      position.y += GetLineHeightWithSpacing();
    }
    if (toggleValue) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kDurationName), new GUIContent("Duration (sec)"));
      position.y += GetLineHeightWithSpacing();
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kEaseName), new GUIContent("Easing"));
      position.y += GetLineHeightWithSpacing();
    }
    useCustomProperty.boolValue = toggleValue;

    EditorGUI.indentLevel -= 1;
    return position;
  }

  public override float GetPropertyHeight(SerializedProperty property, GUIContent label) {
    float propertyHeight = GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
    }

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    propertyHeight += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    propertyHeight += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        propertyHeight = GetMoveTweenHeight(propertyHeight, targetsElement, currentStaggerType);
      }
    }

    return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
  }

  private float GetMoveTweenHeight(float propertyHeight, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      propertyHeight += GetLineHeightWithSpacing();
      propertyHeight += GetLineHeightWithSpacing();

      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kOpenSetting), staggerType);
      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kCloseSetting), staggerType);
    }

    return propertyHeight;
  }

  private float GetTweenSettingHeight(float propertyHeight, SerializedProperty settingProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    if (toggleValue) {
      propertyHeight += GetLineHeightWithSpacing();
      propertyHeight += GetLineHeightWithSpacing();
    }

    return propertyHeight;
  }

  private float GetLineHeightWithSpacing() {
    return EditorGUIUtility.singleLineHeight + EditorGUIUtility.standardVerticalSpacing;
  }
}


[CustomPropertyDrawer(typeof(Cozmo.UI.RepeatableMoveTweenSettings))]
public class RepeatableMoveTweenSettingsDrawer : PropertyDrawer {
  private const string kStaggerTypeName = "staggerType";
  private const string kUniformStaggerName = "uniformStagger_sec";
  private const string kTargetsName = "targets";

  private const string kTargetName = "target";
  private const string kOriginOffsetName = "originOffset";
  private const string kOpenSetting = "openSetting";
  private const string kCloseSetting = "closeSetting";

  private const string kStartTimeName = "startTime_sec";
  private const string kUseCustomName = "useCustom";
  private const string kDurationName = "duration_sec";
  private const string kEaseName = "easing";

  // Draw the property inside the given rect
  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    // Using BeginProperty / EndProperty on the parent property means that
    // prefab override logic works on the entire property.
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    float lineHeight = EditorGUIUtility.singleLineHeight;
    Rect controlRect = new Rect(position.x, position.y, position.width, lineHeight);
    property.isExpanded = EditorGUI.Foldout(controlRect, property.isExpanded, "RepeatableMoveTweenSettings:" + property.name);
    controlRect.y += GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return;
    }

    int indent = EditorGUI.indentLevel;
    EditorGUI.indentLevel = indent + 1;

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    EditorGUI.PropertyField(controlRect, staggerTypeProperty, new GUIContent("Anim Stagger Type"));
    controlRect.y += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      EditorGUI.PropertyField(controlRect, property.FindPropertyRelative(kUniformStaggerName), new GUIContent("Stagger (sec)"));
      controlRect.y += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    Rect targetsRect = new Rect(controlRect.x, controlRect.y, EditorGUIUtility.labelWidth, controlRect.height);
    EditorGUI.PropertyField(targetsRect, targetsArray, new GUIContent("Targets"));
    Rect sizeRect = new Rect(controlRect.x + EditorGUIUtility.labelWidth, controlRect.y,
           controlRect.width - EditorGUIUtility.labelWidth, controlRect.height);
    targetsArray.arraySize = EditorGUI.IntField(sizeRect, targetsArray.arraySize);
    controlRect.y += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      EditorGUI.indentLevel += 1;

      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        controlRect = DrawMoveTweenTarget(controlRect, targetsElement, currentStaggerType);
      }

      EditorGUI.indentLevel -= 1;
    }

    // Set indent back to what it was
    EditorGUI.indentLevel = indent;

    EditorGUI.EndProperty();
  }

  private Rect DrawMoveTweenTarget(Rect position, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    SerializedProperty transformTargetProp = elementProperty.FindPropertyRelative(kTargetName);
    Transform transformTarget = (Transform)transformTargetProp.objectReferenceValue;
    string transformName = (transformTarget != null) ? transformTarget.gameObject.name : "No Transform";
    EditorGUI.PropertyField(position, elementProperty, new GUIContent(transformName));
    position.y += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      EditorGUI.PropertyField(position, transformTargetProp, new GUIContent("Target"));
      position.y += GetLineHeightWithSpacing();

      EditorGUI.PropertyField(position, elementProperty.FindPropertyRelative(kOriginOffsetName), new GUIContent("Origin Offset (px)"));
      position.y += GetLineHeightWithSpacing();

      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kOpenSetting), "Open Settings", staggerType);
      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kCloseSetting), "Close Settings", staggerType);
    }

    return position;
  }

  private Rect DrawTweenSetting(Rect position, SerializedProperty settingProperty, string label, Cozmo.UI.StaggerType staggerType) {
    EditorGUI.LabelField(position, label);

    Rect toggleRect = new Rect(position.x + EditorGUIUtility.labelWidth, position.y,
             position.width - EditorGUIUtility.labelWidth, position.height);
    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    toggleValue = EditorGUI.Toggle(toggleRect, "Use Custom", toggleValue);
    position.y += GetLineHeightWithSpacing();

    EditorGUI.indentLevel += 1;

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kStartTimeName), new GUIContent("Start Time (sec)"));
      position.y += GetLineHeightWithSpacing();
    }
    if (toggleValue) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kDurationName), new GUIContent("Duration (sec)"));
      position.y += GetLineHeightWithSpacing();
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kEaseName), new GUIContent("Easing"));
      position.y += GetLineHeightWithSpacing();
    }
    useCustomProperty.boolValue = toggleValue;

    EditorGUI.indentLevel -= 1;
    return position;
  }

  public override float GetPropertyHeight(SerializedProperty property, GUIContent label) {
    float propertyHeight = GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
    }

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    propertyHeight += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    propertyHeight += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        propertyHeight = GetMoveTweenHeight(propertyHeight, targetsElement, currentStaggerType);
      }
    }

    return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
  }

  private float GetMoveTweenHeight(float propertyHeight, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      propertyHeight += GetLineHeightWithSpacing();
      propertyHeight += GetLineHeightWithSpacing();

      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kOpenSetting), staggerType);
      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kCloseSetting), staggerType);
    }

    return propertyHeight;
  }

  private float GetTweenSettingHeight(float propertyHeight, SerializedProperty settingProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    if (toggleValue) {
      propertyHeight += GetLineHeightWithSpacing();
      propertyHeight += GetLineHeightWithSpacing();
    }

    return propertyHeight;
  }

  private float GetLineHeightWithSpacing() {
    return EditorGUIUtility.singleLineHeight + EditorGUIUtility.standardVerticalSpacing;
  }
}

[CustomPropertyDrawer(typeof(Cozmo.UI.FadeTweenSettings))]
public class FadeTweenSettingsDrawer : PropertyDrawer {
  private const string kStaggerTypeName = "staggerType";
  private const string kUniformStaggerName = "uniformStagger_sec";
  private const string kTargetsName = "targets";

  private const string kTargetName = "target";
  private const string kOpenSetting = "openSetting";
  private const string kCloseSetting = "closeSetting";

  private const string kStartTimeName = "startTime_sec";
  private const string kUseCustomName = "useCustom";
  private const string kDurationName = "duration_sec";
  private const string kEaseName = "easing";

  // Draw the property inside the given rect
  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    // Using BeginProperty / EndProperty on the parent property means that
    // prefab override logic works on the entire property.
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    float lineHeight = EditorGUIUtility.singleLineHeight;
    Rect controlRect = new Rect(position.x, position.y, position.width, lineHeight);
    property.isExpanded = EditorGUI.Foldout(controlRect, property.isExpanded, "Fade Tween Settings:" + property.name);
    controlRect.y += GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return;
    }

    int indent = EditorGUI.indentLevel;
    EditorGUI.indentLevel = indent + 1;

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    EditorGUI.PropertyField(controlRect, staggerTypeProperty, new GUIContent("Anim Stagger Type"));
    controlRect.y += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      EditorGUI.PropertyField(controlRect, property.FindPropertyRelative(kUniformStaggerName), new GUIContent("Stagger (sec)"));
      controlRect.y += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    Rect targetsRect = new Rect(controlRect.x, controlRect.y, EditorGUIUtility.labelWidth, controlRect.height);
    EditorGUI.PropertyField(targetsRect, targetsArray, new GUIContent("Targets"));
    Rect sizeRect = new Rect(controlRect.x + EditorGUIUtility.labelWidth, controlRect.y,
         controlRect.width - EditorGUIUtility.labelWidth, controlRect.height);
    targetsArray.arraySize = EditorGUI.IntField(sizeRect, targetsArray.arraySize);
    controlRect.y += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      EditorGUI.indentLevel += 1;

      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        controlRect = DrawFadeTweenTarget(controlRect, targetsElement, currentStaggerType);
      }

      EditorGUI.indentLevel -= 1;
    }

    // Set indent back to what it was
    EditorGUI.indentLevel = indent;

    EditorGUI.EndProperty();
  }

  private Rect DrawFadeTweenTarget(Rect position, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    SerializedProperty transformTargetProp = elementProperty.FindPropertyRelative(kTargetName);
    CanvasGroup transformTarget = (CanvasGroup)transformTargetProp.objectReferenceValue;
    string transformName = (transformTarget != null) ? transformTarget.gameObject.name : "No Canvas Group";
    EditorGUI.PropertyField(position, elementProperty, new GUIContent(transformName));
    position.y += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      EditorGUI.PropertyField(position, transformTargetProp, new GUIContent("Target"));
      position.y += GetLineHeightWithSpacing();

      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kOpenSetting), "Open Settings", staggerType);
      position = DrawTweenSetting(position, elementProperty.FindPropertyRelative(kCloseSetting), "Close Settings", staggerType);
    }

    return position;
  }

  private Rect DrawTweenSetting(Rect position, SerializedProperty settingProperty, string label, Cozmo.UI.StaggerType staggerType) {
    EditorGUI.LabelField(position, label);

    Rect toggleRect = new Rect(position.x + EditorGUIUtility.labelWidth, position.y,
             position.width - EditorGUIUtility.labelWidth, position.height);
    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    toggleValue = EditorGUI.Toggle(toggleRect, "Use Custom", toggleValue);
    position.y += GetLineHeightWithSpacing();

    EditorGUI.indentLevel += 1;

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kStartTimeName), new GUIContent("Start Time (sec)"));
      position.y += GetLineHeightWithSpacing();
    }
    if (toggleValue) {
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kDurationName), new GUIContent("Duration (sec)"));
      position.y += GetLineHeightWithSpacing();
      EditorGUI.PropertyField(position, settingProperty.FindPropertyRelative(kEaseName), new GUIContent("Easing"));
      position.y += GetLineHeightWithSpacing();
    }
    useCustomProperty.boolValue = toggleValue;

    EditorGUI.indentLevel -= 1;
    return position;
  }

  public override float GetPropertyHeight(SerializedProperty property, GUIContent label) {
    float propertyHeight = GetLineHeightWithSpacing();
    if (!property.isExpanded) {
      return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
    }

    SerializedProperty staggerTypeProperty = property.FindPropertyRelative(kStaggerTypeName);
    propertyHeight += GetLineHeightWithSpacing();

    Cozmo.UI.StaggerType currentStaggerType = (Cozmo.UI.StaggerType)staggerTypeProperty.enumValueIndex;
    if (currentStaggerType == Cozmo.UI.StaggerType.CustomUniform) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty targetsArray = property.FindPropertyRelative(kTargetsName);
    propertyHeight += GetLineHeightWithSpacing();

    if (targetsArray.isExpanded) {
      for (int i = 0; i < targetsArray.arraySize; i++) {
        SerializedProperty targetsElement = targetsArray.GetArrayElementAtIndex(i);
        propertyHeight = GetFadeTweenHeight(propertyHeight, targetsElement, currentStaggerType);
      }
    }

    return propertyHeight - EditorGUIUtility.standardVerticalSpacing;
  }

  private float GetFadeTweenHeight(float propertyHeight, SerializedProperty elementProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (elementProperty.isExpanded) {
      propertyHeight += GetLineHeightWithSpacing();

      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kOpenSetting), staggerType);
      propertyHeight = GetTweenSettingHeight(propertyHeight, elementProperty.FindPropertyRelative(kCloseSetting), staggerType);
    }

    return propertyHeight;
  }

  private float GetTweenSettingHeight(float propertyHeight, SerializedProperty settingProperty, Cozmo.UI.StaggerType staggerType) {
    propertyHeight += GetLineHeightWithSpacing();

    if (staggerType == Cozmo.UI.StaggerType.Custom) {
      propertyHeight += GetLineHeightWithSpacing();
    }

    SerializedProperty useCustomProperty = settingProperty.FindPropertyRelative(kUseCustomName);
    bool toggleValue = useCustomProperty.boolValue;
    if (toggleValue) {
      propertyHeight += GetLineHeightWithSpacing();
      propertyHeight += GetLineHeightWithSpacing();
    }

    return propertyHeight;
  }

  private float GetLineHeightWithSpacing() {
    return EditorGUIUtility.singleLineHeight + EditorGUIUtility.standardVerticalSpacing;
  }
}