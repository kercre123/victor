If you have firewall enabled, you will need to create a certificate in your Keychain Access to allow webotsTest.py to dynamically add webots executables to the firewall exception list.

**If you run into any problems regarding the firewall, reset your firewall preferences by deleting `/Library/Preferences/com.apple.alf.plist`**

Deleting that plist will reset your firewall preferences including any applications you've previously allowed to be on the firewall exception list. This is generally not a big deal as those applications will just prompt again to be allowed on the firewall exception list when they need network access again, but just be aware. You can also check this list by going to System Preferences > Security & Privacy > Firewall > Firewall Options...

*Modified from https://gist.github.com/jpmens/6136898a5d68c650956b*

1. Open Keychain Access.
2. Go to the Keychain Access menu, and under Certificate Assistant, choose Create a Certificate
3. Create Your Certificate
    * Name: WebotsFirewall
    * Identity Type: Self Signed Root
    * Certificate Type: Code Signing
    * *Enable "Let me orverride defaults"*
4. Certificate Information
    * Serial Number (use a random number): 38255937
    * Validity Period: 2000
5. Certificate Information
    * Organization: Anki
    * Organizational Unit: BTA
    * *Leave the rest of the fields empty*
6. Key Pair Information - *Accept defaults and continue*
7. Key Usage Extension - *Accept defaults and continue*
8. Extended Key Usage Extension - *Accept defaults and continue*
9. Basic Constraints Extension - *Accept defaults and continue*
10. Subject Alternate Name Extension - *Accept defaults and continue*
11. Keychain: login - *Press Create*
12. Go to keychain, and right click (control click) on the new certificate and choose Get Info.
13. Open the triangle next to Trust.
14. Go down to Code Signing, and choose Always Trust.
15. Close the box. The system will ask for your admin password. Enter it and click OK.

The first time you run the script with firewall enabled, a popup might ask if you allow the signing with WebotsFirewall certificate. Press "Always Allow".