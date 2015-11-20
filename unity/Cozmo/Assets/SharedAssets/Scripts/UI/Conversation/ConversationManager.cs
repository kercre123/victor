using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  // Manager Class for handling starting, ending, and advancing Conversations
  // using Speech Bubbles. Currently only manages one conversation at a time,
  // will need to be modified if we want to Enqueue multiple conversations.
  public class ConversationManager : MonoBehaviour {
    
    public event Action<ConversationData.Conversation> ConversationEnded;

    [SerializeField]
    private BaseView _LeftBubble;

    [SerializeField]
    private BaseView _RightBubble;

    private ConversationData.Conversation? _currConversation;
    public ConversationData.Conversation? CurrConversation {
      get {
        if (_currConversation.HasValue) {
          return _currConversation;
        }
        else {
          Debug.LogWarning("ConversationManager.CurrConversation is NULL");
          return null;
        }
      }
      set {
        if (value.HasValue) {
          StartConvoFromName(value.Value.SceneName);
        }
      }
    }

    private ConversationData.ConversationLine _currLine;

    private List<SpeechBubble> _speechBubbleList;

    private SpeechBubble _currBubble;

    private int _currIndex = 0;

    /* HACK: for testing
    void Start() {
      List<ConversationData.ConversationLine> testList = new List<ConversationData.ConversationLine>();
      testList.Add(new ConversationData.ConversationLine("testLine", "This is a Test", "fakeSprite", "fakeVO", true));
      testList.Add(new ConversationData.ConversationLine("testLine1", "This is another Test", "fakeSprite", "fakeVO", false));
      testList.Add(new ConversationData.ConversationLine("testLine2", "Yeah. It's a Test", "fakeSprite", "fakeVO", true));
      StartConvoFromData(new ConversationData.Conversation("Test",testList));

      StartCoroutine(TestRoutine(1.5f));
    }

    IEnumerator TestRoutine(float time) {
      yield return new WaitForSeconds(time);
      NextLine();
      StartCoroutine(TestRoutine(1.5f));
    }
    // END HACK */

    // Advance to the next line in the current conversation
    // If on the last line of the conversation, ends the conversation
    public void NextLine() {
      if (!_currConversation.HasValue) {
        Debug.LogError("ConversationManager.NextLine() - No Current Conversation to advance");
        return;
      }
      // Close current Speech Bubble
      _currBubble.CloseView();
      _currIndex++;
      if (_currConversation.Value.Lines.Count <= _currIndex) {
        ConvoEnded();
      }
      else {
        _currLine = _currConversation.Value.Lines[_currIndex];
        _currBubble = CreateSpeechBubble(_currLine);
      }
    }

    // Initializes the specified scene by name from ConversationData, pops up the first SpeechBubble
    public void StartConvoFromName(string convoName) {
      StartConvoFromData(ConversationData.GetConversationFromData(convoName));
    }

    private void StartConvoFromData(ConversationData.Conversation convo) {
      if (_currConversation.HasValue) {
        KillCurrentConvo();
      }
      _currIndex = 0;
      _currConversation = convo;
      if (_currConversation.HasValue && _currConversation.Value.Lines.Count > 0) {
        _currLine = _currConversation.Value.Lines[_currIndex];
        _currBubble = CreateSpeechBubble(_currLine);
      }
      else {
        Debug.LogError("ConversationManager.StartConvoFromData - Invalid Convo, no lines to play");
        _currConversation = null;
      }
    }

    // Fire this when you hit the end of the conversation naturally,
    // handles cleanup for conversation and fires any potential relevant actions.
    private void ConvoEnded() {
      if (!_currConversation.HasValue) {
        Debug.LogError("ConversationManager.ConvoEnded() - No Current Conversation to end");
        return;
      }
      if (ConversationEnded != null) {
        ConversationEnded(_currConversation.Value);
      }
      _currConversation = null;
    }

    // Interrupt current converation at any point without it ending naturally
    // Ideally for handling quitting out or issues mid conversation
    public void KillCurrentConvo() {
      if (_currBubble == null || !_currConversation.HasValue) {
        Debug.LogWarning("ConversationManager.KillCurrentConvo - No Current Conversation to kill");
        return;
      }
      _currBubble.CloseViewImmediately();
      _currConversation = null;
    }

    // Opens and returns a speech bubble based on the specified line
    // Currently I have 2 prefabs for right/left, but later we may 
    // just want a single prefab solution that handles repositioning in code.
    private SpeechBubble CreateSpeechBubble(ConversationData.ConversationLine line) {
      SpeechBubble newBubble;
      if (line.isRight) {
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
