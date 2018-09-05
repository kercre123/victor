/*  
 * sleep tracking, beahviors, and power save
 */

(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="sleeping-title">Sleeping Status</div>');

    $('<h3 id="sleepDebt">Sleep Debt: Unknown (sent infrequently)</h3>').appendTo( elem );
    $('<h3 id="sleepState">Sleep Cycle: Unknown</h3>').appendTo( elem );
    $('<h3 id="sleepReaction">Sleeping Reaction: Unknown</h3>').appendTo( elem );
    $('<h3 id="sleepReason">Last Sleep Reason: Unknown</h3>').appendTo( elem );
    $('<h3 id="wakeReason">Last Wake Reason: Unknown</h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    if(data == null) {
      // TODO:(bn) figure out why this ever happens....
      return;
    }

    if(data.hasOwnProperty('sleep_debt_hours')) {
      $('#sleepDebt').text('Sleep Debt: ' + data['sleep_debt_hours'].toFixed(3) + ' hours');
    }

    if(data.hasOwnProperty('sleep_cycle')) {
      $('#sleepState').text('Sleep Cycle: ' + data['sleep_cycle']);
    }

    if(data.hasOwnProperty('reaction_state')) {
      $('#sleepReaction').text('Sleeping Reaction: ' + data['reaction_state']);
    }

    if(data.hasOwnProperty('last_sleep_reason')) {
      $('#sleepReason').text('Last Sleep Reason: ' + data['last_sleep_reason']);
    }

    if(data.hasOwnProperty('last_wake_reason')) {
      $('#wakeReason').text('Last Wake Reason: ' + data['last_wake_reason']);
    }
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
    return `
      #sleeping-title {
        font-size:16px;
        margin-bottom:20px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
