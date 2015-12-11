using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;

namespace Conversations {

  public class ConversationManager : MonoBehaviour {

    private static readonly IDAS sDAS = DAS.GetInstance(typeof(ConversationManager));

    private static ConversationManager _Instance;

    public static ConversationManager Instance {
      get {
        if (_Instance == null) {
          sDAS.Error("Don't access this until Start!");
        }
        return _Instance;
      }
      private set {
        if (_Instance != null) {
          sDAS.Error("There shouldn't be more than one ConversationManager");
        }
        _Instance = value;
      }
    }

    [SerializeField]
    private BaseView _LeftBubble;

    [SerializeField]
    private BaseView _RightBubble;

    [SerializeField]
    private BaseView _ConvoOverlayPrefab;
    private ConvoBackground _CurrBackground;

    private Conversation _CurrentConversation = new Conversation();
    private string _CurrentConversationKey;
    private SpeechBubble _CurrentSpeechBubble;

    void Awake() {
      Instance = this;
      StartNewConversation("Default");
    }

    public void StartNewConversation(string conversationKey) {
      _CurrentConversation = new Conversation();
      _CurrentConversationKey = conversationKey;
      if (_CurrBackground != null) {
        UIManager.CloseView(_CurrBackground);
      }
      if (_CurrentSpeechBubble != null) {
        UIManager.CloseView(_CurrentSpeechBubble);
      }
    }

    public void AbortCurrentConversation() {
      StartNewConversation("Default");
    }

    public void AddConversationLine(ConversationLine line) {
      _CurrentConversation.AddToConversation(line);
      if (_CurrentSpeechBubble != null) {
        UIManager.CloseView(_CurrentSpeechBubble);
      }
      if (_CurrBackground == null) {
        _CurrBackground = UIManager.OpenView(_ConvoOverlayPrefab) as ConvoBackground;
      }
      _CurrentSpeechBubble = CreateSpeechBubble(line);
    }

    public void SaveConversationToHistory() {
      DataPersistence.DataPersistenceManager.Instance.Data.ConversationHistory.AddConversation(_CurrentConversationKey, _CurrentConversation);
      DataPersistence.DataPersistenceManager.Instance.Save();
      StartNewConversation("Default");
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
