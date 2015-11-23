using UnityEngine;
using System.Collections;

namespace Conversations {
  public class ConversationLine {

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
