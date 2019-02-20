/*  
 * Held-In-Palm "trust"-level tracking
 */

(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="heldinpalm-title">Held-In-Palm Status</div>');

    $('<h3 id="heldInPalmTrust">Held-In-Palm Trust: Unknown (sent infrequently)</h3>').appendTo( elem );
    $('<h3 id="lastTrustEventType">Last Trust Event: Unknown</h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    if(data == null) {
      // TODO:(bn) figure out why this ever happens....
      return;
    }

    if(data.hasOwnProperty('held_in_palm_trust_level')) {
      $('#heldInPalmTrust').text('Held-In-Palm Trust: ' + data['held_in_palm_trust_level'].toFixed(3));
    }
    
    if(data.hasOwnProperty('last_trust_event_type')) {
      $('#lastTrustEventType').text('Last Trust Event: ' + data['last_trust_event_type']);
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
