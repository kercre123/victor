/*  
 * touch state and control system
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="touch-title">Touch Sensor</div>');

    var setDisableTouchSensor = $('<input class="touchButton" type="button" value="Disable touch sensor"/>');
    var setEnableTouchSensor = $('<input class="touchButton" type="button" value="Enable touch sensor"/>');
    var resetTouchCount = $('<input class="touchButton" type="button" value="Reset touch count"/>');

    setDisableTouchSensor.click( function(){
      var payload = { 'enabled' : 'false' };
      sendData( payload );
    });
    setDisableTouchSensor.appendTo( elem );

    setEnableTouchSensor.click( function(){
      var payload = { 'enabled' : 'true' };
      sendData( payload );
    });
    setEnableTouchSensor.appendTo( elem );

    resetTouchCount.click( function(){
      var payload = { 'resetCount' : 'true' };
      sendData( payload );
    });
    resetTouchCount.appendTo( elem );
    
    touchInfoDiv = $('<h3 id="touchInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    touchInfoDiv.empty();
    touchInfoDiv.append('<div class="esnTitle">' + "ESN: " + data["esn"] + '</div>');
    touchInfoDiv.append('<div class="osVersionTitle">' + "OS: " + data["osVersion"] + '</div>');
    touchInfoDiv.append('<div class="modeTitle">' + "enabled: " + data["enabled"] + '</div>');
    touchInfoDiv.append('<div class="countTitle">' + "count: " + data["count"] + '</div>');
    touchInfoDiv.append('<div class="countTitle">' + "calibrated: " + data["calibrated"] + '</div>');
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
    return `
      #touch-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .touchButton {
        margin-bottom:20px;
        margin-right:10px;
      }
      
      .esnTitle {
        margin-bottom:10px;
      }
      
      .osVersionTitle {
        margin-bottom:10px;
      }
      
      .modeTitle {
        margin-bottom:10px;
      }
      
      .countTitle {
        margin-bottom:10px;
      }
      
    `;
  };

})(moduleMethods, moduleSendDataFunc);
