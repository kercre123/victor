using System;
using Conversations;

namespace ScriptedSequences.Actions {
  public class DisplayConversationLine : ScriptedSequenceAction {
    public string LineKey;  // Text shown in the Overlay
    public Speaker Speaker; // An enum that refers to the sprite we should show for this line
    public bool IsRight; // Whether or not its oriented to the right side of the screen or left

    public override ISimpleAsyncToken Act() {

      SimpleAsyncToken token = new SimpleAsyncToken();

      ConversationManager.Instance.AddConversationLine(new ConversationLine(Speaker, LineKey, IsRight));
      token.Succeed();
      return token;
    }
  }
}

