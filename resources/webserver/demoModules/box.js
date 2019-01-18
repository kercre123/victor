/*  
 * box stats
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="vision-title">Touch Sensor</div>');

    $('<div class="image-stat-container">' +
      '<div class="vision-stat-label">Wakeword Events</div>' +
      '<span class="vision-stat" id="wakewordEvents">0</span>' +
      '</div>').appendTo( elem );

  };

  myMethods.onData = function(data, elem) {
    if(data == null) {
      // TODO:(bn) figure out why this ever happens....
      return;
    }

    // console.log(data)

    function numberWithCommas(x) {
      return x.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
    }

    if(data.hasOwnProperty('wakeword_count')) {
      $('#wakewordEvents').text( numberWithCommas(data['wakeword_count']) );
    }
  };

  myMethods.update = function(dt, elem) { 
  };

  myMethods.getStyles = function() {
  };

})(moduleMethods, moduleSendDataFunc);
