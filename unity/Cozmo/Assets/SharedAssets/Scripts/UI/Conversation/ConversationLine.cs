using UnityEngine;
using System.Collections;

namespace Conversations {
  public class ConversationLine {

    public ConversationLine(string speaker, string lineKey, bool isRight) {
      Speaker = speaker;
      LineKey = lineKey;
      IsRight = isRight;
    }

    public string Speaker;
    public string LineKey;
    public bool IsRight;
  }
}
