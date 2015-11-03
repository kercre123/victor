//
// Procedural Lightning for Unity
// (c) 2015 Digital Ruby, LLC
// Source code may be used for personal or commercial projects.
// Source code may NOT be redistributed or sold.
// 

using UnityEngine;

#if UNITY_EDITOR

using UnityEditor;

#endif

namespace DigitalRuby.ThunderAndLightning
{
    [System.Serializable]
    public struct RangeOfIntegers
    {
        public int Minimum;
        public int Maximum;
    }

    [System.Serializable]
    public struct RangeOfFloats
    {
        public float Minimum;
        public float Maximum;
    }

    [System.Serializable]
    public class ReorderableList_GameObject : ReorderableList<GameObject> { }

    [System.Serializable]
    public class ReorderableList_Vector3 : ReorderableList<Vector3> { }

    [System.Serializable]
    public class ReorderableList_Rect : ReorderableList<Rect> { }

    [System.Serializable]
    public class ReorderableList_RectOffset : ReorderableList<RectOffset> { }

    [System.Serializable]
    public class ReorderableList_Int : ReorderableList<int> { }

    [System.Serializable]
    public class ReorderableList_Float : ReorderableList<float> { }

    [System.Serializable]
    public class ReorderableList_String : ReorderableList<string> { }

    public class ReorderableList<T> : ReorderableListBase { public System.Collections.Generic.List<T> List; }
    public class ReorderableListBase { }

    public class ReorderableListAttribute : PropertyAttribute
    {
        public ReorderableListAttribute(string tooltip) { Tooltip = tooltip; }

        public string Tooltip { get; private set; }
    }

    public class SingleLineAttribute : PropertyAttribute
    {
        public SingleLineAttribute(string tooltip) { Tooltip = tooltip; }

        public string Tooltip { get; private set; }
    }

    public class SingleLineClampAttribute : SingleLineAttribute
    {
        public SingleLineClampAttribute(string tooltip, double minValue, double maxValue) : base(tooltip)
        {
            MinValue = minValue;
            MaxValue = maxValue;
        }

        public double MinValue { get; private set; }
        public double MaxValue { get; private set; }
    }

#if UNITY_EDITOR

    [CustomPropertyDrawer(typeof(SingleLineAttribute))]
    [CustomPropertyDrawer(typeof(SingleLineClampAttribute))]
    public class SingleLineDrawer : PropertyDrawer
    {
        private void DrawIntTextField(Rect position, string text, string tooltip, SerializedProperty prop)
        {
            EditorGUI.BeginChangeCheck();
            int value = EditorGUI.IntField(position, new GUIContent(text, tooltip), prop.intValue);
            SingleLineClampAttribute clamp = attribute as SingleLineClampAttribute;
            if (clamp != null)
            {
                value = Mathf.Clamp(value, (int)clamp.MinValue, (int)clamp.MaxValue);
            }
            if (EditorGUI.EndChangeCheck())
            {
                prop.intValue = value;
            }
        }

        private void DrawFloatTextField(Rect position, string text, string tooltip, SerializedProperty prop)
        {
            EditorGUI.BeginChangeCheck();
            float value = EditorGUI.FloatField(position, new GUIContent(text, tooltip), prop.floatValue);
            SingleLineClampAttribute clamp = attribute as SingleLineClampAttribute;
            if (clamp != null)
            {
                value = Mathf.Clamp(value, (float)clamp.MinValue, (float)clamp.MaxValue);
            }
            if (EditorGUI.EndChangeCheck())
            {
                prop.floatValue = value;
            }
        }

        private void DrawRangeField(Rect position, SerializedProperty prop, bool floatingPoint)
        {
            EditorGUIUtility.labelWidth = 30.0f;
            EditorGUIUtility.fieldWidth = 40.0f;
            float width = position.width * 0.49f;
            float spacing = position.width * 0.02f;
            position.width = width;
            if (floatingPoint)
            {
                DrawFloatTextField(position, "Min", "Minimum value", prop.FindPropertyRelative("Minimum"));
            }
            else
            {
                DrawIntTextField(position, "Min", "Minimum value", prop.FindPropertyRelative("Minimum"));
            }
            position.x = position.xMax + spacing;
            position.width = width;
            if (floatingPoint)
            {
                DrawFloatTextField(position, "Max", "Maximum value", prop.FindPropertyRelative("Maximum"));
            }
            else
            {
                DrawIntTextField(position, "Max", "Maximum value", prop.FindPropertyRelative("Maximum"));
            }
        }

        public override void OnGUI(Rect position, SerializedProperty prop, GUIContent label)
        {
            EditorGUI.BeginProperty(position, label, prop);
            int indent = EditorGUI.indentLevel;
            EditorGUI.indentLevel = 0;
            position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), new GUIContent(label.text, (attribute as SingleLineAttribute).Tooltip));

            switch (prop.type)
            {
                case "RangeOfIntegers":
                    DrawRangeField(position, prop, false);
                    break;

                case "RangeOfFloats":
                    DrawRangeField(position, prop, true);
                    break;
           
                default:
                    EditorGUI.HelpBox(position, "[SingleLineDrawer] doesn't work with type '" + prop.type + "'", MessageType.Error);
                    break;
            }

            EditorGUI.indentLevel = indent;
            EditorGUI.EndProperty();
        }
    }

    [CustomPropertyDrawer(typeof(ReorderableListAttribute), true)]
    public class ReorderableListDrawer : UnityEditor.PropertyDrawer
    {
        private UnityEditorInternal.ReorderableList list;

        private UnityEditorInternal.ReorderableList GetList(SerializedProperty property)
        {
            if (list == null)
            {
                list = new UnityEditorInternal.ReorderableList(property.serializedObject, property, true, false, true, true);
                list.drawElementCallback = (UnityEngine.Rect rect, int index, bool isActive, bool isFocused) =>
                {
                    EditorGUIUtility.labelWidth = 100.0f;
                    EditorGUI.PropertyField(rect, property.GetArrayElementAtIndex(index), true);
                };
            }
            return list;
        }

        public override float GetPropertyHeight(SerializedProperty property, UnityEngine.GUIContent label)
        {
            return GetList(property.FindPropertyRelative("List")).GetHeight();
        }

        public override void OnGUI(UnityEngine.Rect position, SerializedProperty property, UnityEngine.GUIContent label)
        {
            ReorderableListAttribute attr = attribute as ReorderableListAttribute;
            string tooltip = (attr == null ? string.Empty : attr.Tooltip);
            var listProperty = property.FindPropertyRelative("List");
            var list = GetList(listProperty);
            var height = 0f;
            for (var i = 0; i < listProperty.arraySize; i++)
            {
                height = Mathf.Max(height, EditorGUI.GetPropertyHeight(listProperty.GetArrayElementAtIndex(i)));
            }
            list.drawHeaderCallback = (Rect r) =>
            {
                EditorGUI.LabelField(r, new GUIContent(label.text, tooltip));
            };
            list.elementHeight = height;
            list.DoList(position);
        }
    }

#endif

}
