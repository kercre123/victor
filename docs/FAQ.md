## FAQ

If you have a question that you get answered (e.g. in a Slack channel) which might plague others, consider creating an entry here for it.

* How do I set a console var fromm Webots?
  - Set the `consoleVarName` and `consoleVarValue` strings under the WebotsKeyboardController in the scene tree at left. Press `]` (closing square bracket) to send the message to the engine to set the console var. 
  - Using `}` (closing curly brace) instead will use the name and value strings as a console _function_ and its arguments instead.
  - Hold down `ALT` to send either of the above to the animation process to set console vars/functions there.