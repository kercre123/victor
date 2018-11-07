/*  
 * alexa state
 */

(function(myMethods, sendData) {
  
  var authState;
  var uxState;
  myMethods.init = function(elem) {
    authState = $('<div></div>').appendTo(elem);
    uxState = $('<div></div>').appendTo(elem);
  };

  function SetAuthState(state) {
    authState.html('<span class="label">Auth State: </span> ' + state);
  }

  function SetUXState(state) {
    uxState.html('<span class="label">UX State: </span> ' + state);
  }

  myMethods.onData = function(data, elem) {
    if( typeof data["authState"] !== 'undefined' ) {
      SetAuthState(data["authState"]);
    }
    if( typeof data["uxState"] !== 'undefined' ) {
      SetUXState(data["uxState"]);
    }
  };

  myMethods.update = function(dt, elem) { };

  myMethods.getStyles = function() {
    return `
      .label {
        font-weight: bold;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);

