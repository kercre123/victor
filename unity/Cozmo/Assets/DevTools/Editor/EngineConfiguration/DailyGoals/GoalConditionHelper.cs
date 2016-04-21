using System;
using Newtonsoft.Json;
using Cozmo.Util;
using Newtonsoft.Json.Linq;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;

namespace Anki.Cozmo {
  public abstract class GoalConditionHelper<T> where T : GoalCondition {
    //TODO: Write ConditionHelper based on the ScriptedSequenceHelper
    /*
    // By Default, we generate an editor for all fields that are int, float, string or bool
    // which lets us not write a custom editor for every single condition or action
    private System.Reflection.FieldInfo[] _Fields;

    private System.Reflection.FieldInfo[] Fields { 
      get {
        if (_Fields == null) {
          _Fields = typeof(T).GetFields(System.Reflection.BindingFlags.Public |
          System.Reflection.BindingFlags.Instance)
            .Where(x => x.FieldType == typeof(int) ||
          x.FieldType == typeof(float) ||
          x.FieldType == typeof(bool) ||
          x.FieldType == typeof(string) ||
          typeof(Enum).IsAssignableFrom(x.FieldType))
            .ToArray();
        }
        return _Fields;
      }
    }

    // Some conditions/actions don't have any parameters, so we don't need to draw the foldout
    protected override bool _Expandable {
      get {
        return Fields.Length > 0;
      }
    }
    */

  }
}