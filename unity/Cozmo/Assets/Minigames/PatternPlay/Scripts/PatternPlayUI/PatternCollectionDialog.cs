using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

namespace PatternPlay {

  public class PatternCollectionDialog : BaseDialog {

    [SerializeField]
    private PatternCollectionBankCard _MemoryBankCardPrefab;

    [SerializeField]
    private RectTransform _MemoryBankCardContentPanel;

    [SerializeField]
    private Text _PercentCompleteLabel;

    [SerializeField]
    private ScrollRect _MemoryBankScrollRect;

    private List<PatternCollectionBankCard> _MemoryBankCards;

    [SerializeField]
    private float _SlowDragSpeedThreshold = 30f;

    [SerializeField]
    private HorizontalLayoutGroup _CardLayoutGroup;

    [SerializeField]
    private float _CardWidth = 600f;

    public void Initialize(PatternMemory patternMemory) {
      // Create all the memory bank cards
      _MemoryBankCards = CreateMemoryBankCards(patternMemory);

      // TODO: Listen for added patterns and update dialog - shouldn't update every frame
      // We'll need to cache the cards for easy access later

      SetCompletionText(patternMemory.GetNumSeenPatterns(), patternMemory.GetNumTotalPatterns());
    }

    protected override void CleanUp() {
    }

    public void OnDrag() {
      // The player could be dragging the scrollview slowly while 
      // looking at the cards at the same time. We want to remove the badge
      // if the player has seen it.
      float scrollXSpeed = Mathf.Abs(_MemoryBankScrollRect.velocity.x);
      if (scrollXSpeed < _SlowDragSpeedThreshold) {
        RemoveBadgesIfSeen();
      }
    }

    public void OnDragEnd() {
      // We want to remove the badge if the player has seen it, 
      // which is generally when they stop scrolling.
      RemoveBadgesIfSeen();
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    }

    private void RemoveBadgesIfSeen() {
      foreach (PatternCollectionBankCard card in _MemoryBankCards) {
        card.RemoveBadgeIfSeen();
      }
    }

    private List<PatternCollectionBankCard> CreateMemoryBankCards(PatternMemory patternMemory) {
      List<PatternCollectionBankCard> memoryBankCards = new List<PatternCollectionBankCard>();

      // Foreach memory bank...
      List<MemoryBank> memoryBanks = patternMemory.GetMemoryBanks();
      GameObject newBankCard;
      PatternCollectionBankCard bankCardScript;
      foreach (MemoryBank bank in memoryBanks) {
        // Only show cards if the player has seen at least one pattern
        if (bank.GetSeenPatterns().Count > 0) {
          // Create a card
          newBankCard = Instantiate(_MemoryBankCardPrefab.gameObject) as GameObject;

          // Layout the card into the scrollable panel
          // The layout will modify the position, so make sure that FALSE is 
          // there so that the world position will update.
          newBankCard.transform.SetParent(_MemoryBankCardContentPanel, false);

          // Initialize the card with the bank
          bankCardScript = newBankCard.GetComponent<PatternCollectionBankCard>();
          bankCardScript.Initialize(bank, _CardWidth);

          // Add the card to the list
          memoryBankCards.Add(bankCardScript);
        }
      }

      return memoryBankCards;
    }

    public void OnCloseButtonTap() {
      RemoveBadgesIfSeen();
      UIManager.CloseDialog(this);
    }

    private void SetCompletionText(int numSeenPatterns, int numTotalPatterns) {
      float percentComplete = numSeenPatterns / (float)numTotalPatterns;
      percentComplete *= 100;
      _PercentCompleteLabel.text = string.Format("{0:N1}% Complete", percentComplete);
    }

    public bool ScrollToFirstNewPattern() {
      bool scrolled = false;
      int badgedCardIndex = GetFirstBadgedCard();
      if (badgedCardIndex >= 0) {
        float scrollValue = CalculateScrollValue(badgedCardIndex);
        SetScrollValue(scrollValue);
        scrolled = true;
      }
      return scrolled;
    }

    public float CalculateScrollValue(int badgedCardIndex) {
      float cardSpacing = _CardLayoutGroup.spacing;
      float cardWidth = _CardWidth;
      float scrollLeftOfScreen = (cardSpacing + cardWidth) * badgedCardIndex;

      int numCards = _MemoryBankCards.Count;
      float fullCardWidth = (numCards * cardWidth) + ((numCards - 1) * cardSpacing);
      float screenCanvasWidth = GetScrollViewWidth();
      float scrollOffOfScreen = fullCardWidth - screenCanvasWidth;

      float normalizedPosition = scrollLeftOfScreen / scrollOffOfScreen;
      normalizedPosition = Mathf.Clamp(normalizedPosition, 0f, 1f);

      return normalizedPosition;
    }

    private int GetFirstBadgedCard() {
      int badgedCardIndex = -1;
      int currentIndex = 0;
      foreach (PatternCollectionBankCard card in _MemoryBankCards) {
        if (card.IsBadged()) {
          badgedCardIndex = currentIndex;
          break;
        }
        currentIndex++;
      }
      return badgedCardIndex;
    }

    private float GetScrollViewWidth() {
      RectTransform rectTransform = _MemoryBankScrollRect.transform as RectTransform;
      return rectTransform.rect.width;
    }

    public void SetScrollValue(float scrollValue) {
      float clampValue = Mathf.Clamp(scrollValue, 0, 1);
      _MemoryBankScrollRect.horizontalNormalizedPosition = clampValue;
    }

    public float GetScrollValue() {
      return _MemoryBankScrollRect.horizontalNormalizedPosition;
    }
  }

}
