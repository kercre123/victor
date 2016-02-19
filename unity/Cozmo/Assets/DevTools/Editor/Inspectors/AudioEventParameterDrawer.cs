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
        { EventGroupType.DevRobot, typeof(GameEvent.DevRobot) },
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

    // We want to limit choices
    private List<EventTypeOption> _Options = new List<EventTypeOption>() {
      new EventTypeOption("VO", EventGroupType.GenericEvent, GameObjectType.Aria),
      new EventTypeOption("UI", EventGroupType.UI, GameObjectType.UI),
      new EventTypeOption("SFX", EventGroupType.SFX, GameObjectType.SFX)
    };


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
      var gameObjectTypeProp = property.FindPropertyRelative("_GameObjectType");

      var options = _Options.Select(x => x.OptionName).ToArray();
      var values = _Options.Select(x => (int)(uint)x.EventType).ToArray();
      eventTypeProp.intValue = EditorGUI.IntPopup(position, eventTypeProp.intValue, options, values);

      var eventType = (EventGroupType)(uint)(eventTypeProp.intValue);

      gameObjectTypeProp.intValue = (int)(uint)_Options.FirstOrDefault(x => x.EventType == eventType).GameObjectType;

      var type = _EventTypeDictionary[eventType];

      options = Enum.GetNames(type);
      values = Enum.GetValues(type).Cast<Enum>().Select(x => (int)(uint)(object)x).ToArray();

      position.y += position.height;

      eventProp.intValue = EditorGUI.IntPopup(position, eventProp.intValue, options, values);

      EditorGUI.EndProperty();
    }
  }
}