using UnityEngine;
using System.Collections;

namespace Conversations {
  public class ConversationLine {

    public ConversationLine(string speaker, string lineID, bool isRight) {
      Speaker = speaker;
      LineID = lineID;
      IsRight = isRight;
    }

    public string Speaker;
    public string LineID;
    public bool IsRight;
  }
}
