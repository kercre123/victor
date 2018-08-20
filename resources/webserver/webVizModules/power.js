/*  
 * power state and control 
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="power-title">Power State info</div>');

    var setEnablePowerSave = $('<input class="powerButton" type="button" value="Enable Power Save"/>');
    var setDisablePowerSave = $('<input class="powerButton" type="button" value="Disable Power Save"/>');

    setEnablePowerSave.click( function(){
      var payload = { 'enablePowerSave' : 'true' };
      sendData( payload );
    });
    setEnablePowerSave.appendTo( elem );

    setDisablePowerSave.click( function(){
      var payload = { 'enablePowerSave' : 'false' };
      sendData( payload );
    });
    setDisablePowerSave.appendTo( elem );

    powerInfoDiv = $('<h3 id="powerInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    powerInfoDiv.empty();
    powerInfoDiv.append('<div class="enabledTitle">' + "PowerSave Enabled: " + data["powerSaveEnabled"] + '</div>');
    powerInfoDiv.append('<div class="requesterTitle">' + "PowerSave Requesters: " + data["powerSaveRequesters"] + '</div>');
  };

  myMethods.update = function(dt, elem) { };

  myMethods.getStyles = function() {
    return `
      #power-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .enabledTitle {
        margin-bottom:10px;
      }
      
      .requesterTitle {
        margin-bottom:10px;
      }
      
      .powerButton {
        margin-bottom:20px;
        margin-right:10px;
      }
      
    `;
  };

})(moduleMethods, moduleSendDataFunc);

