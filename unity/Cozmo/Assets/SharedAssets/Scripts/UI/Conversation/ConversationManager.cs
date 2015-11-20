using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class ConversationManager : MonoBehaviour {

    private ConversationData.Conversation _currConversation;

    private ConversationData.ConversationLine _currLine;

    private List<SpeechBubbleDialog> _speechBubbleList;

    private SpeechBubbleDialog _currBubble;

    private int _currIndex;

    void Awake() {
      // read conversationData JSON FILE PATH HERE 
      /* Replace GetPathForResource with similar function
      string converJsonPath = Anki.DriveEngine.Platform.GetPathForResource("basestation/config/campaign/cutscenes.json");
      string converJsonText = System.IO.File.ReadAllText(converJsonPath);
      JSONObject converJson = new JSONObject(converJsonText);
      ConversationData.ParseJson(converJson);
      */
    }

    public void NextLine() {
      // TODO: Advance to the next line in the conversation
      // push previous speech bubbles upwards (similar to text message conversation)
      _currIndex++;
      if (_currConversation.Lines.Count <= _currIndex) {
        KillCurrentConvo();
        ConvoEnded();
      }
      else {
        _currLine = _currConversation.Lines[_currIndex];
        //_currBubble = CreateSpeechBubble(_currLine);
      }
    }

    public void StartConvo(string sceneName) {
      KillCurrentConvo();
      // TODO: Initializes the specified scene by name from ConversationData
      _currConversation = ConversationData.GetConversationFromData(sceneName);
      _currIndex = 0;
      _currLine = _currConversation.Lines[_currIndex];
      //_currBubble = CreateSpeechBubble(_currLine);
    }

    public void ConvoEnded() {
      // TODO: Fire this when you hit the end of the conversation, handles cleanup for conversation
    }

    public void KillCurrentConvo() {
      // TODO: Interrupt current converation at any point without it ending naturally
    }

    private void /*SpeechBubbleDialog*/ CreateSpeechBubble(ConversationData.ConversationLine line) {
      // TODO: Create and return the speech bubble dialog created by this line

    }

  }

}
