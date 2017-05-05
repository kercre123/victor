/**
 * Utility function shared by the Tutorial, Challenges, and Save/Load Projects pages
 * @author Adam Platti
 * @since May 4, 2017
 */

window._$translations = window._$translations || {};  // replaced later with translation dictionary

/**
 * Returns localized strings for display.  Accepts optional parameters
 *  to substitue into localized string.
 *
 * Example:  $t("{0} Unicorns", 5)  returns  "5 Unicorns"
 *
 * @param {String} str - localized string with possible substitution flags
 * @param
 */
window.$t = function $t(str) {
  // use a translation if you find one, or echo back the string passed in
  str = window._$translations[str] || str;

  // subsitute values into the translated string if params are passed
  if (arguments.length > 1) {
    var numSubs = arguments.length - 1;
    for (var i=0; i<numSubs; i++) {
      // substitute the values in the string like "{0}"
      str = str.replace('{'+i+'}', arguments[i+1]);
    }
  }
  return str;
};


/**
 * Sets text inside an element described by a CSS selector
 * @param {String} selector - CSS selector of element to set text for
 * @param {String} text - text to apply to element
 * @returns {void}
 */
window.setText = function setText(selector, text) {
  var elem = document.querySelector(selector);
  if (elem) {
    elem.textContent = text;
  }
};

/**
 * Fetches JSON and passes it to a callback
 * @param {String} url - url to fetch
 * @param {Function} callback - function called to accept JSON data
 * @returns {void}
 */
window.getJSON = function(url, callback) {
  var request = new XMLHttpRequest();
  request.open('GET', url, true);

  request.onload = function() {
    if (this.status >= 200 && this.status < 400) {
      // Success!
      var data = JSON.parse(this.response);
      callback(data);
    }
  };
  request.send();
};



/**
 * Returns category color of a puzzle block.  Used for coloring the icon.
 * @param {String} icon - name of the icon
 * @returns {String} category color of the icon
 */
window.getBlockIconColor = function(icon) {
  switch (icon) {

    // motion blocks
    case 'cozmo-backward':
    case 'cozmo-backward-fast':
    case 'cozmo-forward':
    case 'cozmo-forward-fast':
    case 'cozmo-turn-left':
    case 'cozmo-turn-right':
    case 'cozmo-dock-with-cube':
      return 'blue';

    // looks blocks
    case 'cozmo-forklift-high':
    case 'cozmo-forklift-low':
    case 'cozmo-forklift-medium':
    case 'cozmo-head-angle-high':
    case 'cozmo-head-angle-medium':
    case 'cozmo-head-angle-low':
    case 'set-led_black':
    case 'set-led_blue':
    case 'set-led_coral':
    case 'set-led_green':
    case 'set-led_mystery':
    case 'set-led_orange':
    case 'set-led_purple':
    case 'set-led_white':
    case 'set-led_yellow':
    case 'cozmo_says':
      return 'purple';

    // event blocks
    case 'cozmo-face':
    case 'cozmo-face-happy':
    case 'cozmo-face-sad':
    case 'cozmo-cube':
    case 'cozmo-cube-tap':
      return 'yellow';

    // control blocks
    case 'control_forever':
    case 'control_repeat':
      return 'orange';

    // actions blocks
    case 'cozmo-anim-bored':
    case 'cozmo_anim-cat':
    case 'cozmo_anim-chatty':
    case 'cozmo-anim-dejected':
    case 'cozmo-anim-dog':
    case 'cozmo-anim-excited':
    case 'cozmo-anim-frustrated':
    case 'cozmo-anim-happy':
    case 'cozmo-anim-mystery':
    case 'cozmo-anim-sleep':
    case 'cozmo-anim-sneeze':
    case 'cozmo-anim-surprise':
    case 'cozmo-anim-thinking':
    case 'cozmo-anim-unhappy':
    case 'cozmo-anim-victory':
      return 'pink';

    // no default to make mistakes more obvious
  }
}
