using UnityEngine;
using UnityEditor;
using System.Linq;
using System.Collections.Generic;
using Anki.Cozmo.Audio.GameEvent;
using System;

namespace Anki.Cozmo.Audio {

  [CustomPropertyDrawer(typeof(AudioEventParameter))]
  public class AudioEventParameterDrawer : PropertyDrawer {

    private static readonly Dictionary<EventGroupType, Type> _EventTypeDictionary =
      new Dictionary<EventGroupType, Type> {
        { EventGroupType.Dev_Robot, typeof(GameEvent.Dev_Robot) },
        { EventGroupType.GenericEvent, typeof(GameEvent.GenericEvent) },
        { EventGroupType.Music, typeof(GameEvent.Music) },
        { EventGroupType.Robot, typeof(GameEvent.Robot) },
        { EventGroupType.SFX, typeof(GameEvent.SFX) },
        { EventGroupType.UI, typeof(GameEvent.UI) },
      };

    private struct EventTypeOption {
      public string OptionName;
      public EventGroupType EventType;
      public GameObjectType GameObjectType;

      public EventTypeOption(string name, EventGroupType eventType, GameObjectType gameObjectType) {
        OptionName = name;
        EventType = eventType;
        GameObjectType = gameObjectType;
      }
    }

    public override float GetPropertyHeight(SerializedProperty property, GUIContent label) {
      return base.GetPropertyHeight(property, label) * 2;
    }

    public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
      position.height /= 2;
      EditorGUI.BeginProperty(position, label, property);

      // Draw label
      position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

      var eventProp = property.FindPropertyRelative("_Event");
      var eventTypeProp = property.FindPropertyRelative("_EventType");

      eventTypeProp.stringValue = EditorGUI.EnumPopup(position, (EventGroupType)Enum.Parse(typeof(EventGroupType), eventTypeProp.stringValue)).ToString();

      var eventType = (EventGroupType)Enum.Parse(typeof(EventGroupType), eventTypeProp.stringValue);

      var type = _EventTypeDictionary[eventType];

      var options = Enum.GetNames(type);
      var values = Enum.GetValues(type).Cast<Enum>().Select(x => (int)(uint)(object)x).ToArray();
      position.y += position.height;

      int intEnumValue = EditorGUI.IntPopup(position, (int)(uint)Enum.Parse(typeof(GameEvent.GenericEvent), eventProp.stringValue), options, values);
      eventProp.stringValue = ((GameEvent.GenericEvent)intEnumValue).ToString();

      EditorGUI.EndProperty();
    }
  }
}