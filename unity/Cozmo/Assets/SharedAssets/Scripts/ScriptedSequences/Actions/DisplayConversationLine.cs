using System;

namespace ScriptedSequences.Actions {
  public class DisplayConversationLine : ScriptedSequenceAction {
    public string LineID; // the ID of the line
    public string Text;  // Text shown in the Overlay
    public string CharacterSprite; // The filepath of the sprite that the Overlay uses
    public string VoID; // Name of the VO event to be played when the line appears
    public bool IsRight; // Whether or not its oriented to the right side of the screen or left

    private Conversations.ConversationManager _ConversationManager;

    public override ISimpleAsyncToken Act() {

      SimpleAsyncToken token = new SimpleAsyncToken();

      var speechBubble = _ConversationManager.CreateSpeechBubble(new ConversationData.ConversationLine(LineID, Text, CharacterSprite, VoID, IsRight));

      if (speechBubble != null) {
        token.Succeed();
      } else {
        token.Fail(new Exception("Failed To Create Speech Bubble for Line "+LineID));
      }

      return token;
    }
  }
}

