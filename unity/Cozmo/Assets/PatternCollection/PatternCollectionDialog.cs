using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionDialog : MonoBehaviour {

  [SerializeField]
  private PatternCollectionBankCard _memoryBankCardPrefab;

  [SerializeField]
  private RectTransform _memoryBankCardContentPanel;

  private Dictionary<MemoryBank, PatternCollectionBankCard> _signatureToCardDict;

	public void Initialize(PatternMemory patternMemory){
    // Create all the memory bank cards
    _signatureToCardDict = CreateMemoryBankCards (patternMemory);

    // TODO: Listen for added patterns and update dialog - shouldn't update every frame
    // We'll need to cache the cards for easy access later

    // TODO: Set up other buttons? (Key pattern filter, back button)
  }
  
  private Dictionary<MemoryBank, PatternCollectionBankCard> CreateMemoryBankCards(PatternMemory patternMemory) {
    Dictionary<MemoryBank, PatternCollectionBankCard> memoryBankCards 
    = new Dictionary<MemoryBank, PatternCollectionBankCard>();

    // Foreach memory bank...
    List<MemoryBank> memoryBanks = patternMemory.GetMemoryBanks ();
    GameObject newBankCard;
    PatternCollectionBankCard bankCardScript;
    foreach (MemoryBank bank in memoryBanks) {
      // Create a card
      newBankCard = Instantiate(_memoryBankCardPrefab.gameObject) as GameObject;

      // Layout the card into the scrollable panel
      // The layout will modify the position, so make sure that FALSE is 
      // there so that the world position will update.
      newBankCard.transform.SetParent(_memoryBankCardContentPanel, false);

      // Initialize the card with the bank
      bankCardScript = newBankCard.GetComponent<PatternCollectionBankCard>();
      bankCardScript.Initialize(bank);

      // Add the card to the dictionary
      memoryBankCards.Add ( bank, bankCardScript );
    }

    return memoryBankCards;
  }

  public void OnCloseButtonTap()
  {
    UIManager.CloseDialog (this.gameObject);
  }
}
