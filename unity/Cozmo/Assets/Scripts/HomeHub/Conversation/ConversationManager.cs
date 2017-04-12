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
    private SpeechBubble _LeftBubble;

    [SerializeField]
    private SpeechBubble _RightBubble;

    [SerializeField]
    private ConvoBackground _ConvoOverlayPrefab;
    private ConvoBackground _CurrBackground;

    private Conversation _CurrentConversation = new Conversation();
    private string _CurrentConversationKey;
    private SpeechBubble _CurrentLeftSpeechBubble;
    private SpeechBubble _CurrentRightSpeechBubble;

    void Awake() {
      Instance = this;
      StartNewConversation("Default");
    }

    public void StartNewConversation(string conversationKey) {
      _CurrentConversation = new Conversation();
      _CurrentConversationKey = conversationKey;
      if (_CurrBackground != null) {
        UIManager.CloseModal(_CurrBackground);
        _CurrBackground = null;
      }
      if (_CurrentLeftSpeechBubble != null) {
        UIManager.CloseModal(_CurrentLeftSpeechBubble);
      }
      if (_CurrentRightSpeechBubble != null) {
        UIManager.CloseModal(_CurrentRightSpeechBubble);
      }
    }

    public void AbortCurrentConversation() {
      StartNewConversation("Default");
    }

    public void AddConversationLine(ConversationLine line) {
      _CurrentConversation.AddToConversation(line);
      if (_CurrBackground == null) {
        UIManager.OpenModal(_ConvoOverlayPrefab, new ModalPriorityData(), HandleConvoBackgroundCreated);
      }
      CreateSpeechBubble(line);
    }

    private void HandleConvoBackgroundCreated(Cozmo.UI.BaseModal newConvoBackground) {
      _CurrBackground = (ConvoBackground)newConvoBackground;
      _CurrBackground.InitButtonText(LocalizationKeys.kButtonContinue);
    }

    public void SaveConversationToHistory() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ConversationHistory.AddConversation(_CurrentConversationKey, _CurrentConversation);
      DataPersistence.DataPersistenceManager.Instance.Save();
      StartNewConversation("Default");
    }

    private void CreateSpeechBubble(ConversationLine line) {
      if (line.IsRight) {
        if (_CurrentRightSpeechBubble != null) {
          UIManager.CloseModal(_CurrentRightSpeechBubble);
        }
        UIManager.OpenModal(_RightBubble, new ModalPriorityData(), (newRightBubble) => {
          _CurrentRightSpeechBubble = (SpeechBubble)newRightBubble;
          _CurrentRightSpeechBubble.Initialize(line);
        });
      }
      else {
        if (_CurrentLeftSpeechBubble != null) {
          UIManager.CloseModal(_CurrentLeftSpeechBubble);
        }
        UIManager.OpenModal(_LeftBubble, new ModalPriorityData(), (newLeftBubble) => {
          _CurrentLeftSpeechBubble = (SpeechBubble)newLeftBubble;
          _CurrentLeftSpeechBubble.Initialize(line);
        });
      }
    }
  }

}
