/*  
 * touch state and control system
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="touch-title">Touch Sensor</div>');

    var setUseDVTConfig = $('<input class="touchButton" type="button" value="Use DVT config"/>');
    var setUseProdConfig = $('<input class="touchButton" type="button" value="Use Production config"/>');

    setUseDVTConfig.click( function(){
      var payload = { 'filterConfig' : 'DVT' };
      sendData( payload );
    });
    setUseDVTConfig.appendTo( elem );

    setUseProdConfig.click( function(){
      var payload = { 'filterConfig' : 'Prod' };
      sendData( payload );
    });
    setUseProdConfig.appendTo( elem );
    
    touchInfoDiv = $('<h3 id="touchInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    touchInfoDiv.empty();
    touchInfoDiv.append('<div class="esnTitle">' + "ESN: " + data["esn"] + '</div>');
    touchInfoDiv.append('<div class="modeTitle">' + "mode: " + data["mode"] + '</div>');
    touchInfoDiv.append('<div class="countTitle">' + "count: " + data["count"] + '</div>');
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
      
      .modeTitle {
        margin-bottom:10px;
      }
      
      .countTitle {
        margin-bottom:10px;
      }
      
    `;
  };

})(moduleMethods, moduleSendDataFunc);
