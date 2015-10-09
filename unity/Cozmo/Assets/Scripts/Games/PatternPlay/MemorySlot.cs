using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// we use this and not just a boolean because some
// memory bank requirements don't care about if the blocks
// are facing cozmo or are a vertical stack (ie the 4 lights pattern).
public enum SlotRequirementFlag {
  FALSE,
  TRUE,
  NOT_REQUIRED
}

public static class RequirementFlagUtil {
  public static SlotRequirementFlag FromBool(bool flag) {
    if (flag) {
      return SlotRequirementFlag.TRUE;
    }
    else {
      return SlotRequirementFlag.FALSE;
    }
  }
}

public class MemorySlotSignature {
  public SlotRequirementFlag facingCozmo;
  public SlotRequirementFlag verticalStack;
  public int lightCount;

  public MemorySlotSignature(SlotRequirementFlag facingCozmo_, SlotRequirementFlag verticalStack_, int lightCount_) {
    facingCozmo = facingCozmo_;
    verticalStack = verticalStack_;
    lightCount = lightCount_;
    if (lightCount == 0) {
      DAS.Error("MemoryBankSignature", "Signatures should not have 0 lightcount");
    }
  }

  public override bool Equals(System.Object obj) {
    if (obj == null) {
      return false;
    }

    MemorySlotSignature p = obj as MemorySlotSignature;
    if ((System.Object)p == null) {
      return false;
    }
    return Equals(p);
  }

  public bool Equals(MemorySlotSignature signature) {
    if (signature == null)
      return false;

    return facingCozmo == signature.facingCozmo && verticalStack == signature.verticalStack && lightCount == signature.lightCount;
  }

  public bool Equals(BlockPattern pattern) {
    if (lightCount != pattern.blocks[0].NumberOfLightsOn()) {
      return false;
    }

    if (facingCozmo == SlotRequirementFlag.TRUE && !pattern.facingCozmo) {
      return false;
    }

    if (facingCozmo == SlotRequirementFlag.FALSE && pattern.facingCozmo) {
      return false;
    }

    if (verticalStack == SlotRequirementFlag.TRUE && !pattern.verticalStack) {
      return false;
    }

    if (verticalStack == SlotRequirementFlag.FALSE && pattern.verticalStack) {
      return false;
    }

    return true;
  }

  public override int GetHashCode() {
    return (int)facingCozmo ^ (int)verticalStack ^ lightCount;
  }

}

public class MemorySlot {

  MemorySlotSignature signature;

  public MemorySlot(MemorySlotSignature signature_) {
    signature = signature_;
  }

  public bool TryAdd(BlockPattern newPattern) {
    if (signature.Equals(newPattern)) {
      seenPatterns.Add(newPattern);
      return true;
    }
    return false;
  }

  public HashSet<BlockPattern> GetSeenPatterns() {
    return seenPatterns;
  }

  // seen patterns stored in cozmo space that fit the signature
  private HashSet<BlockPattern> seenPatterns = new HashSet<BlockPattern>();

}
