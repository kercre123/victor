using UnityEngine;
using System.Collections;

namespace Conversations {
  public class ConversationLine {

    // For Serialization Only!
    [System.Obsolete]
    public ConversationLine() { }

    public ConversationLine(Speaker lineSpeaker, string lineKey, bool isRight) {
      LineSpeaker = lineSpeaker;
      LineKey = lineKey;
      IsRight = isRight;
    }

    public Speaker LineSpeaker;
    public string LineKey;
    public bool IsRight;
  }
}
