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
        { EventGroupType.Robot_Sfx, typeof(GameEvent.Robot_Sfx) },
        { EventGroupType.Robot_Vo, typeof(GameEvent.Robot_Vo) },
        { EventGroupType.Sfx, typeof(GameEvent.Sfx) },
        { EventGroupType.Ui, typeof(GameEvent.Ui) },
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

      Type type = null;
      _EventTypeDictionary.TryGetValue(eventType, out type);

      if (type != null) {
        var options = Enum.GetNames(type);
        var values = Enum.GetValues(type).Cast<Enum>().Select(x => (int)(uint)(object)x).ToArray();
        position.y += position.height;

        int intEnumValue = EditorGUI.IntPopup(position, (int)(uint)Enum.Parse(typeof(GameEvent.GenericEvent), eventProp.stringValue), options, values);
        eventProp.stringValue = ((GameEvent.GenericEvent)intEnumValue).ToString();
      } else {
        int intEnumValue = EditorGUI.IntPopup(position, (int)(uint)GameEvent.GenericEvent.Invalid, new string[] {}, new int[] {});
        eventProp.stringValue = ((GameEvent.GenericEvent)intEnumValue).ToString();
      }

      EditorGUI.EndProperty();
    }
  }
}
