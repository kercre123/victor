using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternMemory {
  
  private Dictionary<MemoryBankSignature, MemoryBank> memoryBanks = new Dictionary<MemoryBankSignature, MemoryBank>();
  private HashSet<BlockPattern> keyPatterns = new HashSet<BlockPattern>();

  public void Initialize() {

    // adding bank signatures.
    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.FALSE, 1);
    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.FALSE, 2);
    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.FALSE, 3);

    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.FALSE, 1);
    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.FALSE, 2);
    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.FALSE, 3);

    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.TRUE, 1);
    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.TRUE, 2);
    AddMemoryBankSignature(BankRequirementFlag.FALSE, BankRequirementFlag.TRUE, 3);

    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.TRUE, 1);
    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.TRUE, 2);
    AddMemoryBankSignature(BankRequirementFlag.TRUE, BankRequirementFlag.TRUE, 3);

    AddMemoryBankSignature(BankRequirementFlag.NOT_REQUIRED, BankRequirementFlag.NOT_REQUIRED, 4);

    // adding key patterns.

  }

  private void AddMemoryBankSignature(BankRequirementFlag facingCozmo, BankRequirementFlag verticalStack, int lightCount) {
    MemoryBankSignature newSignature = new MemoryBankSignature(facingCozmo, verticalStack, lightCount);
    memoryBanks.Add(newSignature, new MemoryBank(newSignature));
  }

  public void Add(BlockPattern pattern) {
    MemoryBankSignature bankSignature = new MemoryBankSignature(
                                          RequirementFlagUtil.FromBool(pattern.facingCozmo),
                                          RequirementFlagUtil.FromBool(pattern.verticalStack), 
                                          pattern.blocks[0].NumberOfLightsOn());

    if (memoryBanks.ContainsKey(bankSignature)) {
      memoryBanks[bankSignature].TryAdd(pattern);
      DAS.Info("MemoryBank", "Pattern Added with signature: " + bankSignature.facingCozmo + " " + bankSignature.verticalStack + " " + bankSignature.lightCount);
      DAS.Info("MemoryBank", "There are now " + memoryBanks[bankSignature].GetSeenPatterns().Count + " patterns in that bank");
    }
    else {
      DAS.Error("PatternPlayController", "Invalid pattern signature in memory bank");
    }
  }

  public bool Contains(BlockPattern pattern) {
    MemoryBankSignature bankSignature = new MemoryBankSignature(
                                          RequirementFlagUtil.FromBool(pattern.facingCozmo),
                                          RequirementFlagUtil.FromBool(pattern.verticalStack), 
                                          pattern.blocks[0].NumberOfLightsOn());

    if (memoryBanks.ContainsKey(bankSignature)) {
      if (memoryBanks[bankSignature].GetSeenPatterns().Contains(pattern)) {
        return true;
      }
    }
    return false;
  }

}
