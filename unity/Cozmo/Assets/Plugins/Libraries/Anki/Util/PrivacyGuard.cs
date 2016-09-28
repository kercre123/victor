
/*
 * PrivacyGuard.cs
 *
 * Unity version of Anki::Util::HidePersonallyIdentifiableInfo()
 *
 * Privacy guard is always enabled for shipping configurations.
 * Privacy guard may be enabled for debug or release configurations
 * by adding ENABLE_PRIVACY_GUARD to build flags.
 *
 */

#if SHIPPING
#define ENABLE_PRIVACY_GUARD
#endif

public class PrivacyGuard {
  public static string HidePersonallyIdentifiableInfo(string s) {
#if ENABLE_PRIVACY_GUARD
    return "<PII>";
#else
    return s;
#endif
  }
}

