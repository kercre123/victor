using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class MemoryBank {
  
  private Dictionary<MemorySlotSignature, MemorySlot> slots = new Dictionary<MemorySlotSignature, MemorySlot>();

  public void Initialize() {
    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.FALSE, 1);
    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.FALSE, 2);
    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.FALSE, 3);

    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.FALSE, 1);
    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.FALSE, 2);
    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.FALSE, 3);

    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.TRUE, 1);
    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.TRUE, 2);
    AddMemorySlotSignature(SlotRequirementFlag.FALSE, SlotRequirementFlag.TRUE, 3);

    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.TRUE, 1);
    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.TRUE, 2);
    AddMemorySlotSignature(SlotRequirementFlag.TRUE, SlotRequirementFlag.TRUE, 3);

    AddMemorySlotSignature(SlotRequirementFlag.NOT_REQUIRED, SlotRequirementFlag.NOT_REQUIRED, 4);
  }

  private void AddMemorySlotSignature(SlotRequirementFlag facingCozmo, SlotRequirementFlag verticalStack, int lightCount) {
    MemorySlotSignature newSignature = new MemorySlotSignature(facingCozmo, verticalStack, lightCount);
    slots.Add(newSignature, new MemorySlot(newSignature));
  }

  public void Add(BlockPattern pattern) {
    MemorySlotSignature slotSignature = new MemorySlotSignature(
                                          RequirementFlagUtil.FromBool(pattern.facingCozmo),
                                          RequirementFlagUtil.FromBool(pattern.verticalStack), 
                                          pattern.blocks[0].NumberOfLightsOn());

    if (slots.ContainsKey(slotSignature)) {
      slots[slotSignature].TryAdd(pattern);
      DAS.Info("MemoryBank", "Pattern Added with signature: " + slotSignature.facingCozmo + " " + slotSignature.verticalStack + " " + slotSignature.lightCount);
      DAS.Info("MemoryBank", "There are now " + slots[slotSignature].GetSeenPatterns().Count + " patterns in that slot");
    }
    else {
      DAS.Error("PatternPlayController", "Invalid pattern signature in memory slot");
    }
  }

  public bool Contains(BlockPattern pattern) {
    MemorySlotSignature bankSignature = new MemorySlotSignature(
                                          RequirementFlagUtil.FromBool(pattern.facingCozmo),
                                          RequirementFlagUtil.FromBool(pattern.verticalStack), 
                                          pattern.blocks[0].NumberOfLightsOn());

    if (slots.ContainsKey(bankSignature)) {
      if (slots[bankSignature].GetSeenPatterns().Contains(pattern)) {
        return true;
      }
    }
    return false;
  }

}
