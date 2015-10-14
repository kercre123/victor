using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternMemory {
  
  private List<MemoryBank> memoryBanks = new List<MemoryBank>();
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
    memoryBanks.Add(new MemoryBank(newSignature));
  }

  public List<MemoryBank> GetMemoryBanks() {
    return memoryBanks;
  }

  public void Add(BlockPattern pattern) {
    MemoryBankSignature bankSignature = new MemoryBankSignature(
                                          RequirementFlagUtil.FromBool(pattern.facingCozmo),
                                          RequirementFlagUtil.FromBool(pattern.verticalStack), 
                                          pattern.blocks[0].NumberOfLightsOn());

    for (int i = 0; i < memoryBanks.Count; ++i) {
      
      if (memoryBanks[i].TryAdd(pattern)) {
        DAS.Info("MemoryBank", "Pattern Added with signature: " + bankSignature.facingCozmo + " " + bankSignature.verticalStack + " " + bankSignature.lightCount);
        DAS.Info("MemoryBank", "There are now " + memoryBanks[i].GetSeenPatterns().Count + " patterns in that bank");
        return;
      }

    }

    DAS.Error("PatternPlayController", "Invalid pattern signature in memory bank");

  }

  public bool Contains(BlockPattern pattern) {
    for (int i = 0; i < memoryBanks.Count; ++i) {
      if (memoryBanks[i].GetSeenPatterns().Contains(pattern)) {
        return true;
      }
    }
    return false;
  }

}
