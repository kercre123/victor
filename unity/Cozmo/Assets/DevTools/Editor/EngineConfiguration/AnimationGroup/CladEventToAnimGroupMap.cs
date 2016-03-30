using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;

/// <summary>
/// Clad event animation entry, used to show mappings within the AnimationGroupEventEditor.
/// </summary>
namespace Anki.Cozmo {
  [JsonConverter(typeof(EventToAnimGroupMapConverter))]
  public class CladEventToAnimGroupMap {

    [JsonIgnore]
    public Dictionary<CladGameEvent, string> EventToGroupDict = new Dictionary<CladGameEvent, string>();

    [JsonProperty("CladEvents")]
    public List<CladGameEvent> CladEvents = new List<CladGameEvent>();
    [JsonProperty("AnimationGroups")]
    public List<string> AnimationGroups = new List<string>();
  }
}

