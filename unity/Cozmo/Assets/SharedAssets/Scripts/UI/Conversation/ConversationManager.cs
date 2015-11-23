using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {

  public class ConversationManager : MonoBehaviour {
    
    [SerializeField]
    private BaseView _LeftBubble;

    [SerializeField]
    private BaseView _RightBubble;

    private ConversationHistory _ConversationHistory = new ConversationHistory();
    private Conversation _CurrentConversation = new Conversation();
    private List<SpeechBubble> _SpeechBubbles = new List<SpeechBubble>();

    public void StartNewConversation() {
      _CurrentConversation = new Conversation();
      ClearSpeechBubbles();
    }

    public void SaveConversationToHistory() {
      _ConversationHistory.AddConversation(_CurrentConversation);
      StartNewConversation();
    }

    public void AddConversationLine(ConversationLine line) {
      _CurrentConversation.AddToConversation(line);
      _SpeechBubbles.Add(CreateSpeechBubble(line));
    }

    private void ClearSpeechBubbles() {
      for (int i = 0; i < _SpeechBubbles.Count; ++i) {
        UIManager.CloseView(_SpeechBubbles[i]);
      }
      _SpeechBubbles.Clear();
    }

    private SpeechBubble CreateSpeechBubble(ConversationLine line) {
      SpeechBubble newBubble;
      if (line.IsRight) {
        newBubble = UIManager.OpenView(_RightBubble) as SpeechBubble;
      }
      else {
        newBubble = UIManager.OpenView(_LeftBubble) as SpeechBubble;
      }
      newBubble.Initialize(line);
      return newBubble;
    }

  }

}
