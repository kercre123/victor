/*  
 * Held-In-Palm "trust"-level tracking
 */

(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="heldinpalm-title">Held-In-Palm Status</div>');
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    if(data == null) {
      // TODO:(GB) figure out why this ever happens....
      return;
    }
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
    return `
      #heldinpalm-title {
        font-size:16px;
        margin-bottom:20px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
